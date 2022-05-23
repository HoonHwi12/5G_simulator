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
void SendScheduler(int *fd, int scheduler);
void SendFinalScheduler(int *fd);
experience processSamples(std::vector<experience> _samples);
void loadStateDict(DQN model, DQN target_model);


/* HyperParams*/
const int BATCH_SIZE        = 32;
int TRAIN_TTI               = 10000;
const int TEST_TTI          = 3000;
const int MIN_REPLAY_MEM    = 1000;
const float GAMMA           = 0.999;  // discount factor for bellman equation
const float EPS_START       = 1.0;    // greedy stuff
const float EPS_END         = 0.01;
const float EPS_DECAY       = 0.0001;
const int NET_UPDATE        = 10;     // how many episodes until we update the target DQN 
const int MEM_SIZE          = 50000; // replay memory size
//const float LR              = 0.01;  // learning rate
const float LR_START        = 0.01;
const float LR_END          = 0.00001;
const float LR_DECAY        = 0.0001;
const float MOMENTUM        = 0.05;  // SGD MOMENTUM
 // environment concerns
const int NUM_ACTIONS       = 6;    // number of schedulers
const int CQI_SIZE          = 25;   // number of downlink channels per eNB


// training times
std::chrono::steady_clock::time_point start;
std::chrono::steady_clock::time_point end;
std::chrono::nanoseconds duration;

