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


#ifndef DQN_PACKETSCHEDULER_MLWDF_H_
#define DQN_PACKETSCHEDULER_MLWDF_H_

#include "downlink-packet-scheduler.h"

class DQN_PacketScheduler_MLWDF : public DownlinkPacketScheduler {
public:
	DQN_PacketScheduler_MLWDF();
	virtual ~DQN_PacketScheduler_MLWDF();
	void DoSchedule();

	virtual double ComputeSchedulingMetric (RadioBearer *bearer, double spectralEfficiency, int subChannel);

private:

};

#endif /* DLPFPACKETSCHEDULER_H_ */
