#ifndef ReplayMemory_H
#define ReplayMemory_H

#include <deque>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <random>
#include <torch/torch.h>

class ReplayMemory{
	typedef std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor> experience;
    private: 
        int64_t mem_size_max;
        std::deque<experience> memory;
        static int randomIndex(int i){
            std::random_device rd;
            std::mt19937 mt(rd());
            std::uniform_int_distribution<> dist(0, i);
            return dist(mt);
        }
    public: 
        ReplayMemory(int64_t size) {
            mem_size_max = size;
            // std::cout << "RepMemory Ready  ~" << std::endl;
         }

        ~ReplayMemory(){}

        int64_t capacity() const{
            return mem_size_max;
        }

        int64_t size() const{
            return memory.size();
        }

        void push(torch::Tensor _state1, torch::Tensor _action, torch::Tensor _state2, torch::Tensor _reward){
            // create a new experience
            experience this_experience (_state1, _action, _state2 , _reward);
            // if we have reached capacity, remove the oldest experience
            if(memory.size() >= mem_size_max){
                memory.pop_back();
            }
            memory.push_front(this_experience);
        }
        bool canProvideSamples(size_t _batchSize){
            if(size() >= _batchSize){
                return true;
            }
            return false;
        }

        std::vector<experience> sampleMemory(int batch_size){
        	std::vector<experience> samples(batch_size);
        	std::sample(memory.begin(), memory.end(), 
        				samples.begin(), samples.size(),
        				std::mt19937{std::random_device{}()});
            return samples;
        }
};

#endif