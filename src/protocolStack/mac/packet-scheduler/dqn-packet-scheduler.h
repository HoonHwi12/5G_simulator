/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010,2011,2012,2013 TELEMATICS LAB, Politecnico di Bari
 *
 * This file is part of LTE-Sim
 *
 * LTE-Sim is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation;
 *
 * LTE-Sim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LTE-Sim; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Giuseppe Piro <g.piro@poliba.it>
 */


#ifndef DQN_PACKETSCHEDULER_H_
#define DQN_PACKETSCHEDULER_H_

#include "downlink-packet-scheduler.h"

class DQN_PacketScheduler : public DownlinkPacketScheduler {
public:
	DQN_PacketScheduler();
	virtual ~DQN_PacketScheduler();
	void DoSchedule();

	virtual double ComputeSchedulingMetric (RadioBearer *bearer, double spectralEfficiency, int subChannel);
	double ComputeEXPcomponent(RadioBearer *bearer);
	double ComputeEXP(RadioBearer *bearer);
	double ComputeLOG(RadioBearer *bearer);
	double ComputeMLWDF(RadioBearer *bearer);
	double ComputeEXPrule(RadioBearer *bearer);

	double mean_mlwdf = 0;
	double mean_expmlwdf = 0;
	double mean_exppf = 0;
	double mean_log = 0;

	double cov_mlwdf = 0;
	double cov_expmlwdf = 0;
	double cov_exppf = 0;
	double cov_log = 0;

	double min_mlwdf = 100;
	double min_expmlwdf = 100;
	double min_exppf = 100;
	double min_log = 100;

	double max_mlwdf = 0;
	double max_expmlwdf = 0;
	double max_exppf = 0;
	double max_log = 0;

	int t_samples = 0;


private:

};

#endif /* DLPFPACKETSCHEDULER_H_ */
