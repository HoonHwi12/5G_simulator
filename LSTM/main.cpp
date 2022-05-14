// by ljh

#include "LSTM.h"
#include <iomanip>
#include <algorithm>


//BY SHJ


#include <torch/torch.h>

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <array>
#include <deque>
#include <iterator>

#include <vector>
#include <queue>

#include "Memory.h"

//By HH
#include "../src/shared-memory.cpp"

/*HyperParams*/
void initWeights(torch::nn::Module& m);

int main(int argc, char** argv){

	std::string model_name;
	//std::string before_model_name;
//
	//int curts = 0;  // 1 tti 
	//int packet = 0; // packet size 
	//int cnt = 0; // 100 TTI packet count
	//int temp = 0 ; // 100 packet size add 
//
	//int temp_curt = 0;
	////
	//std::queue<float> train_set;
	//std::queue<float> curtti;
//
	//FILE *fp = fopen(argv[1], "r");
//
	// by HH
	char *lte_buffer = (char*)malloc(SHARED_SIZE);
	char *lstm_buffer = (char*)malloc(SHARED_SIZE);
	char *dqn_buffer = (char*)malloc(SHARED_SIZE);

	int buffer_value=0;
	int lstm_output=0;
	int LTE_TTI;

	//printf("A");
	//train set
	//if(fp == NULL) {
	//	printf("Error reading %s\n", "trace_file1");
	//	return 0;
	//}
	//
	//while((fscanf(fp, "%d %d \n", &curts, &packet) == 2)){
	//	train_set.push(packet);
	//	curtti.push(curts);
	//}
	//		
//
	//int train_set_len = train_set.size(); 
//
	////load
//
	//torch::Tensor train_s =torch::zeros({1,train_set_len});
//
	//for(int i=0 ; i < train_set_len ; i++){
	//		train_s[0][i]=train_set.front();
	//		train_set.pop();
	//}
	//
	//torch::Tensor data_train = torch::zeros({10,20000});
//
	//int p=0;
	//for(int i=0; i<10; i++){
	//	for(int j=0; j<20000; j++){
	//		data_train[i][j]=train_s[0][p++];
	//	}
	//}
	
	// CUDA init
	auto cuda_available = torch::cuda::is_available();
	torch::Device device(cuda_available ? torch::kCUDA : torch::kCPU);
	
	// Hyper parameters
	const int64_t INPUT_SIZE = 20000;
	const int64_t HIDDEN_SIZE = 64;
	const int64_t NUM_CLASSES = 20000; // 5 -> 1 
	//std::string location = argv[2];
	//int epoch = atoi(argv[3]);
	//float lr = atof(argv[4]);
	//int state = atoi(argv[5]);
	//
	//double LR = lr;

	//int acc_cnt = 0;
	//auto all_acc = 0.0f;
	//std::cout << "Learning Rate: " << LR << '\n';


	//BY SHJ Network
	LSTM predictionNet(INPUT_SIZE, HIDDEN_SIZE, NUM_CLASSES);



	//predictionNet->apply(initWeights);

	predictionNet->to(device);
	// Optimizer
	torch::optim::Adam optimizer(predictionNet->parameters(), torch::optim::AdamOptions(0.01));
	//torch::Tensor output;


	std::cout << std::fixed << std::setprecision(4);
	//train the model 
	

	//float running_loss = 0.0;
	//size_t num_correct = 0;
	//int count = 10;
	//int k = 0;

/*
	if(state == 0){
		int item = atoi(argv[6]);
		//model_name = location+"_"+std::to_string(item)+"_model.pt";
		model_name = "model/400_vs/output/train_100_0_model.pt";
		for(int i = 0 ; i< epoch; i++){
			num_correct  = 0;
			k=0;
			count = 10;

			std::cout <<"-------------Epoch["<<i+1<<"]--------------"<< std::endl;
			while(count > 1){
				torch::Tensor data = torch::zeros({1,20000});
				torch::Tensor target = torch::zeros({1,20000});

				for(int j=0 ; j< 20000 ; j++){
					data[0][j] = data_train[k][j];
					target[0][j] = data_train[item][j];
				}
				
				//Min-Max Normalization(Squence)
				auto min_d = data.min();
				auto max_d = data.max();
			
				for(int j=0 ; j < 20000; j++){
					data[0][j] = (data[0][j] - min_d)/(max_d-min_d);
				}
				auto min_p = target.min();
				auto max_p = target.max();

				for(int j=0 ; j < 20000; j++){
					target[0][j] = (target[0][j] - min_p)/(max_p-min_p);
				}		
				k++;

				count--;
				data = data.unsqueeze_(0);
				target = target.unsqueeze_(0);


				data = data.cuda();

				target = target.cuda();

				
				output = predictionNet->forward(data);

				output.unsqueeze_(0);
				
				//std::cout <<"output "<<output<<std::endl;
				auto loss = torch::nn::functional::l1_loss(output, target);

				std::cout << "Loss: "<< loss;
				//update running loss
				running_loss +=loss.item<float>() * data.size(0);
				//Calculate prediction
				
				optimizer.zero_grad();
				loss.backward();
				optimizer.step();

				auto low_target = target - (target * 0.05);
				auto high_target = target + (target * 0.05);
	//
			
				for(int i=0 ; i < INPUT_SIZE ;i++){
					if(output[0][0][i].item<float>() == target[0][0][i].item<float>()){
						num_correct++;
					}else if(low_target[0][0][i].item<float>() <= output[0][0][i].item<float>() && output[0][0][i].item<float>()<= high_target[0][0][i].item<float>()){
						num_correct++;
					}
				}
				
				auto train_accuracy = (static_cast<double>(num_correct) / INPUT_SIZE)*100;
				auto sample_mean_loss = running_loss;
				std::cout << "Epoch [" <<i+1<<"/"<< epoch<<"]"<<"item["<<k<<"]  Trainset - Loss:" << sample_mean_loss<<"  Accuracy: "<<train_accuracy<<"%"<<std::endl;

				num_correct = 0;
				running_loss = 0;
			}
			//item model save
			torch::save(predictionNet, model_name);
		}
	}
	else{ */
		//inference 5 = 1 			
		//LTE-sim tti id(curtti), packet size receive
		//
		//model choice(item / epoch-1 자리변경)

		std::string location = "../model/400_vc/output/train_100";
		int epoch = 0;
		model_name = location+"_"+std::to_string(epoch)+"_model.pt";
		//model_name = "../model/400_vc/output/train_100_0_model.pt";
		torch::load(predictionNet,model_name);


		//by HH Trace File
		int index, packetSize = 0;
		std::ifstream ifTraceFile;
		std::string traceFile = "../trace/ue_400_vid_200_cbr_200";
		ifTraceFile.open (traceFile, std::ifstream::in);
		torch::Tensor data_test;

		if (!ifTraceFile.good ())
		{
			std::cout << " Wrong Trace File!"<< std::endl;
			data_test = torch::randn({1,20000});
		}
		else
		{
			data_test = torch::zeros({1,20000});

			while (index != 19999)
			{
				ifTraceFile >> index >> packetSize;
				data_test[0][index] = packetSize;

				//printf("index %d size %d\n", index, packetSize);
			}
		}
		ifTraceFile.close ();


		//torch::Tensor data_test = torch::randn({1,20000});


		
		//for testing
		//for(int j=0; j<20000 ; j++){
		//	target_test[0][j] = data_train[0][j];
		//}
		
		//auto min_p = target_test.min();
		//auto max_p = target_test.max();

		//for(int j=0 ; j < 20000; j++){
		//	target_test[0][j] = (target_test[0][j] - min_p)/(max_p-min_p);
		//}
		
		// i = ( N number of prediction tti)



		// by HH
		int lte_shmid;
		int dqn_shmid;
		int lstm_shmid = SharedMemoryCreate(LSTM_KEY);


		printf("waiting for LTE-Sim\n");
		// LTE-sim 에서 PID 받을때까지 대기
		while(buffer_value != -1)
		{     
			lte_shmid = SharedMemoryInit(LTE_KEY);
			if(lte_shmid != -1)
			{
				SharedMemoryRead(lte_shmid, lte_buffer);
				buffer_value = atoi(lte_buffer);

				// 받은 PID 전송
				sprintf(lstm_buffer,"%d", atoi(lte_buffer));
				if( SharedMemoryWrite(lstm_shmid, lstm_buffer) == -1 )
				{
					printf("Shared Memory Write Error\n");
					return FAIL;
				}
			}
			
			sleep(0.0001);
		} 

		// lstm memory 초기화
		sprintf(lstm_buffer,"%d", -1);
		SharedMemoryWrite(lstm_shmid, lstm_buffer);		
		dqn_shmid = SharedMemoryInit(DQN_KEY);

		//LSTM start
		while(1)
		{	
			int index_point=9;
			while(1)
			{
				// LTE-Sim 에서 TTI 데이터 받을때까지 대기
				do
				{
					SharedMemoryRead(lstm_shmid, lstm_buffer);
					LTE_TTI = atoi(lstm_buffer);
				}while( LTE_TTI == -1 );

				// Receive Packet Size
				if( SharedMemoryRead(lte_shmid, lte_buffer) == -1 )
				{
					printf("Shared Memory Read Error\n");
					return FAIL;
				}
				buffer_value = atoi(lte_buffer);
				//printf("index: %d value:%d\n", LTE_TTI-index_point, buffer_value);

				data_test[0][LTE_TTI-index_point] = buffer_value;

				// send receive signal to LTE-Sim
				sprintf(lstm_buffer,"%d", -1);
				if( SharedMemoryWrite(lstm_shmid, lstm_buffer) == -1)
				{
					printf("Shared Memory Write Error\n");
					return FAIL;
				}

				if(index_point == 0)
				{
					printf("receive complete\n");
					break;
				}
				index_point--;
			}
			
			clock_t lstm_start=clock();
			// Min-Max norm
			auto min_d = data_test.min();
			auto max_d = data_test.max();

			for(int j=0 ; j < 20000; j++){
				data_test[0][j] = (data_test[0][j] - min_d)/(max_d-min_d);
			}

			data_test = data_test.unsqueeze_(0);
			data_test = data_test.cuda();
		
			std::cout <<min_d<<max_d;	
			//output (10 tti -> next 10tti) inverse norm
			torch::Tensor output2 = predictionNet->forward(data_test);
			for(int i =LTE_TTI+10; i<LTE_TTI+20; i++){
				std::cout << output2[0][i];
				output2[0][i]=output2[0][i]*(max_d-min_d)+min_d;
				std::cout << output2[0][i];
			}

			lstm_output = 0;
			for(int i =LTE_TTI+10; i<LTE_TTI+20; i++){
				lstm_output += output2[0][i].item<int>();
			}
			
			// Send output to DQN
			sprintf(dqn_buffer, "%d", lstm_output);
			if( SharedMemoryWrite(dqn_shmid, dqn_buffer) == -1)
			{
				printf("Shared Memory write Error\n");
				return FAIL;
			}
			//printf("Write %s to dqn\n", dqn_buffer);
			data_test = data_test.squeeze_(0);
		}
		

		/* for checking
		auto low_target_test = target_test - (target_test * 0.05);
		auto high_target_test = target_test + (target_test * 0.05);
		float acc_corr = 0;

		for(int i=10000 ; i < 10010 ;i++){
			if(output2[0][i].item<float>() == target_test[0][i].item<float>()){
				acc_corr++;
			}else if(low_target_test[0][i].item<float>() <= output2[0][i].item<float>() && output2[0][i].item<float>()<= high_target_test[0][i].item<float>()){
				acc_corr++;
			}
		}
		std::cout <<acc_corr <<std::endl;
		std::cout <<"Inference accurracy:"<<(acc_corr/10)*100<<"%"<<std::endl;
		*/
	//}


		//printf("----------------Training finisted !!!-------------------- \n\n\n");
		//printf("------------Testing ------------\n");
//
/*		predictionNet->eval();
		torch::NoGradGuard no_grad;
//
		running_loss = 0.0f;
		num_correct = 0;
//
		count = 1;
		k = 0;
		while(count > 0){
			
			torch::Tensor data = torch::zeros({1,20000});
			torch::Tensor target = torch::zeros({1,20000});
			for(int i=0 ; i<20000; i++){
				data[0][i] = test_s[k][i];
				target[0][i] = data_train[0][i];
			}
			k++;
			count--;
//
			data = data.unsqueeze_(0);
			target = target.unsqueeze_(0);
//
			data = data.cuda();
			target = target.cuda();
//
			output= predictionNet->forward(data);
//
			output.unsqueeze_(0);
			auto loss = torch::nn::functional::l1_loss(output, target);

			running_loss +=loss.item<float>() * data.size(0);
//
			//auto prediction_size = output.argmax(1);
//
			auto low_target = target - (target * 0.05);
			auto high_target = target + (target * 0.05);
//
		
			for(int i=0 ; i < test_set_len ;i++){
				if(output[0][0][i].item<float>() == target[0][0][i].item<float>()){
					//std::cout << " output["<<i<<"]: "<<output[0][0][i].item<float>()<<" target["<<i<<"]: "<<target[0][0][i].item<float>()<<std::endl;
					num_correct++;
				}else if(low_target[0][0][i].item<float>() <= output[0][0][i].item<float>() && output[0][0][i].item<float>()<= high_target[0][0][i].item<float>()){
					//std::cout << " output["<<i<<"]: "<<output[0][0][i].item<float>()<<" target["<<i<<"]: "<<target[0][0][i].item<float>()<<std::endl;
					num_correct++;
				}
			}
			
			//num_correct += prediction_size.eq(target).sum().item<int64_t>();
//
		}

//
		auto test_accuracy = (static_cast<double>(num_correct) / test_set_len)*100;
		auto test_sample_mean_loss = running_loss;
//
		std::cout <<"Testset - Loss: " <<test_sample_mean_loss<< ", Accuracy: " << test_accuracy <<"%"<< std::endl;
		printf("----------------Testing finisted !!! ------------------ \n\n\n");*/
	return 0;
}

//torch::Tensor processSequence(Memory* _test_queue){
//	torch::Tensor _sequence;
//	std::vector<torch::Tensor> _temp;
//	for(auto iter = _test_queue->begin(); iter != _test_queue->end();iter ++ ){
//			_temp.push(*iter);
//	}
//	_sequence = torch::cat(_temp,0);
//	return _sequence;
//}
//
void initWeights(torch::nn::Module& m){
  if ((typeid(m) == typeid(torch::nn::LSTM)) || (typeid(m) == typeid(torch::nn::LSTMImpl))) {
    auto p = m.named_parameters(false);
    auto w = p.find("weight");
    auto b = p.find("bias");

    if (w != nullptr) torch::nn::init::xavier_uniform_(*w);
    if (b != nullptr) torch::nn::init::constant_(*b, 0.01);
  }
}
