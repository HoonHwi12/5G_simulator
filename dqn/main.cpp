#include "DQN.h"
TORCH_MODULE(DQN); // shared ownership
#include "ReplayMemory.h"
#include "EpsilonGreedy.h"
#include "Agent.h"
#include "LTENetworkState.h"
#include <torch/torch.h>
// pipe includes
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
// others
#include <iostream>
#include <cstdlib>

#include "../src/shared-memory.h"

#define STATE_FIFO "../../state_fifo"
#define SCHED_FIFO "../../sched_fifo"
#define CQI_FIFO   "../../cqi_fifo"

typedef std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor> experience;

void ConnectSchedulerFifo(int *fd);
void OpenStateFifo(int *fd, int *noUEs);
void OpenCQIFifo(int *fd);
std::string FetchState(int *fd);
std::string FetchCQIs(int *fd);
void FetchInitUEs(int *fd, LTENetworkState *network_state);
LTENetworkState* initConnections(int* _sh_fd, int* _st_fd, int* _cqi_fd);

void SendScheduler(int *fd, int scheduler0, int scheduler1=0, int scheduler2=0, int scheduler3=0);
void SendFinalScheduler(int *fd);

experience processSamples(std::vector<experience> _samples);
void loadStateDict(DQN model, DQN target_model);

void initWeights(torch::nn::Module& m);

/* HyperParams*/
const int BATCH_SIZE        = 32;
int TRAIN_TTI               = 200; //20000;
const int TEST_TTI          = 200;//2500;
const int MIN_REPLAY_MEM    = 10;// 1000;
const float GAMMA           = 0.999;  // discount factor for bellman equation
const float EPS_START       = 1.0;    // greedy stuff
const float EPS_END         = 0.01;
const float EPS_DECAY       = 0.001;
const int NET_UPDATE        = 10;     // how many episodes until we update the target DQN 

const int TRAIN_FREQ        = 4;

const int MEM_SIZE          = 50000; // replay memory size
const float LR_START        = 0.01;
const float LR_END          = 0.00001;
const float LR_DECAY        = 0.001;
const float MOMENTUM        = 0.05;  // SGD MOMENTUM
 // environment concerns
const int NUM_ACTIONS       = 6;    // number of schedulers
const int CQI_SIZE          = 25;   // number of downlink channels per eNB

//Adaptive DQN
//const int ADA_ACTIONS       = 17569; // 14641 + 2928
//* hyunji
const int ADA_ACTIONS       = 11;
const int ADA_ACTION        = 44;

const int NUM_OUTPUT        = 4;

// training times
std::chrono::steady_clock::time_point start;
std::chrono::steady_clock::time_point end;
std::chrono::nanoseconds duration;

