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
#include <pthread.h>

#include "cmath"
#include "libtorch/include/ATen/ops/isfinite.h"
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
extern const int UPDATE_FREQUENCY;
int BATCH_SIZE              = 32;
int TRAIN_START             = 6000;
int TRAIN_TTI               = 50000;
const int TEST_TTI          = 0;
const int MIN_REPLAY_MEM    = 6000; //3000
//const int UPDATE_FREQUENCY  = 1;
const float GAMMA           = 0.99;  // discount factor for bellman equation
const float EPS_START       = 1.0;    // greedy stuff
const float EPS_END         = 0.01;
const float EPS_DECAY       = 0.0001; // exploration rate, small->slower

const int NET_UPDATE        = 1000;     // how many episodes until we update the target DQN, ACTUALLY=>NET_UPDATExUPDATE_FREQUENCY
const int MEM_SIZE          = 10000; // replay memory size
const float LR_START        = 0.01;
const float LR_END          = 0.00001;
const float LR_DECAY        = 0.00001; //0.001
const float MOMENTUM        = 0.05;  // SGD MOMENTUM
 // environment concerns
const int NUM_ACTIONS       = 11;    // number of actions (0~10 = 11)
const int CQI_SIZE          = 25;   // number of downlink channels per eNB

//Adaptive DQN
//const int ADA_ACTIONS       = 17569; // 14641 + 2928
//* hyunji
int ADA_ACTIONS       = 11;
int ADA_ACTION        = 44;
int NUM_OUTPUT        = 4;

//* HH: for dqn network size
int NUMUE             = 0;

// training times
std::chrono::steady_clock::time_point start;
std::chrono::steady_clock::time_point end;
std::chrono::nanoseconds duration;

// clock_t function_clock = clock();
// void *thread_function_clock(void* a)
// {
//   clock_t thread_clock = clock();
//   while(1)
//   {
//     if( (thread_clock > function_clock)
//         && (float)(thread_clock - function_clock)/CLOCKS_PER_SEC >60 )
//     {
//       printf("timeout (%f) (%f)\n", thread_clock/CLOCKS_PER_SEC, function_clock/CLOCKS_PER_SEC);
//       sleep(10000);
//       break;
//     }
//     else
//     {
//       printf("not timeout (%f) (%f)\n", thread_clock/CLOCKS_PER_SEC, function_clock/CLOCKS_PER_SEC);
//       thread_clock = clock();
//       sleep(1);
//     }
//   }
// }

