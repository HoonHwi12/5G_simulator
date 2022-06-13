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
	 	linear2(torch::nn::Linear(80, action_size) ){

		// 	 		conv1(torch::nn::Conv2dOptions(3, 2, 3).stride(1).bias(false)),
		// conv2(torch::nn::Conv2dOptions(3, 2, 3).stride(1).bias(false)) 
	 		register_module("linear1", linear1);
	 		register_module("linear2", linear2);
			// register_module("conv1", conv1);
			// register_module("conv2", conv2);
	 	}


	torch::Tensor forward(torch::Tensor input){
		torch::Tensor h1 = linear1(input);
		torch::Tensor h2 = torch::tanh(h1);
		// torch::Tensor h1 = torch::relu(input);
		// h1 = conv1(h1);
		// torch::Tensor h2 = torch::relu(conv2(input));
		// torch::Tensor h2 = torch::tanh(h1);

		return linear2(h2);
	}

	torch::nn::Linear linear1, linear2;
	//torch::nn::Conv2dOptions conv1, conv2;
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