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

#include "dqn-packet-scheduler.h"
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

extern int exp_flag;
extern double aWi;
extern double HOL;
extern double m_avgHOLDelayes;
extern double avgHOL;
extern double m_aW;
extern int nbFlow;

DQN_PacketScheduler::DQN_PacketScheduler()
{
  SetMacEntity (0);
  CreateFlowsToSchedule ();
}

DQN_PacketScheduler::~DQN_PacketScheduler()
{
  Destroy ();
}

void
DQN_PacketScheduler::DoSchedule ()
{
	//printf("Start DQN packet scheduler\n", GetMacEntity ()->GetDevice ()->GetIDNetworkNode());
#ifdef SCHEDULER_DEBUG
	std::cout << "Start dqn packet scheduler for node "
			<< GetMacEntity ()->GetDevice ()->GetIDNetworkNode()<< std::endl;
#endif
      
  UpdateAverageTransmissionRate ();
  CheckForDLDropPackets ();
  SelectFlowsToSchedule();
  //printf("flows size: %d\n", GetFlowsToSchedule()->size());
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
DQN_PacketScheduler::ComputeSchedulingMetric (RadioBearer *bearer, double spectralEfficiency, int subChannel)
{
   /*
   * For the DQN scheduler the metric is computed
   * as follows:
   */
  double metric;
  double seff = (spectralEfficiency * 180000.) / bearer->GetAverageTransmissionRate();
  double weight0 = d_dqn_output0 / 100;
  double weight1 = d_dqn_output1 / 100;
  double weight2 = d_dqn_output2 / 100; 
  double weight3 = d_dqn_output3 / 100;

  // weight0 = 0;
  // weight1 = 0;
  // weight2=0;
  // weight3=1;

  //printf("weight(%f %f %f %f)\n",weight0, weight1, weight2, weight3);
  if(weight0 > 1 || weight1 >1 || weight2 > 1 || weight3 > 1)
  {
    printf("weight value error(%f %f %f %f)\n", weight0,weight1,weight2,weight3);
    sleep(1000);
  }

  //printf("Compute DQN metric weight0(%f)/weight1(%f)/weight2(%f)\n", weight0, weight1, weight2);

  if (bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_INFINITE_BUFFER ||
      bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_CBR )
      //bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_TRACE_BASED ||
		  //bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_VOIP)
  {
	  metric = seff;

    //if (bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_INFINITE_BUFFER) printf("APPLICATION_TYPE_INFINITE_BUFFER\n");
    //if (bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_CBR) printf("APPLICATION_TYPE_CBR\n");
    //if (bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_TRACE_BASED) printf("APPLICATION_TYPE_TRACE_BASED\n");
    //if (bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_VOIP) printf("APPLICATION_TYPE_VOIP\n");
  }
  else
  {
    
    if(exp_flag==0)
    {
      exp_flag=1;
      ComputeEXPcomponent(bearer);
    }
    //clock_t inf_mlwdf=clock();
    double mlwdf_metric = ComputeMLWDF(bearer)*seff;
    //printf("mlwdf expm exppf log(%0.3f ", (float)(clock()-inf_mlwdf)/CLOCKS_PER_SEC*1000);

    //clock_t inf_exp=clock();
    double exp_metric = ComputeEXP(bearer)*seff;
    //printf("%0.3f ", (float)(clock()-inf_exp)/CLOCKS_PER_SEC*1000);

    //clock_t inf_exprule=clock();
    double exprule_metric = ComputeEXPrule(bearer)*seff;
    //printf("%0.3f ", (float)(clock()-inf_exprule)/CLOCKS_PER_SEC*1000);    

    //clock_t inf_log=clock();
    double log_metric = ComputeLOG(bearer)*seff;
    //printf("%0.3f)\n", (float)(clock()-inf_log)/CLOCKS_PER_SEC*1000);

    if(isfinite(mlwdf_metric) == 0) mlwdf_metric = 0;
    if(isfinite(exp_metric) == 0 ) exp_metric = 0;
    if(isfinite(exprule_metric) == 0) exprule_metric = 0;
    if(isfinite(log_metric) == 0) log_metric = 0;

/*
    t_samples++;
    mean_mlwdf = (mean_mlwdf*(t_samples-1) + mlwdf_metric) / t_samples;
    mean_expmlwdf = (mean_expmlwdf*(t_samples-1) + exprule_metric) / t_samples;
    mean_exppf = (mean_exppf*(t_samples-1) + exp_metric) / t_samples;
    mean_log = (mean_log*(t_samples-1) + log_metric) / t_samples;
    
    cov_mlwdf = cov_mlwdf + pow((mlwdf_metric-mean_mlwdf),2)/t_samples;
    cov_expmlwdf = cov_expmlwdf + pow((exprule_metric-mean_expmlwdf),2)/t_samples;
    cov_exppf = cov_exppf + pow((exp_metric-mean_exppf),2)/t_samples;
    cov_log = cov_log + pow((log_metric-mean_log),2)/t_samples;

    mlwdf_metric = (mlwdf_metric-mean_mlwdf) / sqrt(cov_mlwdf);
    exprule_metric = (exprule_metric-mean_expmlwdf) / sqrt(cov_expmlwdf);
    exp_metric = (exp_metric-mean_exppf) / sqrt(cov_exppf);
    log_metric = (log_metric-mean_log) / sqrt(cov_log);

    if(isfinite(exp_metric) == 0 ) exp_metric = 0;
    if(isfinite(log_metric) == 0) log_metric = 0;
    if(isfinite(mlwdf_metric) == 0) mlwdf_metric = 0;
    if(isfinite(exprule_metric) == 0) exprule_metric = 0;
  */

    if(min_mlwdf > mlwdf_metric) min_mlwdf=mlwdf_metric;
    if(min_expmlwdf > exp_metric) min_expmlwdf=exp_metric;
    if(min_exppf > exprule_metric) min_exppf=exprule_metric;
    if(min_log > log_metric) min_log=log_metric;

    if(max_mlwdf < mlwdf_metric) max_mlwdf=mlwdf_metric;
    if(max_expmlwdf < exp_metric) max_expmlwdf=exp_metric;
    if(max_exppf < exprule_metric) max_exppf=exprule_metric;
    if(max_log < log_metric) max_log=log_metric;

    if((max_mlwdf - min_mlwdf) != 0) mlwdf_metric = (mlwdf_metric-min_mlwdf) / (max_mlwdf - min_mlwdf);
    if((max_expmlwdf - min_expmlwdf) != 0) exp_metric = (exp_metric-min_expmlwdf) / (max_expmlwdf - min_expmlwdf);
    if((max_exppf - min_exppf) != 0) exprule_metric = (exprule_metric-min_exppf) / (max_exppf - min_exppf);
    if((max_log - min_log) != 0) log_metric = (log_metric-min_log) / (max_log - min_log);

    mlwdf_metric = pow(mlwdf_metric, weight0);
    exp_metric = pow(exp_metric, weight1);
    exprule_metric = pow(exprule_metric, weight2);
    log_metric = pow(log_metric, weight3);

    //printf("M(%f %f %f %f)",mlwdf_metric,exp_metric, exprule_metric,log_metric);

    //metric = (spectralEfficiency * 180000.) / bearer->GetAverageTransmissionRate() * ( mlwdf_metric + exp_metric + exprule_metric + log_metric );
    //metric = (spectralEfficiency * 180000.) / bearer->GetAverageTransmissionRate() * ( log_metric );
    metric = mlwdf_metric + exp_metric + exprule_metric + log_metric;
                  
  }

    //printf("m(%f)", metric);
    return metric;
}


double DQN_PacketScheduler::ComputeEXPcomponent(RadioBearer *bearer)
{
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
            avgHOL += flow->GetBearer ()->GetHeadOfLinePacketDelay ();
            aWi = aWi * HOL;
            m_aW += aWi;
            nbFlow++;
          }
      }
  }
  m_avgHOLDelayes = avgHOL/nbFlow;
  m_aW = m_aW/nbFlow;
  if (m_aW < 0.000001) m_aW=0;
}


