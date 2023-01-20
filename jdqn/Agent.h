#ifndef Agent_h
#define Agent_h

#include <random>
#include <chrono>
#include <torch/torch.h>
#include "DQN.h"

template <typename T, typename R>
class Agent{
	private:		
		int num_actions;
		torch::DeviceType device;
		// seed the RNG
        std::random_device rd;
        std::mt19937 mt;
        // inference time
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point end;
	public:
		int current_step;
		T* strategy;
		Agent(T* strategy_, int num_actions_){
			strategy     = strategy_;
			num_actions  = num_actions_ - 1; // include 0
			current_step = 0;
			device = (torch::cuda::is_available() ? torch::kCUDA : torch::kCPU);
			//std::cout << "Agent Ready ~" << std::endl;
		}
		~Agent();

		torch::Tensor explore(torch::Tensor state){
			h_log("debug 410\n");
			torch::Tensor explore_action = torch::zeros({2, 1});

			torch::Tensor random_action = torch::zeros(1);

			for(int i = 0 ; i < 1; i++){
				std::uniform_int_distribution<int> dist_actions(0, num_actions);
				random_action[i] = dist_actions(mt);
			}

			//printf("Explore action: %d\n",random_action[0].item<int>());

			explore_action.index_put_({0, 0}, random_action[0]);
			explore_action.index_put_({1}, 1);

			return explore_action;
		}


		torch::Tensor exploit(torch::Tensor state, R policy_net, bool timeLog){
			h_log("debug400\n");
			torch::NoGradGuard no_grad;

			torch::Tensor output = policy_net->state_forward(state);	
	
			h_log("debug401\n");
			torch::Tensor exploit_action = torch::zeros({2, 1});
			
			torch::Tensor arg_action =  at::argmax(output,1);
			
			end = std::chrono::steady_clock::now();
			h_log("debug402\n");

			exploit_action.index_put_({0, 0}, arg_action[0]);

			//printf("exploit action %d\n", exploit_action[0][0].item<int>());
			
			return exploit_action;
		}



		torch::Tensor selectAction(torch::Tensor state, R policy_net){
			float exploration_rate = strategy->explorationRate(current_step);
			// printf("eps: %f\n", exploration_rate);
			current_step++;
			std::uniform_int_distribution<int> dist_actions(0, num_actions);
	        std::uniform_real_distribution<double> dist_rate(0.0, 1.0);
	        mt.seed(rd());
			float random_rate = dist_rate(mt);
			// printf("rdr: %f\n", random_rate)
			if(exploration_rate > random_rate){ // explore
				return explore(state);
			} else { 				
				return exploit(state, policy_net, false);
			}
		}


		torch::Tensor CurrentQ(R policy_net, torch::Tensor states){
			torch::Tensor q_values = policy_net->forward(states.cuda());
			//std::cout<<"state:\n"<<states<<std::endl;

			return q_values;

			//torch::Tensor max_qs   = std::get<0>(q_values.max(1));
			//return max_qs;
		}

		torch::Tensor NextQ(R target_net, torch::Tensor next_states){
			torch::Tensor q_values = target_net->forward(next_states.cuda());
			return q_values;
		}

		std::chrono::nanoseconds inferenceTime(){
			std::chrono::nanoseconds nano = end - start;
			return nano;
		}





	
};

#endif