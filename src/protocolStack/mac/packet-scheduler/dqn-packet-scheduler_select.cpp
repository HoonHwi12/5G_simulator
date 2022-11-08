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

#include "dqn-packet-scheduler_select.h"
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

DQN_PacketScheduler_SELECT::DQN_PacketScheduler_SELECT()
{
  SetMacEntity (0);
  CreateFlowsToSchedule ();
}

DQN_PacketScheduler_SELECT::~DQN_PacketScheduler_SELECT()
{
  Destroy ();
}

void
DQN_PacketScheduler_SELECT::DoSchedule ()
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
DQN_PacketScheduler_SELECT::ComputeSchedulingMetric (RadioBearer *bearer, double spectralEfficiency, int subChannel)
{
   /*
   * For the DQN scheduler the metric is computed
   * as follows:
   */
  double metric;
  double selected_metric;
  double weight0 = d_dqn_output0 / 100;
  double weight1 = d_dqn_output1 / 100;
  double weight2 = d_dqn_output2 / 100; 
  double weight3 = d_dqn_output3 / 100;

  //printf("Compute DQN metric weight0(%f)/weight1(%f)/weight2(%f)\n", weight0, weight1, weight2);

  if (bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_INFINITE_BUFFER ||
      bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_CBR )
      // bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_TRACE_BASED ||
		  // bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_VOIP)
  {
	  metric = (spectralEfficiency * 180000.) / bearer->GetAverageTransmissionRate();
  }
  else
  {
    if(weight0==0)
    {
      //compute AW~
      FlowsToSchedule *flowsToSchedule = GetFlowsToSchedule ();
      FlowToSchedule *flow;

      double AW_avgAW;
      double m_aW = 0;
      int nbFlow = 0;

      // compute AW
      for (auto flow : *GetFlowsToSchedule())
      {
        RadioBearer *bearer = flow->GetBearer ();

        if (bearer->HasPackets ())
          {
            if ((bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_TRACE_BASED)
                ||
                (bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_VOIP))
              {
                QoSForEXP *qos = (QoSForEXP*) bearer->GetQoSParameters ();
                double aWi =  - (log10 (qos->GetDropProbability())
                                  /
                                  qos->GetMaxDelay ());
                double HOL = bearer->GetHeadOfLinePacketDelay ();
                aWi = pow(aWi, weight1);
                HOL = pow(HOL, weight2);

                aWi = aWi * HOL;
                m_aW += aWi;
                nbFlow++;
              }
          }
      }
      m_aW = m_aW/nbFlow;
      if (m_aW < 0.000001) m_aW=0;
      //~compute AW

      QoSForEXP *qos = (QoSForEXP*) bearer->GetQoSParameters ();

      double HOL = bearer->GetHeadOfLinePacketDelay ();
      HOL = pow(HOL, weight2);

      double alfa = -log10(qos->GetDropProbability()) / qos->GetMaxDelay ();
      alfa = pow(alfa, weight1);
      
      double AW = alfa * HOL;

      if (AW < 0.000001) AW=0;

      AW_avgAW = AW - m_aW;

      if (AW_avgAW < 0.000001) AW_avgAW=0;

      selected_metric = exp ( AW_avgAW / (1 + sqrt (m_aW)) );
    }
    else if(weight0==1)
    {
      QoSParameters *qos = bearer->GetQoSParameters ();
      double HOL = bearer->GetHeadOfLinePacketDelay ();
      double targetDelay = qos->GetMaxDelay ();

      targetDelay = pow(targetDelay, weight3);
      HOL = pow(HOL, weight2);

      selected_metric = log (1.1 + ( (5 * HOL) / targetDelay ));
    }
    else if(weight0==2)
    {
      QoSForM_LWDF *qos = (QoSForM_LWDF*) bearer->GetQoSParameters ();

      double a = (-log10 (qos->GetDropProbability())) / qos->GetMaxDelay ();
      double HOL = bearer->GetHeadOfLinePacketDelay ();

      a = pow(a, weight1);
      HOL = pow(HOL, weight2);

      selected_metric = (a * HOL);
    }
    else if(weight0==3)
    {
      double avgHOL = 0.;
      int nbFlows = 0;

      // compute average of hol delays
      for (auto flow : *GetFlowsToSchedule())
      {
        if (flow->GetBearer ()->HasPackets ())
        {
        if ((flow->GetBearer ()->GetApplication ()->GetApplicationType ()
              == Application::APPLICATION_TYPE_TRACE_BASED)
            ||
            (flow->GetBearer ()->GetApplication ()->GetApplicationType ()
              == Application::APPLICATION_TYPE_VOIP))
          {
            avgHOL += flow->GetBearer ()->GetHeadOfLinePacketDelay ();
            nbFlows++;
          }
        }
      }
      avgHOL = pow(avgHOL, weight2);
      double m_avgHOLDelayes = avgHOL/nbFlows;

      QoSParameters *qos = bearer->GetQoSParameters ();
      double HOL = bearer->GetHeadOfLinePacketDelay ();
      double targetDelay = qos->GetMaxDelay ();

      targetDelay = pow(targetDelay, weight3);

      //COMPUTE METRIC USING EXP RULE:
      double numerator = (6/targetDelay) * HOL;
      double denominator = (1 + sqrt (m_avgHOLDelayes));

      selected_metric = exp(numerator / denominator);
    }

    metric = pow( (spectralEfficiency * 180000.) / bearer->GetAverageTransmissionRate(), weight1) // PF
               * selected_metric;
  }

    return metric;
}

