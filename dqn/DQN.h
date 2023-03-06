#ifndef DQN_h
#define DQN_h

#include <torch/torch.h>

extern int BATCH_SIZE;
extern int ADA_ACTIONS;
extern int ADA_ACTION;
extern int NUM_OUTPUT;
extern int NUMUE;
extern const int UPDATE_FREQUENCY;

//#define HLOG
//#define NET_LOG
#define QLOG

//* hperf log
//#define TIME_LOG
//#define TTI_LOG
#define REWARD_LOG
#define PERF_LOG
#define LOSS_LOG
//#define WEIGHT_LOG

#ifdef HLOG
#define h_log(fmt, ...) \
    do { fprintf(stderr, "[hoonhwi] Log: " fmt, ## __VA_ARGS__); } while (0)        
#endif
#ifndef HLOG
#define h_log(fmt, ...) \
    do {  } while (0)        
#endif // HLOG

#ifdef TIME_LOG
#define t_log(fmt, ...) \
    do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)        
#endif
#ifndef TIME_LOG
#define t_log(fmt, ...) \
    do {  } while (0)        
#endif // t_log

#ifdef NET_LOG
#define n_log(fmt, ...) \
    do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)        
#endif
#ifndef NET_LOG
#define n_log(fmt, ...) \
    do {  } while (0)        
#endif // inf_log

#ifdef QLOG
#define q_log(fmt, ...) \
    do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)        
#endif
#ifndef QLOG
#define q_log(fmt, ...) \
    do {  } while (0)        
#endif // QLOG

struct DQNImpl : torch::nn::Module {

	 DQNImpl(int64_t state_size, int64_t action_size, int64_t batch_size) 
	 	//:conv1(torch::nn::Conv2dOptions(1, batch_size, 3).stride(1).padding(1)), // Conv2dOptions(in channels, out channels, kernel size)
		:conv1(torch::nn::Conv3dOptions(batch_size, batch_size*2, {1,3,3}).stride(1)), // Conv2dOptions(in channels, out channels, kernel size)
		conv2(torch::nn::Conv3dOptions(batch_size*2, batch_size*4, {1,3,3}).stride(1)),
		conv3(torch::nn::Conv3dOptions(64, 64, 3).stride(1)),
		linear1(torch::nn::Linear(70*NUMUE, 512) ), //  for freq(10), 70 for freq(5), 21 for freq(1)
	 	linear2(torch::nn::Linear(512, action_size) ),
		state_conv1(torch::nn::Conv3dOptions(1, 3, {1,3,3}).stride(1)),
		state_conv2(torch::nn::Conv3dOptions(3, 6, {1,3,3}).stride(1)),
		state_conv3(torch::nn::Conv3dOptions(64, 64, 3).stride(1)),
		state_linear1(torch::nn::Linear(105*NUMUE, 512) ), //  for freq(10), 105 for freq(5), 21 for freq(1)
	 	state_linear2(torch::nn::Linear(512, action_size) )
		// conv1(torch::nn::Conv3dOptions(1, 1, 3).stride(1).padding(1)),
		// conv2(torch::nn::Conv2dOptions(batch_size, batch_size, 3).stride(1).padding(1)),
		// conv3(torch::nn::Conv2dOptions(64, 64, 3).stride(1)),
		// linear1(torch::nn::Linear(200*NUMUE, 512) ), //200*numue
	 	// linear2(torch::nn::Linear(512, action_size) ),
		// state_conv1(torch::nn::Conv2dOptions(1, batch_size, 3).stride(1).padding(1)),
		// state_conv2(torch::nn::Conv2dOptions(batch_size, batch_size, 3).stride(1).padding(1)),
		// state_conv3(torch::nn::Conv2dOptions(64, 64, 3).stride(1)),
		// state_linear1(torch::nn::Linear(200*NUMUE, 512) ),
	 	// state_linear2(torch::nn::Linear(512, action_size) )		
		{
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

	torch::Tensor forward(torch::Tensor input){
		// torch::Tensor h1 = linear1(input);
		// torch::Tensor h2 = torch::tanh(h1);
		//input = input.unsqueeze(2);
		#ifdef NET_LOG
		n_log("forward: before conv1 *******************\n");
		input.print();
		#endif
		//std::cout << input << std::endl;

		torch::Tensor h1 = conv1(input);
		#ifdef NET_LOG
		n_log("forward: after conv1, h1:\n");
		h1.print();
		#endif
		//std::cout << h1 << std::endl;

		h1 = torch::relu(h1);
		#ifdef NET_LOG
		n_log("forward: after relu, h1:\n");
		h1.print();
		#endif
		//std::cout << h1 << std::endl;

		torch::Tensor h2 = conv2(h1);
		#ifdef NET_LOG
		n_log("forward: after conv2, h2:\n");
		h2.print();
		#endif
		//std::cout << h2 << std::endl;
		
		h2 = torch::relu(h2);
		#ifdef NET_LOG
		n_log("forward: after relu, h2:\n");
		h2.print();
		#endif
		//std::cout << h2 << std::endl;

		//h2 = conv3(h2);
		//h_log("after conv3, h2:\n");
		//h2.print();
		//h2 = torch::relu(h2);


		h2 = h2.view({BATCH_SIZE, NUM_OUTPUT, -1});
		#ifdef NET_LOG
		n_log("forward: after view h2:\n");
		h2.print();
		#endif
		//std::cout << h2 << std::endl;

		torch::Tensor h3 = linear1(h2);
		#ifdef NET_LOG
		n_log("forward: after linear1, h3:\n");
		h3.print();
		#endif
		//std::cout << h3 << std::endl;

		h3 = torch::relu(h3);
		#ifdef NET_LOG
		n_log("forward: after relu, h3:\n");
		h3.print();
		#endif
		//std::cout << h3 << std::endl;

		h3 = linear2(h3);
		#ifdef NET_LOG
		n_log("forward: after linear2, h3:\n");
		h3.print();
		#endif
		//std::cout << h3 << std::endl;

		return h3;
	}

	torch::Tensor state_forward(torch::Tensor input){
		// torch::Tensor h1 = linear1(input);
		// torch::Tensor h2 = torch::tanh(h1);
		input = input.unsqueeze(0); //1channel, size: 1 x update_freq x channel x ue
		#ifdef NET_LOG
		n_log("state_forward before conv1\n");
		input.print();
		#endif

		torch::Tensor h1 = state_conv1(input);
		#ifdef NET_LOG
		n_log("after conv1, h1:\n");
		h1.print();
		#endif
		h1 = torch::relu(h1);

		torch::Tensor h2 = state_conv2(h1);
		#ifdef NET_LOG
		n_log("after conv2, h2:\n");
		h2.print();
		#endif
		h2 = torch::relu(h2);

		//h2 = state_conv3(h2);
		//h_log("after conv3, h2:\n");
		//h2.print();
		//h2 = torch::relu(h2);

		h2 = h2.view({NUM_OUTPUT, -1});
		#ifdef NET_LOG
		n_log("after VIEW h2:\n");
		h2.print();
		#endif

		h2 = state_linear1(h2);
		#ifdef NET_LOG
		n_log("after linear1, h2:\n");
		h2.print();
		#endif
		h2 = torch::relu(h2);

		h2 = state_linear2(h2);
		#ifdef NET_LOG
		n_log("after linear2, h2:\n");
		h2.print();
		#endif

		return h2;
	}

	torch::nn::Linear linear1, linear2, state_linear1, state_linear2;
	torch::nn::Conv3d state_conv1, state_conv2, state_conv3;
	torch::nn::Conv3d conv1, conv2, conv3;
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