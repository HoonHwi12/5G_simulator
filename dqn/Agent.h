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
			std::cout << "Agent Ready ~" << std::endl;
		}
		~Agent();

		torch::Tensor explore(torch::Tensor state){
			h_log("debug 410\n");
			//std::uniform_int_distribution<int> dist_actions(0, num_actions);
			//std::uniform_int_distribution<int> dist_actions(0, 4);
			torch::Tensor explore_action = torch::zeros({2, 4});
			//uint32_t random_action = dist_actions(mt);

			torch::Tensor random_action = torch::zeros(4);

			for(int i = 0 ; i < 4; i++){
				std::uniform_int_distribution<int> dist_actions(0, num_actions);
				random_action[i] = dist_actions(mt);
			}

			printf("Explore action: %d, %d, %d, %d\n",random_action[0].item<int>(),
													random_action[1].item<int>(),
													random_action[2].item<int>(),
													random_action[3].item<int>());
													

			//* by HH: FLS dummy code
			// if(random_action > 14641)
			// {
			// 	explore_action.index_put_({0, 0}, -1);
			// 	explore_action.index_put_({0, 1}, 3);
			// 	explore_action.index_put_({0, 2}, (int)random_action);
			// 	explore_action.index_put_({0, 3}, -1);
			// 	explore_action.index_put_({1}, 1);
			// }
			// else
			{
				explore_action.index_put_({0, 0}, random_action[0]);
				explore_action.index_put_({0, 1}, random_action[1]);
				explore_action.index_put_({0, 2}, random_action[2]);
				explore_action.index_put_({0, 3}, random_action[3]);
				explore_action.index_put_({1}, 1);
			}

			start, end = std::chrono::steady_clock::now(); // time of zero is explore
			return explore_action;
		}


		torch::Tensor exploit(torch::Tensor state, R policy_net, bool timeLog){
			h_log("debug400\n");
			torch::NoGradGuard no_grad;

			start = std::chrono::steady_clock::now();
			clock_t infstart = clock();

			torch::Tensor output = policy_net->state_forward(state);	

			//std::cout <<"output_before:" << output<<std::endl;
			output = output.reshape({4,11});		
			//std::cout <<"reshape output: " <<output<<std::endl;		
			h_log("debug401\n");
			torch::Tensor exploit_action = torch::zeros({2, 4});
			//uint32_t arg_action = at::argmax(output, 1).item<int>();
			
			torch::Tensor arg_action =  at::argmax(output,1);

			//std::cout <<"arg_action: "<< arg_action <<std::endl;
			
			end = std::chrono::steady_clock::now();
			h_log("debug402\n");
			//if(timeLog) printf("InferenceTime %0.7f ms/ Exploit! \n", (float)(clock()-infstart)/CLOCKS_PER_SEC);

			//* by HH: FLS dummy code
			// if(arg_action > 14641)
			// {
			// 	exploit_action.index_put_({0, 0}, -1);
			// 	exploit_action.index_put_({0, 1}, 3);
			// 	exploit_action.index_put_({0, 2}, (int)arg_action);
			// 	exploit_action.index_put_({0, 3}, -1);
			// 	exploit_action.index_put_({1}, 1);
			// }
			// else
			{
				exploit_action.index_put_({0, 0}, arg_action[0]);
				exploit_action.index_put_({0, 1}, arg_action[1]);
				exploit_action.index_put_({0, 2}, arg_action[2]);
				exploit_action.index_put_({0, 3}, arg_action[3]);
				exploit_action.index_put_({1}, 0);
			}

			printf("exploit action %d %d %d %d\n", exploit_action[0][0].item<int>(), exploit_action[0][1].item<int>(), exploit_action[0][2].item<int>(), exploit_action[0][3].item<int>());
		
			if(!timeLog) printf("Exploit! \n");
			
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