double DQN_PacketScheduler_SELECT::ComputeEXPrule(RadioBearer *bearer)
{
  double avgHOL = 0.;
  int nbFlows = 0;

  // compute average of hol delays
  for (auto flow : *GetFlowsToSchedule())
  {
    if (flow->GetBearer ()->HasPackets ())
      {
        if ((flow->GetBearer ()->GetApplication ()->GetApplicationType ()
              == Application::APPLICATION_TYPE_TRACE_BASED)
            ||
            (flow->GetBearer ()->GetApplication ()->GetApplicationType ()
              == Application::APPLICATION_TYPE_VOIP))
          {
            avgHOL += flow->GetBearer ()->GetHeadOfLinePacketDelay ();
            nbFlows++;
          }
      }
  }
  double m_avgHOLDelayes = avgHOL/nbFlows;

  QoSParameters *qos = bearer->GetQoSParameters ();
  double HOL = bearer->GetHeadOfLinePacketDelay ();
  double targetDelay = qos->GetMaxDelay ();

  //COMPUTE METRIC USING EXP RULE:
  double numerator = (6/targetDelay) * HOL;
  double denominator = (1 + sqrt (m_avgHOLDelayes));

  return exp(numerator / denominator);
}



double DQN_PacketScheduler_SELECT::ComputeEXP(RadioBearer *bearer)
{
  //compute AW~
  FlowsToSchedule *flowsToSchedule = GetFlowsToSchedule ();
  FlowsToSchedule::iterator iter;
  FlowToSchedule *flow;

  double AW_avgAW;
  double m_aW = 0;
  int nbFlow = 0;

  // compute AW
  for (auto flow : *GetFlowsToSchedule())
  {
    RadioBearer *bearer = flow->GetBearer ();

    if (bearer->HasPackets ())
      {
        if ((bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_TRACE_BASED)
            ||
            (bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_VOIP))
          {
            QoSForEXP *qos = (QoSForEXP*) bearer->GetQoSParameters ();
            double aWi =  - (log10 (qos->GetDropProbability())
                              /
                              qos->GetMaxDelay ());
            double HOL = bearer->GetHeadOfLinePacketDelay ();
            aWi = aWi * HOL;
            m_aW += aWi;
            nbFlow++;
          }
      }
  }
  m_aW = m_aW/nbFlow;
  if (m_aW < 0.000001) m_aW=0;
  //~compute AW

  QoSForEXP *qos = (QoSForEXP*) bearer->GetQoSParameters ();

  double HOL = bearer->GetHeadOfLinePacketDelay ();
  double alfa = -log10(qos->GetDropProbability()) / qos->GetMaxDelay ();
  
  double AW = alfa * HOL;

  if (AW < 0.000001) AW=0;

  AW_avgAW = AW - m_aW;

  if (AW_avgAW < 0.000001) AW_avgAW=0;

  return exp ( AW_avgAW / (1 + sqrt (m_aW)) );
}


double DQN_PacketScheduler_SELECT::ComputeLOG(RadioBearer *bearer)
{
  QoSParameters *qos = bearer->GetQoSParameters ();
  double HOL = bearer->GetHeadOfLinePacketDelay ();
  double targetDelay = qos->GetMaxDelay ();

  return log (1.1 + ( (5 * HOL) / targetDelay ));
}