int main(int argc, char** argv) {
	int constant_scheduler = 0;
  bool use_dqn = false;
  bool model_saved = false;

  printf("batch(%d)/minReplay(%d)/EPS(%0.2f~%0.2f/%0.4f)/Update(%d)\n",
    BATCH_SIZE, MIN_REPLAY_MEM, EPS_START, EPS_END, EPS_DECAY, NET_UPDATE);
  // file naming
  std::string scheduler_string = "dqn";
  std::string model_number = "0";
  bool is_load=false;

  // by HH test time
  clock_t test_start = clock();
  
	if(argc >= 2 )
  {
    // use fixed scheduler or dqn
		constant_scheduler = atoi(argv[1]);
    scheduler_string = argv[1];
    if(constant_scheduler==7)
    {
      use_dqn = true;
      scheduler_string = "dqn";
    }
	} 
  if(argc >= 3)
  {
    if( strcmp(argv[2], "0")==0) use_lstm=false;          // default=false
    else use_lstm=true;
  }
  if(argc >= 4) model_number = argv[3]; // default="0"
  if(argc == 5)
  {
    if( strcmp(argv[4], "0")==0 ) is_load=false;          // default=false
    else is_load=true;
  }

  // connect shared memory
  int dqn_shmid = SharedMemoryCreate(DQN_KEY);
  char *dqn_buffer = (char*)malloc(SHARED_SIZE);
  if(dqn_shmid == -1)
  {
    printf("Shared Memory Create Error\n");
    return FAIL;
  }


  // LTE-Sim에 LSTM 사용여부 전송
  if(use_lstm == false)
  {    
    sprintf(dqn_buffer,"%d", -1);
  }
  else
  {
    sprintf(dqn_buffer,"%d", 0x89);
  }

  if( SharedMemoryWrite(dqn_shmid, dqn_buffer) == -1)
  {
    printf("shared memory write failed\n");
  }


  std::cout << "PYTORCH version " << TORCH_VERSION << std::endl;

	// Decide CPU or GPU
	torch::Device device = torch::kCPU;
	std::cout << "CUDA DEVICE COUNT: " << torch::cuda::device_count() << std::endl;
	if (torch::cuda::is_available()) {
    	std::cout << "CUDA available - working on GPU." << std::endl;
    	device = torch::kCUDA;
  }
  // pipe 
  int sh_fd, st_fd, cqi_fd;
  // initial connections
  LTENetworkState* networkEnv = initConnections(&sh_fd, &st_fd, &cqi_fd);
  int noUEs = networkEnv->noUEs;

  // simulation output traces
  std::string update;
  std::string cqi_update;

   // DQN components
  torch::Tensor reset_state        =   networkEnv->reset().to(device);
  torch::Tensor state;
  ReplayMemory* exp                = new ReplayMemory(MEM_SIZE);

  EpsilonGreedy* eps               = new EpsilonGreedy(EPS_START, EPS_END, EPS_DECAY);// start, end, decay
  EpsilonGreedy* lr_rate           = new EpsilonGreedy(LR_START, LR_END, LR_DECAY);

  //* hyunji
  //torch::Tensor ada_actions = torch::zeros({NUM_OUTPUT,ADA_ACTIONS});
  Agent<EpsilonGreedy, DQN>* agent = new Agent<EpsilonGreedy, DQN>(eps,ADA_ACTIONS);
  DQN policyNet(reset_state.size(1), ADA_ACTION);
  DQN targetNet(reset_state.size(1), ADA_ACTION);

  // logging files for training 
  //  ~ please make sure that test_results/ is valid folder
  std::string log_file_name, model_name, base;
  base               = "test_results/" + scheduler_string + std::to_string(noUEs);
  log_file_name      = base + "_training.txt";
  model_name         = base + "_" + model_number + "_model.pt";
  h_log("File name ready\n");

  // by HH LOAD MODEL
  if(is_load)
  {
    torch::load(policyNet, model_name);
    printf("load model success, waiting for LTE-Sim\n");
  }
  else{
    policyNet->apply(initWeights);
  }

  // copy weights to targetnet
  loadStateDict(policyNet, targetNet);

  policyNet->to(device);
  targetNet->to(device);
  // setting up training variables
  std::vector<experience> samples;
  h_log("samples ready\n");
  torch::Tensor current_q_values, next_q_values, target_q_values;
  torch::optim::Adam optimizer(policyNet->parameters(), torch::optim::AdamOptions(LR_START));
  h_log("optimizer ready\n");

  // get update from first update packets
  update     = FetchState(&st_fd); 
  h_log("fetch state complete\n");
  cqi_update = FetchCQIs(&cqi_fd);
  h_log("fetch cqi complete\n");
  networkEnv->UpdateNetworkState(update); 
  h_log("update networkstates complete\n");
  networkEnv->ProcessCQIs(cqi_update);
  // training loop variables
  int valid_TTI_explore = 0;
  int valid_TTI_exploit = 0;
  float reward_copy = 0;
  int update_counter = 0;
  int training_freq = 0;
  bool explore = true;

  if(!use_dqn)
  {
    TRAIN_TTI += TEST_TTI;
  }
  while(1){ // training loop
    h_log("entering while(1)\n");
  	torch::Tensor state = networkEnv->CurrentState(true);
  	networkEnv->TTI_increment();

  	// selecting an action
    torch::Tensor action = torch::zeros({2,4});
    torch::Tensor action_input = torch::zeros(4);

    h_log("action ready\n");

    if(use_dqn) // select action explore/exploit in dqn
    {
      if(networkEnv->TTIcounter > TRAIN_TTI)
      {
        action = agent->exploit(state.to(device), policyNet, true);
      }
      else
      {
        action = agent->selectAction(state.to(device), policyNet);

        if(use_dqn && action[0][0].item<int>() >= 0)
        {
          /*
          action_input.index_put_({0}, action[0][0].item<int>() * 1331
                + action[0][1].item<int>() * 121
                + action[0][2].item<int>() * 11
                + action[0][3].item<int>() );
                */
          action_input.index_put_({0}, action[0][0]);
          action_input.index_put_({1}, action[0][1]);
          action_input.index_put_({2}, action[0][2]);
          action_input.index_put_({3}, action[0][3]);
        }
        else
        {
          action_input.index_put_({0}, action[0][2].item<int>()); // fls
        }
      }
    }
    else { // use fixed scheduler
      action.index_put_({0,0}, -1);
      action.index_put_({0,1}, constant_scheduler);
      action.index_put_({0,2}, -1);
      action.index_put_({0,3}, -1);
      action.index_put_({1}, -1);
    }
    h_log("select action complete\n");

    SendScheduler(&sh_fd, action[0][0].item<int>(), action[0][1].item<int>(), action[0][2].item<int>(), action[0][3].item<int>());

  	// observe new state
  	update     = FetchState(&st_fd); 
    cqi_update = FetchCQIs(&cqi_fd);
    if(strcmp(update.c_str(), "end") == 0){ // check if end of simulation
      std::cout << "END signal received!" << std::endl;
      break;
  	}
    update_counter++;
    
    // process new state + cqis
    networkEnv->UpdateNetworkState(update); 
    networkEnv->ProcessCQIs(cqi_update);
    // next state and reward
    torch::Tensor reward = networkEnv->CalculateReward(); 
    reward_copy = reward[0].item<float>();

    if( (networkEnv->TTIcounter < TRAIN_TTI) && use_dqn)
    {
      torch::Tensor next_state  = networkEnv->CurrentState(false);
      // store experiece in replay memory
      exp->push(state, action_input.to(torch::kCPU), next_state, reward); 

      start = std::chrono::steady_clock::now(); //training time logging
      clock_t infstart=clock();

      if( !use_lstm || (int)networkEnv->TTIcounter %10 ==0)
      {
        // if enough samples
        if(exp->canProvideSamples((size_t)MIN_REPLAY_MEM)){ 
          update_counter++;
          // access learning rate
          auto options = static_cast<torch::optim::AdamOptions&> (optimizer.defaults());
          options.lr(lr_rate->explorationRate(networkEnv->TTIcounter - MIN_REPLAY_MEM));
          h_log("lr update\n");

          //sample random batch and process
          samples = exp->sampleMemory(BATCH_SIZE); 
          experience batch = processSamples(samples);
      
          // work out the qs
          //* HJ
          current_q_values = agent->CurrentQ(policyNet, std::get<0>(batch));
          current_q_values = current_q_values.gather(1,std::get<1>(batch));
          h_log("currenQ ready\n");
          next_q_values = (agent->NextQ(targetNet, std::get<2>(batch))).to(torch::kCPU);
          next_q_values = std::get<1>(next_q_values.max(1));

          torch::Tensor abs = at::abs(current_q_values);
          torch::Tensor max_q = at::max(abs);
          float m_q = max_q.item<float>();

          // bellman equation
          target_q_values = (next_q_values.multiply(GAMMA)) +  std::get<3>(batch);
          // loss and backprop
          torch::Tensor loss = (torch::mse_loss(current_q_values.to(device), target_q_values.to(device))).to(device);
          printf("loss %f\n", loss.item().toFloat());

          loss.set_requires_grad(true);
          optimizer.zero_grad();
          loss.backward();
          optimizer.step();
          h_log("optimizer step complete\n");

          // update targetNet with policyNey parameters
          if(update_counter > NET_UPDATE){
            // copy weights to targetnet
            loadStateDict(policyNet, targetNet);
            update_counter = 0;
          }
        } // enough samples
      }

      // training time logging
      end = std::chrono::steady_clock::now();
      duration = end - start;    
      h_log("time logging\n");
      // checks the valid-TTI's explore/exploitation count
      if (action[1][0].item<int>() == 0) valid_TTI_exploit++;
      else if(action[1][0].item<int>() > 0) valid_TTI_explore++;
      printf("\tInferenceTime %0.7f ms\tExploit %d,\tExplore %d\n", (float)(clock()-infstart)/CLOCKS_PER_SEC, valid_TTI_exploit, valid_TTI_explore);

    } // training loop
    
    if( model_saved == false && (networkEnv->TTIcounter >= TRAIN_TTI ))
    {
      torch::NoGradGuard no_grad;
      model_saved = true;
      torch::save(policyNet, model_name);
    }

    if(networkEnv->TTIcounter >= (TRAIN_TTI + TEST_TTI)) break;
  }// training loop

  // log training loop satisfaction rates, false flag signals training
  networkEnv->log_satisfaction_rates(scheduler_string, noUEs, false);

  // send end signal
  SendFinalScheduler(&sh_fd);

  close(sh_fd);
  close(st_fd);
  close(cqi_fd);
  delete networkEnv;

  printf("TEST END, Test Duration: %0.4f s\n", (float)(clock()-test_start) / CLOCKS_PER_SEC);

  return 0;
}

