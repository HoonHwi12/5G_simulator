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

#include "dqn-packet-scheduler_mlwdf.h"
#include "../mac-entity.h"
#include "../../packet/Packet.h"
#include "../../packet/packet-burst.h"
#include "../../../device/NetworkNode.h"
#include "../../../flows/radio-bearer.h"
#include "../../../protocolStack/rrc/rrc-entity.h"
#include "../../../flows/application/Application.h"
#include "../../../device/GNodeB.h"
#include "../../../protocolStack/mac/AMCModule.h"
#include "../../../phy/gnb-phy.h"
#include "../../../core/spectrum/bandwidth-manager.h"
#include "../../../core/idealMessages/ideal-control-messages.h"

#include "dl-exp-packet-scheduler.h"
#include "dl-fls-packet-scheduler.h"
#include "../../../flows/QoS/QoSForEXP.h"
#include "../../../flows/QoS/QoSForM_LWDF.h"

extern double d_dqn_output0;
extern double d_dqn_output1;
extern double d_dqn_output2;
extern double d_dqn_output3;

DQN_PacketScheduler_MLWDF::DQN_PacketScheduler_MLWDF()
{
  SetMacEntity (0);
  CreateFlowsToSchedule ();
}

DQN_PacketScheduler_MLWDF::~DQN_PacketScheduler_MLWDF()
{
  Destroy ();
}

void
DQN_PacketScheduler_MLWDF::DoSchedule ()
{
	//printf("Start DQN packet scheduler\n", GetMacEntity ()->GetDevice ()->GetIDNetworkNode());
#ifdef SCHEDULER_DEBUG
	std::cout << "Start EXP packet scheduler for node "
			<< GetMacEntity ()->GetDevice ()->GetIDNetworkNode()<< std::endl;
#endif

  UpdateAverageTransmissionRate ();
  CheckForDLDropPackets ();
  SelectFlowsToSchedule();

  if (GetFlowsToSchedule ()->size() == 0)
	{}
  else
	{
	  RBsAllocation ();
	}

  StopSchedule ();
  ClearFlowsToSchedule ();
}


double
DQN_PacketScheduler_MLWDF::ComputeSchedulingMetric (RadioBearer *bearer, double spectralEfficiency, int subChannel)
{
   /*
   * For the DQN scheduler the metric is computed
   * as follows:
   */
  double metric;
  double weight0 = d_dqn_output0 / 100;
  double weight1 = d_dqn_output1 / 100;
  double weight2 = d_dqn_output2 / 100; 
  double weight3 = d_dqn_output3 / 100;

  //printf("Compute DQN metric weight0(%f)/weight1(%f)/weight2(%f)\n", weight0, weight1, weight2);
  

  if (bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_INFINITE_BUFFER ||
      bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_CBR )
  {
	  metric = (spectralEfficiency * 180000.) / bearer->GetAverageTransmissionRate();
  }
  else
  {
    QoSForM_LWDF *qos = (QoSForM_LWDF*) bearer->GetQoSParameters ();
    double a = (-log10 (qos->GetDropProbability())) / qos->GetMaxDelay ();
    double HOL = bearer->GetHeadOfLinePacketDelay ();
    
    metric = pow( (spectralEfficiency * 180000.) / bearer->GetAverageTransmissionRate(), weight0) // PF
               * pow(a, weight1) * pow(HOL, weight2); // MLWDF

  }

    return metric;
}
