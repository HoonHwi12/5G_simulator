#ifndef EpsilonGreedy_H
#define EpsilonGreedy_H

#include <iostream>
#include <cmath>

class EpsilonGreedy{
	private:
		float start;
		float end;
		float decay;
	public:
		EpsilonGreedy(float start_, float end_ , float decay_){
			start = start_;
			end   = end_;
			decay = decay_;
			std::cout << "EpsGreedy Ready ~" << std::endl;
		}

		float explorationRate(int current_step){
			return end + (start - end) * exp(-1 * decay * current_step);
		}
};
#endif