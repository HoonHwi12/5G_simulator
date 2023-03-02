#ifndef ReplayMemory_H
#define ReplayMemory_H

#include <deque>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <random>
#include <torch/torch.h>
#include <tuple>

//std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor> experience[MEM_SIZE];
typedef std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor> tuple_list;
typedef std::vector< std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor> > sample_list;
int mem_capacity=0;
int mem_iter=0;

bool canProvideSamples(){
    if (mem_capacity >= BATCH_SIZE){
        return true;
    }
    return false;
}

static int randomIndex(int i){
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<> dist(0, i);
    return dist(mt);
}

sample_list sampleMemory(tuple_list *tl)
{
    tuple_list *samples;
    sample_list return_val;
    int randomval;

    for(int i=0; i<BATCH_SIZE; i++)
    {        
        randomval = randomIndex(mem_capacity-1);
        //printf("randomval(%d): %d (%d)\n", i, randomval, mem_capacity);
        samples = tl+randomval;

        return_val.push_back(*samples);
        //std::cout << "sample" << randomval << std::get<1>(*samples.at(randomval));
    }
    return return_val;
}

// class ReplayMemory{
// 	typedef std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor> experience;
//     private: 
//         int64_t mem_size_max;
//         //std::deque<experience> memory;
        
//         static int randomIndex(int i){
//             std::random_device rd;
//             std::mt19937 mt(rd());
//             std::uniform_int_distribution<> dist(0, i);
//             return dist(mt);
//         }
//     public: 
//         std::deque<experience> memory;
        
//         ReplayMemory(int64_t size) {
//             mem_size_max = size;
//             // std::cout << "RepMemory Ready  ~" << std::endl;
//          }
//         ~ReplayMemory(){}

//         int64_t capacity() const{
//             return mem_size_max;
//         }

//         int64_t size() const{
//             return memory.size();
//         }

//         void push(torch::Tensor _state1, torch::Tensor _action, torch::Tensor _state2, torch::Tensor _reward){
//             // create a new experience
//             experience this_experience (_state1, _action, _state2 , _reward);
//             // if we have reached capacity, remove the oldest experience
//             if(memory.size() >= mem_size_max){
//                 memory.pop_back();
//             }
//             memory.push_front(this_experience);
//             //memory.push_back(this_experience);
//             //memory.insert(memory.begin(), this_experience);
//             // printf("mem_push %ld %ld %d\n", memory.begin(), memory.end(), memory.size());
//             // printf("mem at0(%ld) end(%ld) begin(%ld) end(%ld)\n", memory.at(0), memory.at(memory.size()-1), memory.rbegin(), memory.rend());
//         }

//         bool canProvideSamples(size_t _batchSize){
//             if(size() >= _batchSize){
//                 return true;
//             }
//             return false;
//         }

//         std::vector<experience> sampleMemory(int batch_size){
//         	std::vector<experience> samples(batch_size);
//         	// std::sample(memory.begin(), memory.end(), 
//         	// 			samples.begin(), samples.size(),
//         	// 			 std::mt19937{std::random_device{}()});
//             std::sample(memory.begin(), memory.end(), 
//         	 			samples.begin(), samples.size(),
//         	 			 std::mt19937{std::random_device{}()});
//             printf("%ld %ld %ld %d\n", memory.begin(), memory.end(), samples.begin(), samples.size());
//             return samples;
//         }
// };

#endif