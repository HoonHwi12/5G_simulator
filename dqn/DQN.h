#ifndef DQN_h
#define DQN_h

#include <torch/torch.h>

//#define HLOG
#ifdef HLOG
#define h_log(fmt, ...) \
    do { fprintf(stderr, "[hoonhwi] Log: " fmt, ## __VA_ARGS__); } while (0)        
#endif // HLOG

#ifndef HLOG
#define h_log(fmt, ...) \
    do {  } while (0)        
#endif // HLOG

struct DQNImpl : torch::nn::Module {

	 DQNImpl(int64_t state_size, int64_t action_size) 
	 	:linear1(torch::nn::Linear(state_size,80) ),
	 	linear2(torch::nn::Linear(80, action_size) ) {
	 		register_module("linear1", linear1);
	 		register_module("linear2", linear2);
	 	}


	torch::Tensor forward(torch::Tensor input){
		torch::Tensor h1 = linear1(input);
		torch::Tensor h2 = torch::tanh(h1);
		return linear2(h2);
	}

	torch::nn::Linear linear1, linear2;
};

#endif
// model 1
// #include <torch/torch.h>

// struct DQNImpl : torch::nn::Module {

// 	DQNImpl(int64_t state_size, int64_t action_size) 
// 		: linear1(torch::nn::Linear(state_size,80) ),
// 		linear2(torch::nn::Linear(80, action_size) ) {
// 			register_module("linear1", linear1);
// 			register_module("linear2", linear2);
// 		}

// 	torch::Tensor forward(torch::Tensor input){
// 		torch::Tensor h1 = linear1(input);
// 		torch::Tensor h2 = torch::tanh(h1);
// 		return linear2(h2);
// 	}

// 	torch::nn::Linear linear1, linear2;
// };