int main(int argc, char** argv) {
	int constant_scheduler = 0;
  bool use_dqn = true;
  // file naming
  std::string scheduler_string;
	if(argc > 1){

    // use fixed scheduler or dqn
		constant_scheduler = atoi(argv[1]);
		scheduler_string = argv[1];
    use_dqn = false;
    printf("scheduler is %d\n", constant_scheduler);
	} else {
    scheduler_string = "dqn";
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

	// Decide CPU or GPU
	torch::Device device = torch::kCPU;
  
  std::cout << "CUDA available: " << torch::cuda::is_available() << std::endl;

	std::cout << "CUDA DEVICE COUNT: " << torch::cuda::device_count() << std::endl;
	if (torch::cuda::is_available()) {
    	std::cout << "CUDA available - working on GPU." << std::endl;
    	device = torch::kCUDA;
  }
  // pipe 
  int sh_fd, st_fd, cqi_fd;

  // remove(SCHED_FIFO);
  // remove(STATE_FIFO);
  // remove(CQI_FIFO);

  // initial connections
  LTENetworkState* networkEnv = initConnections(&sh_fd, &st_fd, &cqi_fd);
  h_log("init connection end\n");
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
  Agent<EpsilonGreedy, DQN>* agent = new Agent<EpsilonGreedy, DQN>(eps,NUM_ACTIONS);
  DQN policyNet(reset_state.size(1), NUM_ACTIONS); 
  DQN targetNet(reset_state.size(1), NUM_ACTIONS);

  // logging files for training 
  //  ~ please make sure that test_results/ is valid folder
  std::string log_file_name, model_name, base;
  base               = "test_results/" + scheduler_string + "_" + std::to_string(noUEs);
  log_file_name      = base + "_training.txt";
  model_name         = base + "_model.pt";
  std::ofstream output_file(log_file_name);

  // copy weights to targetnet
  loadStateDict(policyNet, targetNet); 
  policyNet->to(device);
  targetNet->to(device);

  // setting up training variables
  std::vector<experience> samples;
  torch::Tensor current_q_values, next_q_values, target_q_values;
  torch::optim::SGD optimizer(policyNet->parameters(), torch::optim::SGDOptions(LR_START).momentum(MOMENTUM));

  // get update from first update packets
  h_log("fetch state\n");
  update     = FetchState(&st_fd); 
  h_log("fetch cqi\n");
  cqi_update = FetchCQIs(&cqi_fd);
  h_log("fetch complete\n");
  networkEnv->UpdateNetworkState(update); 
  networkEnv->ProcessCQIs(cqi_update);
  
  // training loop variables
  int reward_counter = 0;
  int valid_TTI_explore = 0;
  int valid_TTI_exploit = 0;
  float reward_copy = 0;
  bool explore = true;
  int update_counter = 0;

  if(!use_dqn)
  {
    TRAIN_TTI += TEST_TTI;
  }

  while(1){ // training loop
  	torch::Tensor state = networkEnv->CurrentState(true);
  	networkEnv->TTI_increment();

  	// selecting an action
    torch::Tensor action = torch::zeros(2);
    if(use_dqn){ // select action explore/exploit in dqn
      action = agent->selectAction(state.to(device), policyNet);
    } else { // use fixed scheduler
      action.index_put_({0}, constant_scheduler);
      action.index_put_({1}, 0);
    }
    h_log("send scheduler\n");

    // execute action
    //printf("send scheduler: %d\n", action[0].item<int>());
  	SendScheduler(&sh_fd, action[0].item<int>());
  	// observe new state
    h_log("fetch state\n");
  	update     = FetchState(&st_fd); 
    cqi_update = FetchCQIs(&cqi_fd);
    if(strcmp(update.c_str(), "end") == 0){ // check if end of simulation
      std::cout << "END signal received!" << std::endl;
      break;
  	}
    update_counter++;
    // process new state + cqis
    h_log("update state\n");
    networkEnv->UpdateNetworkState(update); 

    h_log("PROCESS CQIS\n");
    networkEnv->ProcessCQIs(cqi_update);
    // next state and reward
    torch::Tensor next_state  = networkEnv->CurrentState(false);
    torch::Tensor reward = networkEnv->CalculateReward(); 
    reward_copy = reward[0].item<float>();
    // store experiece in replay memory
    h_log("STORE TO EXP\n");
    exp->push(state.to(torch::kCPU), action.to(torch::kCPU), next_state.to(torch::kCPU), reward.to(torch::kCPU)); 

    start = std::chrono::steady_clock::now(); //training time logging
    clock_t infstart = clock();
    if(use_dqn){ // if we are using dqn
    	// if enough samples
	    if(exp->canProvideSamples((size_t)MIN_REPLAY_MEM)){ 
        update_counter++;
        // access learning rate
        auto options = static_cast<torch::optim::SGDOptions&> (optimizer.defaults());
	   		options.lr(lr_rate->explorationRate(networkEnv->TTIcounter - MIN_REPLAY_MEM));
	    	//sample random batch and process
	    	samples = exp->sampleMemory(BATCH_SIZE); 
        experience batch = processSamples(samples);
        // work out the qs
        current_q_values = agent->CurrentQ(policyNet, std::get<0>(batch), std::get<1>(batch));
	      next_q_values = (agent->NextQ(targetNet, std::get<2>(batch))).to(torch::kCPU);
        // bellman equation
        target_q_values = (next_q_values.multiply(GAMMA)) +  std::get<3>(batch);
        // loss and backprop
        torch::Tensor loss = (torch::mse_loss(current_q_values.to(device), target_q_values.to(device))).to(device);
        //printf("loss  %f \n", loss.item().toFloat());
        loss.set_requires_grad(true);
        optimizer.zero_grad();
        loss.backward();
        optimizer.step();
        // update targetNet with policyNey parameters
        if(update_counter > NET_UPDATE){
	        // copy weights to targetnet
	        loadStateDict(policyNet, targetNet); 
	        update_counter = 0;
        }
	    } // enough samples
    }// fixed scheduler

    // training time logging
    end = std::chrono::steady_clock::now();
    duration = end - start;
    
    // checks the valid-TTI's explore/exploitation count
    if (action[1].item<int>() == 0) valid_TTI_exploit++;
    else if(action[1].item<int>() > 0) valid_TTI_explore++;

    printf("\tInferenceTime %0.7f ms\tExploit %d,\tExplore %d\n", (float)(clock()-infstart)/CLOCKS_PER_SEC, valid_TTI_exploit, valid_TTI_explore);

    //printf("\tExploit: %d,\tExplore: %d\t\n\n", valid_TTI_exploit, valid_TTI_explore);

    //output to file 
    if((int)networkEnv->TTIcounter % 100 == 0){
      if(use_dqn){
        output_file << networkEnv->TTIcounter << ", " << reward_copy << ", " << valid_TTI_explore << ", " << valid_TTI_exploit << ", "  << duration.count() << std::endl;
      } else {
        output_file << networkEnv->TTIcounter << ", " << reward_copy <<  ", "  << duration.count() << std::endl;
      }
    }
    
    // decide to break to testing
    if((networkEnv->TTIcounter > TRAIN_TTI )){
      networkEnv->TTI_increment();
      // scheduler 11 stops the UEs at current position
      //SendScheduler(&sh_fd, 11);
      SendScheduler(&sh_fd, 0); // HH EDIT HERE

      update     = FetchState(&st_fd); 
      cqi_update = FetchCQIs(&cqi_fd);
      if (update.size() > 0 ){
        networkEnv->UpdateNetworkState(update); 
        networkEnv->ProcessCQIs(cqi_update); 
      }
      break; // break out of training loog
    }
  }// training loop

  // log training loop satisfaction rates, false flag signals training
   networkEnv->log_satisfaction_rates(scheduler_string, noUEs, false);
   output_file.close();

  if(use_dqn){ // only testing loops for DQN
    torch::save(policyNet, model_name);
     log_file_name = base + "_testing.txt";
     output_file.open(log_file_name);
    int training_ttis = networkEnv->TTIcounter;

    while(1){ // testing loop
      torch::Tensor state = networkEnv->CurrentState(true); 
      networkEnv->TTI_increment();

    	// select action explore/exploit
      torch::Tensor action = agent->exploit(state.to(device), policyNet, true);

    	// execute action
      SendScheduler(&sh_fd, action[0].item<int>());
      // observe new state
      update     = FetchState(&st_fd); 
      cqi_update = FetchCQIs(&cqi_fd);
      // check if end of simulation
      if(strcmp(update.c_str(), "end") == 0){
        std::cout << "END signal received!" << std::endl;
        break;
      } 
      networkEnv->UpdateNetworkState(update); // process new state
      networkEnv->ProcessCQIs(cqi_update); // process cqis
      torch::Tensor next_state  = networkEnv->CurrentState(false);

      torch::Tensor reward = networkEnv->CalculateReward(); // observe reward
      reward_copy = reward[0].item<float>();

      // by HH: REPLAY, OPTIMIZE
      exp->push(state.to(torch::kCPU), action.to(torch::kCPU), next_state.to(torch::kCPU), reward.to(torch::kCPU)); 
      samples = exp->sampleMemory(BATCH_SIZE); 
      experience batch = processSamples(samples);
      // work out the qs
      current_q_values = agent->CurrentQ(policyNet, std::get<0>(batch), std::get<1>(batch));
      next_q_values = (agent->NextQ(targetNet, std::get<2>(batch))).to(torch::kCPU);
      // bellman equation
      target_q_values = (next_q_values.multiply(GAMMA)) +  std::get<3>(batch);
      // loss and backprop
      torch::Tensor loss = (torch::mse_loss(current_q_values.to(device), target_q_values.to(device))).to(device);
      //printf("loss  %f \n", loss.item().toFloat());
      loss.set_requires_grad(true);
      optimizer.zero_grad();
      loss.backward();
      optimizer.step();


      output_file << networkEnv->TTIcounter << ", " << reward_copy << ", "<< agent->inferenceTime().count() << std::endl;
      if(networkEnv->TTIcounter == (training_ttis + TEST_TTI)) break;
    } // testing loop
    output_file.close();
    networkEnv->log_satisfaction_rates(scheduler_string, noUEs, true);
  } // only dqn should test loop

// send end signal
  SendFinalScheduler(&sh_fd);

  close(sh_fd);
  close(st_fd);
  close(cqi_fd);
  delete networkEnv;
  return 0;
}

void loadStateDict(DQN model, DQN target_model) {
  torch::autograd::GradMode::set_enabled(false);  // make parameters copying possible
  auto new_params = target_model->named_parameters(); // implement this
  auto params = model->named_parameters(true /*recurse*/);
  auto buffers = model->named_buffers(true /*recurse*/);
  for (auto& val : new_params) {
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
  }
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
  torch::Tensor actions_tensor = torch::zeros(0);
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
  h_log("mk CQI FIFO\n");
  mkfifo(CQI_FIFO, S_IFIFO|0777);
  // block for LTESim to connect
  h_log("OPEN rdonly CQI FIFO\n");
  *fd = open(CQI_FIFO, O_CREAT|O_RDONLY);

  h_log("close CQI FIFO\n");
  close(*fd);
}

void OpenStateFifo(int *fd, int *noUEs){
  // create the state fifo
  h_log("mk state FIFO\n");
  mkfifo(STATE_FIFO, S_IFIFO|0777);
  char noUEs_in[80];
  int input_bytes;
  // block for LTESim to connect
  h_log("open O_RDONLY state FIFO\n");
  *fd = open(STATE_FIFO, O_CREAT|O_RDONLY);
  // read the number of UEs
  input_bytes = read(*fd, noUEs_in, sizeof(noUEs_in));
  close(*fd);
  *noUEs = atoi(noUEs_in);
  h_log("close O_RDONLY state FIFO\n");
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
  h_log("open cqi complete\n");
  std::string::size_type size;
  read(*fd, &size, sizeof(size));
  h_log("read cqi complete\n");
  std::string message(size, ' ');
  // read the CQIs
  read(*fd, &message[0], size);
  close(*fd);
  h_log("close cqi complete\n");
  return message;
}

void FetchInitUEs(int *fd, LTENetworkState *network_state){
  // read size of UE summary
  *fd = open(STATE_FIFO, O_RDONLY);
  std::string::size_type size = 0;
  read(*fd, &size, sizeof(size));
  std::string message(size, ' ');
  // read the UE summary
  read(*fd, &message[0], size);
  close(*fd);
  network_state->InitState(message);
}

void SendScheduler(int *fd, int scheduler){
  std::string sched = std::to_string(scheduler);
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
	ConnectSchedulerFifo(_sh_fd);
	// open CQI fifo
  h_log("OPEN CQI FIFO\n");
  OpenCQIFifo(_cqi_fd);

  // open state fifo,connect and fetch #UEs
  int _noUEs = 0;
  h_log("OPEN STATE FIFO\n");
  OpenStateFifo(_st_fd, &_noUEs);
  // initialise the network state environment
  LTENetworkState *networkEnv = new LTENetworkState(_noUEs, CQI_SIZE);
  // initialise UE and Application from LTE-sim
  h_log("FETCH INIT UES\n");
  FetchInitUEs(_st_fd, networkEnv);
  // print a summary
  networkEnv->print_summary();
  return networkEnv;

}