double DQN_PacketScheduler::ComputeEXPrule(RadioBearer *bearer)
{
  //double avgHOL = 0.;
  //int nbFlows = 0;
 
  // compute average of hol delays
  // for (auto flow : *GetFlowsToSchedule())
  // {
  //   if (flow->GetBearer ()->HasPackets ())
  //     {
  //       if ((flow->GetBearer ()->GetApplication ()->GetApplicationType ()
  //             == Application::APPLICATION_TYPE_TRACE_BASED)
  //           ||
  //           (flow->GetBearer ()->GetApplication ()->GetApplicationType ()
  //             == Application::APPLICATION_TYPE_VOIP))
  //         {
  //           avgHOL += flow->GetBearer ()->GetHeadOfLinePacketDelay ();
  //           nbFlows++;
  //         }
  //     }
  // }
  // double m_avgHOLDelayes = avgHOL/nbFlows;

  QoSParameters *qos = bearer->GetQoSParameters ();
  double HOL = bearer->GetHeadOfLinePacketDelay ();
  double targetDelay = qos->GetMaxDelay ();

  //COMPUTE METRIC USING EXP RULE:
  double numerator = (6/targetDelay) * HOL;
  double denominator = (1 + sqrt (m_avgHOLDelayes));

  return exp(numerator / denominator);
}

double DQN_PacketScheduler::ComputeEXP(RadioBearer *bearer)
{
  //compute AW~
  //FlowsToSchedule *flowsToSchedule = GetFlowsToSchedule ();
  //FlowsToSchedule::iterator iter;
  //FlowToSchedule *flow;

  double AW_avgAW;
  //double m_aW = 0;
  //int nbFlow = 0;

  // compute AW
  // for (auto flow : *GetFlowsToSchedule())
  // {
  //   RadioBearer *bearer = flow->GetBearer ();

  //   if (bearer->HasPackets ())
  //     {
  //       if ((bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_TRACE_BASED)
  //           ||
  //           (bearer->GetApplication ()->GetApplicationType () == Application::APPLICATION_TYPE_VOIP))
  //         {
  //           QoSForEXP *qos = (QoSForEXP*) bearer->GetQoSParameters ();
  //           double aWi =  - (log10 (qos->GetDropProbability())
  //                             /
  //                             qos->GetMaxDelay ());
  //           double HOL = bearer->GetHeadOfLinePacketDelay ();
  //           aWi = aWi * HOL;
  //           m_aW += aWi;
  //           nbFlow++;
  //         }
  //     }
  // }
  // m_aW = m_aW/nbFlow;
  // if (m_aW < 0.000001) m_aW=0;
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


double DQN_PacketScheduler::ComputeLOG(RadioBearer *bearer)
{
  QoSParameters *qos = bearer->GetQoSParameters ();
  double HOL = bearer->GetHeadOfLinePacketDelay ();
  double targetDelay = qos->GetMaxDelay ();

  return log (1.1 + ( (5 * HOL) / targetDelay ));
}


double DQN_PacketScheduler::ComputeMLWDF(RadioBearer *bearer)
{
  QoSForM_LWDF *qos = (QoSForM_LWDF*) bearer->GetQoSParameters ();

  double a = (-log10 (qos->GetDropProbability())) / qos->GetMaxDelay ();
  double HOL = bearer->GetHeadOfLinePacketDelay ();

  return (a * HOL);
}