int main(int argc, char** argv) {
  //main_start:
  //pipe_bug = false;
  
	int constant_scheduler = 0;
  bool use_dqn = false;
  bool model_saved = false;

  //printf("batch(%d)/minReplay(%d)/EPS(%0.2f~%0.2f/%0.4f)/Update(%d)\n",
//    BATCH_SIZE, MIN_REPLAY_MEM, EPS_START, EPS_END, EPS_DECAY, NET_UPDATE);
  // file naming
  std::string scheduler_string = "dqn";
  std::string model_number = "0";
  bool is_load=false;

  // by HH test time
  clock_t test_start = clock();
  // pthread_t thread_id1;
  // if(pthread_create(&thread_id1, NULL, thread_function_clock, NULL) != 0)
  // {
  //   printf("thread create error!\n");
  // }
  
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
  if(argc >= 3) model_number = argv[2]; // default="0"
  if(argc >= 4)
  {
    if( strcmp(argv[3], "0")==0 ) is_load=false;          // default=false
    else is_load=true;
  }  
  if(argc == 5)
  {
    if( strcmp(argv[4], "0")==0) use_lstm=false;          // default=false
    else use_lstm=true;
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


  //std::cout << "PYTORCH version " << TORCH_VERSION << std::endl;

	// Decide CPU or GPU
	torch::Device device = torch::kCPU;
	//std::cout << "CUDA DEVICE COUNT: " << torch::cuda::device_count() << std::endl;
	if (torch::cuda::is_available()) {
    	//std::cout << "CUDA available - working on GPU." << std::endl;
    	device = torch::kCUDA;
  }
  // pipe 
  int sh_fd, st_fd, cqi_fd;
  // initial connections
  LTENetworkState* networkEnv = initConnections(&sh_fd, &st_fd, &cqi_fd);

  // if(pipe_bug == true)
  // {
  //   remove(STATE_FIFO);
  //   remove(CQI_FIFO);
  //   remove(SCHED_FIFO);

  //   // LTE-SIM에 PIPE BUG 발생 알림
  //   sprintf(dqn_buffer,"%d", 0x71);
  //   printf("pipe bug detected %s\n", dqn_buffer);

  //   if( SharedMemoryWrite(dqn_shmid, dqn_buffer) == -1)
  //   {
  //     printf("shared memory write failed\n");
  //   }

  //   goto main_start;
  // }
  int noUEs = networkEnv->noUEs;

  // simulation output traces
  std::string update;
  std::string cqi_update;

   // DQN components
  torch::Tensor reset_state        =   networkEnv->reset().to(device);
  torch::Tensor state;
  //ReplayMemory* exp                = new ReplayMemory(MEM_SIZE);
  //tuple_list tl;

  EpsilonGreedy* eps               = new EpsilonGreedy(EPS_START, EPS_END, EPS_DECAY);// start, end, decay
  EpsilonGreedy* lr_rate           = new EpsilonGreedy(LR_START, LR_END, LR_DECAY);

  //* hyunji
  //torch::Tensor ada_actions = torch::zeros({NUM_OUTPUT,ADA_ACTIONS});
  Agent<EpsilonGreedy, DQN>* agent = new Agent<EpsilonGreedy, DQN>(eps,NUM_ACTIONS);
  DQN policyNet(reset_state.size(1), NUM_ACTIONS, BATCH_SIZE);
  DQN targetNet(reset_state.size(1), NUM_ACTIONS, BATCH_SIZE);

  // logging files for training 
  //  ~ please make sure that test_results/ is valid folder
  std::string log_file_name, model_name, base;
  base               = "test_results/" + scheduler_string + std::to_string(noUEs);
  log_file_name      = base + "_training.txt";
  model_name         = base + "_" + model_number + "_model.pt";
  h_log("File name ready\n");
  #ifdef HLOG
  std::cout << "base_name(" << base << ")"<<std::endl;
  std::cout << "model_name(" << model_name << ")" <<std::endl;
  #endif

  // by HH LOAD MODEL
  if(is_load)
  {
    //printf("model [%s] loading...", model_name.c_str());
    torch::load(policyNet, model_name);
    //printf("success\n");
    //sleep(3);
  }
  else{
    policyNet->apply(initWeights);
  }

  // copy weights to targetnet
  loadStateDict(policyNet, targetNet);
  

  policyNet->to(device);
  targetNet->to(device);

  // setting up training variables
  //std::vector<experience> samples;
  tuple_list experience_memory[MEM_SIZE];
  sample_list samples;


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
    //TRAIN_TTI += TEST_TTI;
  }

  torch::Tensor update_state = torch::ones({networkEnv->noUEs, networkEnv->state_size, UPDATE_FREQUENCY});
  torch::Tensor before_state = torch::ones({networkEnv->noUEs, networkEnv->state_size, UPDATE_FREQUENCY});
  torch::Tensor next_state = torch::ones({networkEnv->noUEs, networkEnv->state_size, UPDATE_FREQUENCY});

  torch::Tensor exp_current_state[MEM_SIZE];
  torch::Tensor exp_next_state[MEM_SIZE];
  torch::Tensor exp_reward[MEM_SIZE];
  torch::Tensor exp_action_input[MEM_SIZE];

  h_log("2222\n");
  // selecting an action
  torch::Tensor action = torch::zeros({2,4});
  torch::Tensor action_input = torch::zeros({4});

  while(1){ // training loop
    h_log("entering while(1)\n");

    clock_t infstart=clock();

    #ifdef TTI_LOG
    if( (networkEnv->TTIcounter >= TRAIN_START) ) printf("%d ", networkEnv->TTIcounter);
    #endif

    torch::Tensor state = networkEnv->CurrentState(true);
    for (int ue_index=0 ; ue_index<networkEnv->noUEs; ue_index++)
    {
      for (int ch_index=0 ; ch_index<networkEnv->state_size; ch_index++)
      {
        update_state.index_put_({ue_index, ch_index, (networkEnv->TTIcounter) % UPDATE_FREQUENCY}, state[ue_index][ch_index].clone());
      }
    }    
  	networkEnv->TTI_increment();

    h_log("action ready\n");

    if(use_dqn && networkEnv->TTIcounter < TRAIN_START)
    {
      //action = agent->explore(state.to(device));
      action.index_put_({0,0}, 0);
      action.index_put_({0,1}, 0);
      action.index_put_({0,2}, 0);
      action.index_put_({0,3}, 10);
      action.index_put_({1}, 0);      
      
      SendScheduler(&sh_fd, action[0][0].item<int>(), action[0][1].item<int>(), action[0][2].item<int>(), action[0][3].item<int>());

      // observe new state
      update     = FetchState(&st_fd); 
      cqi_update = FetchCQIs(&cqi_fd);
      if(strcmp(update.c_str(), "end") == 0){ // check if end of simulation
        std::cout << "END signal received!" << std::endl;
        break;
      }

      networkEnv->UpdateNetworkState(update); 
      networkEnv->ProcessCQIs(cqi_update);

      // state = networkEnv->CurrentState(false);
      // for (int ue_index=0 ; ue_index<networkEnv->noUEs; ue_index++)
      // {
      //   for (int ch_index=0 ; ch_index<networkEnv->state_size; ch_index++)
      //   {
      //     update_state.index_put_({ue_index, ch_index, (networkEnv->TTIcounter) % UPDATE_FREQUENCY}, state[ue_index][ch_index]);
      //   }
      // }   

      if((networkEnv->TTIcounter) % UPDATE_FREQUENCY== 0) //* hh: update_state is full, update state to next state
      {
        before_state = next_state.clone(); //* hh: next state to before state
        next_state = update_state.clone();
        next_state = next_state.transpose(2,0);
      }
    }
    else
    {
      if(use_dqn) // select action explore/exploit in dqn
      {
        if(networkEnv->TTIcounter > TRAIN_TTI && (networkEnv->TTIcounter) % UPDATE_FREQUENCY==0)
        {
          action = agent->exploit(before_state.to(device), policyNet, true);
        }
        else if((networkEnv->TTIcounter) % UPDATE_FREQUENCY==0)
        {
          action = agent->selectAction(before_state.to(device), policyNet);
          h_log("selectAction finish\n");

          if(use_dqn && action[0][0].item<int>() >= 0)
          {
            action_input[0] = action[0][0];
            action_input[1] = action[0][1];
            action_input[2] = action[0][2];
            action_input[3] = action[0][3];
          }
          else
          {
            h_log("action_input index_put\n");
            action_input.index_put_({0}, action[0][2].item<int>()); // fls
            h_log("action_input index_put finish\n");
          }
        }

        // * hperf weight logging
        #ifdef WEIGHT_LOG
        printf("%d %d %d %d ", action[0][0].item<int>(), action[0][1].item<int>(), action[0][2].item<int>(), action[0][3].item<int>());
        #endif
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

      #ifdef TIME_LOG
      clock_t t_select_action=clock();
      printf("sendAction(%0.4f ms) Exploit(%d)Explore(%d) ", (float)(t_select_action-infstart)/CLOCKS_PER_SEC*1000, valid_TTI_exploit, valid_TTI_explore);
      #endif

      // observe new state
      update     = FetchState(&st_fd); 
      cqi_update = FetchCQIs(&cqi_fd);
      if(strcmp(update.c_str(), "end") == 0){ // check if end of simulation
        std::cout << "END signal received!" << std::endl;
        break;
      }
      
      // process new state + cqis
      networkEnv->UpdateNetworkState(update); 
      networkEnv->ProcessCQIs(cqi_update);
      
      // next state and reward
      torch::Tensor reward = networkEnv->CalculateReward(); 
      reward_copy = reward[0].item<float>();

      // * hperf reward log
      #ifdef REWARD_LOG
      if(networkEnv->TTIcounter > TRAIN_START) printf("%f ", reward[0].item<float>());
      #endif

      
      if( (networkEnv->TTIcounter < TRAIN_TTI) && use_dqn)
      {
        // state = networkEnv->CurrentState(false);
        // for (int ue_index=0 ; ue_index<networkEnv->noUEs; ue_index++)
        // {
        //   for (int ch_index=0 ; ch_index<networkEnv->state_size; ch_index++)
        //   {
        //     update_state.index_put_({ue_index, ch_index, (networkEnv->TTIcounter) % UPDATE_FREQUENCY}, state[ue_index][ch_index]);
        //   }
        // }
        
        if((networkEnv->TTIcounter) % UPDATE_FREQUENCY== 0) //* hh: update_state is full, training start
        {
          before_state = next_state.clone();
          next_state = update_state.clone();
          next_state = next_state.transpose(2,0);

          #ifdef NET_LOG
          n_log("next state: ");
          next_state.print();
          // n_log("state: ");
          // state.print();
          // n_log("action_input: ");
          // action_input.print();
          #endif

          //next_state = next_state.unsqueeze(0);
          //state = state.unsqueeze(0);
          //action_input = action_input.unsqueeze(0);

          start = std::chrono::steady_clock::now(); //training time logging

          //printf("%d %d %d %d\n", action[0][0].item<int>(), action[0][1].item<int>(), action[0][2].item<int>(), action[0][3].item<int>());

          n_log("allocate exp memory\n");
          n_log("mem_iter(%d) UPDATE_FREQUENCY(%d) mem_capacity(%d)\n", mem_iter, UPDATE_FREQUENCY, mem_capacity);
          // store experiece in replay memory
          if( mem_iter >= MEM_SIZE) mem_iter = 0;

          //experience_memory[mem_iter].push_front(std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor>(state, action_input.to(torch::kCPU), next_state, reward));
          //exp_current_state[mem_iter] = state.clone();
          exp_current_state[mem_iter] = before_state.clone();
          exp_next_state[mem_iter] = next_state.clone();
          exp_reward[mem_iter] = reward.clone();
          exp_action_input[mem_iter] = action_input.clone();
          experience_memory[mem_iter] = std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor>(exp_current_state[mem_iter], exp_action_input[mem_iter], exp_next_state[mem_iter], exp_reward[mem_iter]);

          if(mem_capacity < MEM_SIZE) mem_capacity++;
          mem_iter++;

          n_log("allocate exp memory finished\n");

          #ifdef NET_LOG
          std::cout << "tl0" << std::get<1>(experience_memory[0]);
          if(mem_capacity>1) std::cout << "tl1" << std::get<1>(experience_memory[1]);
          if(mem_capacity>2) std::cout << "tl2" << std::get<1>(experience_memory[2]);
          if(mem_capacity>3) std::cout << "tl3" << std::get<1>(experience_memory[3]);
          #endif
          
          //exp->push(state, action_input.to(torch::kCPU), next_state, reward);
          

          //action_input = action_input.squeeze(0);

          #ifdef TIME_LOG
          clock_t t_save_exp=clock();
          printf("saveEXP(%0.4f ms) ", (float)(t_save_exp-t_select_action)/CLOCKS_PER_SEC*1000);
          #endif

          // if enough samples
          if(canProvideSamples() )
          { 
            update_counter++;
            // access learning rate
            auto options = static_cast<torch::optim::AdamOptions&> (optimizer.defaults());
            options.lr(lr_rate->explorationRate(networkEnv->TTIcounter - MIN_REPLAY_MEM));
            h_log("lr update\n");

            //sample random batch and process
            //samples = exp->sampleMemory(BATCH_SIZE); 
            samples = sampleMemory(experience_memory);

            //   printf("samples:\n");            
            //   for (auto i : samples){
            //     printf("get0: ");
            //     std::get<0>(i).print();
            //     printf("get1: ");
            //     std::get<1>(i).print();
            //     printf("get2: ");
            //     std::get<2>(i).print();
            //     printf("get3: ");
            //     std::get<3>(i).print();
            //   }

            h_log("sample memory\n");
            experience batch = processSamples(samples);
            h_log("process samples\n");
            torch::Tensor current_q_index = std::get<1>(batch).to(device); // size: 40 32x4

            // #ifdef NET_LOG
            // n_log("current_q_index: ");
            // current_q_index.print();
            // std::cout << current_q_index << std::endl;
            // #endif
            //current_q_index = current_q_index.reshape({-1, NUM_OUTPUT}); // size: 32x4
            current_q_index = current_q_index.unsqueeze(2);
            current_q_index = current_q_index.toType(torch::kInt64);
            //current_q_index.print();
            //std::cout <<"current_q_index ***********************\n" << current_q_index<<std::endl;

            h_log("get current q start\n");
            current_q_values = agent->CurrentQ(policyNet, std::get<0>(batch)).to(device); // size: 32x4x11

            #ifdef TIME_LOG
            clock_t t_forward_current=clock();
            printf("forward_currentQ(%0.4f ms) ", (float)(t_forward_current-t_save_exp)/CLOCKS_PER_SEC*1000);
            #endif

            //current_q_values = torch::nan_to_num(current_q_values);
            h_log("get current q end\n");
            //current_q_values = current_q_values.reshape({-1,4,11}); // size: 32x4x11
            //current_q_values.print();
            //std::cout <<"current Q before gather***********************\n" << current_q_values <<std::endl;

            //current_q_index = current_q_index.cuda();
            //current_q_values = current_q_values.cuda();

            #ifdef NET_LOG
            printf("current_q_index: ");
            current_q_index.print();
            #endif

            current_q_values = current_q_values.gather(2, current_q_index); // size: 4x11
            torch::Tensor currentQ = current_q_values.sum().to(device);
            q_log("currentQ(%f) ", currentQ.item<float>());
            #ifdef NET_LOG
            printf("current_q_values: ");
            current_q_values.print();
            #endif
            //std::cout <<"current Q after gather***********************\n" << current_q_values<<std::endl;
            //current_q_values = current_q_values.reshape({-1, NUM_OUTPUT});

            h_log("currenQ ready\n");
            next_q_values = (agent->NextQ(targetNet, std::get<2>(batch))).to(device);

            #ifdef TIME_LOG
            clock_t t_forward_next=clock();
            printf("forward_nextQ(%0.4f ms) ", (float)(t_forward_next-t_forward_current)/CLOCKS_PER_SEC*1000);
            #endif

            
            h_log("next Q index\n");
            torch::Tensor next_q_index = at::argmax(next_q_values,2);
            next_q_index = next_q_index.unsqueeze(2);
            
            h_log("next q print\n");
            next_q_values = next_q_values.gather(2, next_q_index);
            torch::Tensor nextQ = next_q_values.sum().to(device);
            q_log("nextQ(%f) ", nextQ.item<float>());
            #ifdef NET_LOG
            printf("next_q_values: ");
            next_q_values.print();
            #endif
            //next_q_values.print();
            //std::cout <<"next_q_values ***********************\n" << next_q_values<<std::endl;

            // bellman equation
            h_log("debug 06\n");
            //next_q_values = next_q_values.reshape({NUM_OUTPUT, -1});
            torch::Tensor batch_reward = std::get<3>(batch).sum().to(device);
            q_log("reward(%f) ", batch_reward.item<float>());
            h_log("debug 066\n");

            #ifdef NET_LOG
            printf("batch_reward: ");
            batch_reward.print();
            #endif

            //batch_reward = batch_reward.unsqueeze(1);
            //batch_reward = batch_reward.unsqueeze(2);
            //batch_reward = torch::nan_to_num(batch_reward);

            target_q_values = ((nextQ.multiply(GAMMA)) + batch_reward).to(device);
            q_log("targetQ(%f)\n", target_q_values.item<float>());
            //target_q_values = torch::nan_to_num(target_q_values);

            h_log("debug 07\n");
            //target_q_values = target_q_values.reshape({-1, NUM_OUTPUT});
            //target_q_values.print();

            h_log("debug 0605\n");
            // loss and backprop
            torch::Tensor loss = (torch::mse_loss(currentQ, target_q_values)).to(device);

            //* hperf loss log
            #ifdef LOSS_LOG
            if(networkEnv->TTIcounter > TRAIN_START) printf("%f ", loss.item<float>());
            #endif

            loss.set_requires_grad(true);
            h_log("debug 0608\n");
            optimizer.zero_grad();
            h_log("debug 0606\n");
            loss.backward();

            #ifdef TIME_LOG
            clock_t t_backward=clock();
            printf("backward(%0.4f ms) ", (float)(t_backward-t_forward_next)/CLOCKS_PER_SEC*1000);
            #endif

            h_log("debug 0607\n");
            optimizer.step();
            h_log("optimizer step complete\n");

            // update targetNet with policyNey parameters
            if(update_counter > NET_UPDATE){
              // copy weights to targetnet
              loadStateDict(policyNet, targetNet);
              update_counter = 0;
            }
          } // enough samples
        } // every UPDATE_FREQUENCY

        // training time logging
        end = std::chrono::steady_clock::now();
        duration = end - start;

        // checks the valid-TTI's explore/exploitation count
        if (action[1][0].item<int>() == 0) valid_TTI_exploit++;
        else if(action[1][0].item<int>() > 0) valid_TTI_explore++;
        //printf("\tInferenceTime %0.7f ms\tExploit %d,\tExplore %d\n", (float)(clock()-infstart)/CLOCKS_PER_SEC, valid_TTI_exploit, valid_TTI_explore);
        q_log("Exploit(%d)Explore(%d) ", valid_TTI_exploit, valid_TTI_explore);

      } // training loop
    }

    //* hperf time log
    #ifdef TIME_LOG
    if(networkEnv->TTIcounter >= TRAIN_START) printf("LOOP(%0.4f ms) ", (float)(clock()-infstart)/CLOCKS_PER_SEC*1000);
    #endif    
    
    if( model_saved == false && (networkEnv->TTIcounter >= TRAIN_TTI ))
    {
      torch::NoGradGuard no_grad;
      model_saved = true;
      torch::save(policyNet, model_name);
    }

    if(networkEnv->TTIcounter >= (TRAIN_TTI + TEST_TTI))
    {
      break;
    }
    if(networkEnv->TTIcounter > TRAIN_START) printf("\n");
  }// training loop

  // log training loop satisfaction rates, false flag signals training
  networkEnv->log_satisfaction_rates(scheduler_string, noUEs, false);

  // send end signal
  SendFinalScheduler(&sh_fd);

  close(sh_fd);
  close(st_fd);
  close(cqi_fd);
  delete networkEnv;

  //printf("Average GBR: %0.6f, Average delay: %0.6f, Average plr: %0.6f, fairness: %0.6f, throughput: %0.6f, goodput: %0.6f\n",
  //  sum_gbr/networkEnv->TTIcounter, sum_delay/networkEnv->TTIcounter, sum_plr/networkEnv->TTIcounter, jfi, sum_throughput/throughput_num, sum_goodput/goodput_num);
  //printf("TEST END, Test Duration: %0.4f s\n", (float)(clock()-test_start) / CLOCKS_PER_SEC);

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

  #ifdef NET_LOG
  int index=0;
  printf("samples:\n");            
  for (auto i : _samples){
    printf("index: %d\n", index++);
    printf("get0: ");
    std::get<0>(i).print();
    printf("get1: ");
    std::get<1>(i).print();
    printf("get2: ");
    std::get<2>(i).print();
    printf("get3: ");
    std::get<3>(i).print();
  }  
  #endif

  torch::Tensor states_tensor;
  torch::Tensor actions_tensor;
  torch::Tensor new_states_tensor;
  torch::Tensor rewards_tensor;

  states_tensor = torch::stack(states, 0);
  actions_tensor = torch::stack(actions, 0);
  new_states_tensor = torch::stack(new_states, 0);
  rewards_tensor = torch::stack(rewards, 0);

  experience _batch(states_tensor , actions_tensor, new_states_tensor , rewards_tensor);
  return _batch;
}

void ConnectSchedulerFifo(int *fd){
  // connect to scheduler fifo
  *fd = open(SCHED_FIFO, O_WRONLY);
  unlockpt(*fd);
  close(*fd);
}

void OpenCQIFifo(int *fd){
  // create the cqi fifo
  //printf("mk cqi fifo\n");
  mkfifo(CQI_FIFO, S_IFIFO|0777);
  // block for LTESim to connect
  *fd = open(CQI_FIFO, O_RDONLY);
  //printf("close cqi fifo\n");
  unlockpt(*fd);
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
  unlockpt(*fd);
  close(*fd);
  *noUEs = atoi(noUEs_in);
}

std::string FetchState(int *fd){
  // read state size
  h_log("open state fifo\n");
  *fd = open(STATE_FIFO, O_RDONLY);
  std::string::size_type size;
  h_log("read state fifo\n");
  read(*fd, &size, sizeof(size));
  // read state/update from LTEsim
  std::string message(size, ' ');
  h_log("read state message\n");
  read(*fd, &message[0], size);

  h_log("close state message\n");
  unlockpt(*fd);
  close(*fd);
  return message;
}

std::string FetchCQIs(int *fd){
  // read CQI message size
  h_log("open fetch cqi fifo\n");
  *fd = open(CQI_FIFO, O_RDONLY);
  std::string::size_type size;
  h_log("read fetch cqi fifo\n");
  read(*fd, &size, sizeof(size));
  std::string message(size, ' ');
  // read the CQIs
  read(*fd, &message[0], size);
  close(*fd);
  h_log("close fetch cqi fifo\n");

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
  NUMUE = networkEnv->noUEs;
  // initialise UE and Application from LTE-sim
  h_log("fetch init ues\n");
  FetchInitUEs(_st_fd, networkEnv);
  // print a summary
  networkEnv->print_summary();

  h_log("init connections end\n");
  return networkEnv;

}
