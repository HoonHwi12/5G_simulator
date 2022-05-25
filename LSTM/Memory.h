#ifndef Memory_H
#define Memory_H

#include <deque>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <random>
#include <torch/torch.h>

class Memory{
	private:
		std::deque<torch::Tensor> memory;
		int64_t mem_size_max;
		
	public:
		Memory(int64_t size){
			mem_size_max = size;
		}
		~Memory(){}
		
		int64_t capacity() const{
			return mem_size_max;
		}
		int64_t size() const{
			return memory.size();
		}
		void push(torch::Tensor _trace){
			// create new trace set
			torch::Tensor this_trace = _trace;
			// reach max capacity
			if (memory.size() >= mem_size_max)
				memory.pop_front();
			memory.push_back(this_trace);
		}
		
		void print(){
			for (std::deque<torch::Tensor>::iterator it=memory.begin(); it !=memory.end(); it++)
				std::cout << "item: " << *it << std::endl;
		}
		using iterator=typename std::deque<torch::Tensor>::iterator;
		iterator begin(){
			return memory.begin();
		}
		iterator end(){
			return memory.end();
		}
		

};
#endif