void initWeights(torch::nn::Module& m){
  if ((typeid(m) == typeid(torch::nn::LinearImpl)) || (typeid(m) == typeid(torch::nn::Linear))) {
    auto p = m.named_parameters(false);
    auto w = p.find("weight");
    auto b = p.find("bias");

    if (w != nullptr) torch::nn::init::xavier_uniform_(*w);
    if (b != nullptr) torch::nn::init::constant_(*b, 0.01);
  }
}

void loadStateDict(DQN model, DQN target_model) {
  torch::autograd::GradMode::set_enabled(false);  // make parameters copying possible
  auto new_params = target_model->named_parameters(); // implement this
  auto params = model->named_parameters(true /*recurse*/);
  auto buffers = model->named_buffers(true /*recurse*/);

//   int i=0;
  for (auto& val : new_params) {
    //i++;
    auto name = val.key();
    auto* t = params.find(name);
    if (t != nullptr) {
      t->copy_(val.value());
    } else {
      t = buffers.find(name);
      if (t != nullptr) {
        t->copy_(val.value());
      }
    }
    //if(i>490000) break;
  }
  torch::autograd::GradMode::set_enabled(true);
}

experience processSamples(std::vector<experience> _samples){
  std::vector<torch::Tensor> states;
  std::vector<torch::Tensor> actions;
  std::vector<torch::Tensor> new_states;
  std::vector<torch::Tensor> rewards;
  for (auto i : _samples){
    states.push_back(std::get<0>(i));
    actions.push_back(std::get<1>(i));
    new_states.push_back(std::get<2>(i));
    rewards.push_back(std::get<3>(i));
  }

  torch::Tensor states_tensor = torch::zeros(0);
  torch::Tensor new_states_tensor = torch::zeros(0);
  torch::Tensor actions_tensor = torch::zeros({0,4});
  torch::Tensor rewards_tensor = torch::zeros(0);

  states_tensor = torch::cat(states, 0);
  actions_tensor = torch::cat(actions, 0);
  new_states_tensor = torch::cat(new_states, 0);
  rewards_tensor = torch::cat(rewards, 0);

  experience _batch(states_tensor , actions_tensor, new_states_tensor , rewards_tensor);
  return _batch;
}

