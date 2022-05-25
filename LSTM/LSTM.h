#include <torch/torch.h>
#include <cstdlib>
#include <cstdio>

using torch::indexing::Slice;
class LSTMImpl : public torch::nn::Module{
	public:
		//LSTMImpl(int64_t input_size, int64_t hidden_size, int64_t num_classes);
		//torch::Tensor forward(torch::Tensor input);

		LSTMImpl(int64_t input_size, int64_t hidden_size, int64_t num_class)
			:lstm(torch::nn::LSTMOptions(input_size, hidden_size).num_layers(20)), fc(torch::nn::Linear(hidden_size,num_class)){
				register_module("lstm", lstm);
				register_module("fc", fc);
			}

		torch::Tensor forward(torch::Tensor input){
			input =  std::get<0>(lstm->forward(input)).index({Slice(), -1});
			output = fc->forward(input);

			return output;


		}

	private:
		torch::Tensor output;
		torch::nn::LSTM lstm;
		torch::nn::Linear fc;
};


TORCH_MODULE(LSTM);

//lass GRUImpl : public torch::nn::Module{
//	public:
//