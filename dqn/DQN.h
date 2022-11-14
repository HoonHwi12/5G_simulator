#ifndef DQN_h
#define DQN_h

#include <torch/torch.h>

extern int BATCH_SIZE;
extern int ADA_ACTIONS;
extern int ADA_ACTION;
extern int NUM_OUTPUT;
extern int NUMUE;

//#define HLOG
#define INF_LOG

#ifdef HLOG
#define h_log(fmt, ...) \
    do { fprintf(stderr, "[hoonhwi] Log: " fmt, ## __VA_ARGS__); } while (0)        
#endif
#ifndef HLOG
#define h_log(fmt, ...) \
    do {  } while (0)        
#endif // HLOG

#ifdef INF_LOG
#define inf_log(fmt, ...) \
    do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)        
#endif
#ifndef INF_LOG
#define inf_log(fmt, ...) \
    do {  } while (0)        
#endif // inf_log



struct DQNImpl : torch::nn::Module {

	 DQNImpl(int64_t state_size, int64_t action_size) 
	 	:conv1(torch::nn::Conv2dOptions(1, 32, 3).stride(1).padding(1)),
		conv2(torch::nn::Conv2dOptions(32, 32, 3).stride(1).padding(1)),
		conv3(torch::nn::Conv2dOptions(64, 64, 3).stride(1)),
		linear1(torch::nn::Linear(200*NUMUE, 512) ),
	 	linear2(torch::nn::Linear(512, ADA_ACTIONS) ),
		state_conv1(torch::nn::Conv2dOptions(1, 32, 3).stride(1).padding(1)),
		state_conv2(torch::nn::Conv2dOptions(32, 32, 3).stride(1).padding(1)),
		state_conv3(torch::nn::Conv2dOptions(64, 64, 3).stride(1)),
		state_linear1(torch::nn::Linear(200*NUMUE, 512) ),
	 	state_linear2(torch::nn::Linear(512, ADA_ACTIONS) ) {
	 		register_module("conv1", conv1);
			register_module("conv2", conv2);
			register_module("conv3", conv3);
			register_module("linear1", linear1);
	 		register_module("linear2", linear2);
			register_module("state_conv1", state_conv1);
			register_module("state_conv2", state_conv2);
			register_module("state_conv3", state_conv3);
			register_module("state_linear1", state_linear1);
			register_module("state_linear2", state_linear2);
	 	}


	torch::Tensor state_forward(torch::Tensor input){
		// torch::Tensor h1 = linear1(input);
		// torch::Tensor h2 = torch::tanh(h1);
		input = input.unsqueeze(0);
		h_log("state forward before conv1\n");
		//input.print();
		torch::Tensor h1 = state_conv1(input);
		h_log("after conv1, h1:\n");
		//h1.print();
		h1 = torch::relu(h1);

		torch::Tensor h2 = state_conv2(h1);
		h_log("after conv2, h2:\n");
		//h2.print();
		h2 = torch::relu(h2);

		//h2 = state_conv3(h2);
		//h_log("after conv3, h2:\n");
		//h2.print();
		//h2 = torch::relu(h2);

		h2 = h2.view({NUM_OUTPUT, -1});
		h_log("after VIEW h2:\n");
		//h2.print();

		h2 = state_linear1(h2);
		h_log("after linear1, h2:\n");
		//h2.print();
		h2 = torch::relu(h2);

		h2 = state_linear2(h2);
		h_log("after linear2, h2:\n");
		//h2.print();

		return h2;
	}

	torch::Tensor forward(torch::Tensor input){
		// torch::Tensor h1 = linear1(input);
		// torch::Tensor h2 = torch::tanh(h1);
		input = input.unsqueeze(1);
		h_log("before conv1 *******************\n");
		//input.print();
		torch::Tensor h1 = conv1(input);
		h_log("after conv1, h1:\n");
		//h1.print();
		h1 = torch::relu(h1);

		torch::Tensor h2 = conv2(h1);
		h_log("after conv2, h2:\n");
		//h2.print();
		h2 = torch::relu(h2);

		//h2 = conv3(h2);
		//h_log("after conv3, h2:\n");
		//h2.print();
		//h2 = torch::relu(h2);


		h2 = h2.view({BATCH_SIZE, NUM_OUTPUT, -1});
		h_log("after VIEW h2:\n");
		//h2.print();

		torch::Tensor h3 = linear1(h2);
		h_log("after linear1, h3:\n");
		//h3.print();
		h3 = torch::relu(h3);

		h3 = linear2(h3);
		h_log("after linear2, h3:\n");
		//h3.print();

		return h3;
	}

	torch::nn::Linear linear1, linear2, state_linear1, state_linear2;
	torch::nn::Conv2d state_conv1, state_conv2, state_conv3;
	torch::nn::Conv2d conv1, conv2, conv3;
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