void ConnectSchedulerFifo(int *fd){
  // connect to scheduler fifo
  *fd = open(SCHED_FIFO, O_CREAT|O_WRONLY);
  close(*fd);
}

void OpenCQIFifo(int *fd){
  // create the cqi fifo
  printf("mk cqi fifo\n");
  mkfifo(CQI_FIFO, S_IFIFO|0777);
  // block for LTESim to connect
  *fd = open(CQI_FIFO, O_RDONLY);
  printf("close cqi fifo\n");
  close(*fd);
}

void OpenStateFifo(int *fd, int *noUEs){
  // create the state fifo
  mkfifo(STATE_FIFO, S_IFIFO|0777);
  char noUEs_in[80];
  int input_bytes;

  // block for LTESim to connect
  *fd = open(STATE_FIFO, O_RDONLY);
  
  // read the number of UEs
  input_bytes = read(*fd, noUEs_in, sizeof(noUEs_in));
  close(*fd);
  *noUEs = atoi(noUEs_in);
}

std::string FetchState(int *fd){
  // read state size
  *fd = open(STATE_FIFO, O_RDONLY);
  std::string::size_type size;
  read(*fd, &size, sizeof(size));
  // read state/update from LTEsim
  std::string message(size, ' ');
  read(*fd, &message[0], size);
  close(*fd);
  return message;
}

std::string FetchCQIs(int *fd){
  // read CQI message size
  *fd = open(CQI_FIFO, O_RDONLY);
  std::string::size_type size;
  read(*fd, &size, sizeof(size));
  std::string message(size, ' ');
  // read the CQIs
  read(*fd, &message[0], size);
  close(*fd);
  return message;
}

void FetchInitUEs(int *fd, LTENetworkState *network_state){
  // read size of UE summary
  *fd = open(STATE_FIFO, O_RDONLY);

  h_log("read fd\n");
  std::string::size_type size = 0;
  read(*fd, &size, sizeof(size));
  h_log("read finish, size:%d\n", size);
  std::string message(size, ' ');
  // read the UE summary
  h_log("read message\n");
  read(*fd, &message[0], size);
  close(*fd);
  h_log("init state\n");
  network_state->InitState(message);
}

void SendScheduler(int *fd, int scheduler0, int scheduler1, int scheduler2, int scheduler3){
  std::string sched = std::to_string(scheduler0)+"|"+
                      std::to_string(scheduler1)+"|"+
                      std::to_string(scheduler2)+"|"+
                      std::to_string(scheduler3);

  char const *scheduler_send = sched.c_str();
  // send scheduler
  *fd = open(SCHED_FIFO, O_CREAT|O_WRONLY);
  write(*fd, scheduler_send, strlen(scheduler_send));

  close(*fd);
}

void SendFinalScheduler(int *fd)
{
  std::string sched = "end";

  char const *scheduler_send = sched.c_str();
  // send scheduler
  *fd = open(SCHED_FIFO, O_CREAT|O_WRONLY);
  write(*fd, scheduler_send, strlen(scheduler_send));

  close(*fd);
}


LTENetworkState* initConnections(int* _sh_fd, int* _st_fd, int* _cqi_fd){
	// connect to the scheduler pipe
  h_log("connect scheduler fifo\n");
	ConnectSchedulerFifo(_sh_fd);
	// open CQI fifo
  
  h_log("open cqi fifo\n");
  OpenCQIFifo(_cqi_fd);
  // open state fifo,connect and fetch #UEs
  int _noUEs = 0;
  h_log("open state fifo\n");
  OpenStateFifo(_st_fd, &_noUEs);
  
  // initialise the network state environment
  LTENetworkState *networkEnv = new LTENetworkState(_noUEs, CQI_SIZE);
  // initialise UE and Application from LTE-sim
  h_log("fetch init ues\n");
  FetchInitUEs(_st_fd, networkEnv);
  // print a summary
  networkEnv->print_summary();

  h_log("init connections end\n");
  return networkEnv;

}
