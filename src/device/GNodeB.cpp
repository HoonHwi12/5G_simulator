/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 TELEMATICS LAB, Politecnico di Bari
 *
 * This file is part of 5G-air-simulator
 *
 * 5G-air-simulator is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation;
 *
 * 5G-air-simulator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 5G-air-simulator; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Telematics Lab <telematics-dev@poliba.it>
 * Author: Sergio Martiradonna <sergio.martiradonna@poliba.it>
 */


#include "NetworkNode.h"
#include "UserEquipment.h"
#include "GNodeB.h"
#include "Gateway.h"
#include "../protocolStack/mac/packet-scheduler/packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/mt-uplink-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/dl-pf-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/dl-mt-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/dl-mlwdf-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/dl-exp-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/dl-fls-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/exp-rule-downlink-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/log-rule-downlink-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/dl-rr-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/enhanced-uplink-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/roundrobin-uplink-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/nb-roundrobin-uplink-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/nb-fifo-uplink-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/nb-uplink-packet-scheduler.h"
#include "../phy/gnb-phy.h"
#include "../core/spectrum/bandwidth-manager.h"
#include "../protocolStack/packet/packet-burst.h"
#include "../protocolStack/mac/harq-manager.h"
#include "../componentManagers/FrameManager.h"
#include "../protocolStack/mac/random-access/gnb-random-access.h"

//HH
#include "../protocolStack/mac/packet-scheduler/dqn-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/dqn-packet-scheduler_mlwdf.h"
#include "../protocolStack/mac/packet-scheduler/dqn-packet-scheduler_select.h"

#include "../core/eventScheduler/simulator.h"
#include "../componentManagers/FlowsManager.h"
// cout stream buffer redirect
#include <sstream>
#include <cstdio>
#include <string>
#include <cstring>
// pipe server
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <fstream>

#define STATE_FIFO "state_fifo"
#define SCHE_FIFO "sched_fifo"
#define CQI_FIFO   "cqi_fifo"

extern double d_dqn_output0;
extern double d_dqn_output1;
extern double d_dqn_output2;
extern double d_dqn_output3;


GNodeB::GNodeB (int idElement,
                Cell *cell)
    : GNodeB::GNodeB (idElement,
                      cell,
                      cell->GetCellCenterPosition ()->GetCoordinateX (),
                      cell->GetCellCenterPosition ()->GetCoordinateY (),
                      25) // default value for urban macro-cell scenario
{}

GNodeB::GNodeB (int idElement,
                Cell *cell,
                double posx,
                double posy)
    : GNodeB::GNodeB (idElement,
                      cell,
                      posx,
                      posy,
                      25) // default value for urban macro-cell scenario
{}

GNodeB::GNodeB (int idElement,
                Cell *cell,
                double posx,
                double posy,
                double posz)
{
  SetIDNetworkNode (idElement);
  SetNodeType(NetworkNode::TYPE_GNODEB);
  SetCell (cell);

  CartesianCoordinates *position = new CartesianCoordinates(posx, posy, posz);
  Mobility* m = new ConstantPosition ();
  m->SetAbsolutePosition (position);
  SetMobilityModel (m);
  delete position;

  m_userEquipmentRecords = new UserEquipmentRecords;

  GnbPhy *phy = new GnbPhy ();
  phy->SetDevice(this);
  SetPhy (phy);

  ProtocolStack *stack = new ProtocolStack (this);
  SetProtocolStack (stack);

  Classifier *classifier = new Classifier ();
  classifier->SetDevice (this);
  SetClassifier (classifier);
}

GNodeB::~GNodeB()
{
  Destroy ();
  m_userEquipmentRecords->clear();
  delete m_userEquipmentRecords;
}

void
GNodeB::RegisterUserEquipment (UserEquipment *UE)
{
  UserEquipmentRecord *record = new UserEquipmentRecord (UE);
  UserEquipmentRecords *UERs = GetUserEquipmentRecords ();
  UserEquipmentRecords::iterator it, ins_pos;
  ins_pos = UERs->end();
  if (UERs->size()>0)
    {
      for (it=UERs->end()-1; it>=UERs->begin(); it--)
        {
          // UEs should be inserted before multicast destinations
          if ( it == UERs->begin() )
            {
              ins_pos = it;
              break;
            }
          if ( (*it)->GetUE()->GetNodeType()==NetworkNode::TYPE_MULTICAST_DESTINATION )
            {
              ins_pos = it;
            }
          else
            {
              break;
            }
        }
    }

  UERs->insert(ins_pos,record);
  UE->SetTargetNodeRecord (record);
}

void
GNodeB::DeleteUserEquipment (UserEquipment *UE)
{
  UserEquipmentRecords *new_records = new UserEquipmentRecords ();

  for (auto record : *GetUserEquipmentRecords())
    {
      if (record->GetUE ()->GetIDNetworkNode () != UE->GetIDNetworkNode ())
        {
          //records->erase(iter);
          //break;
          new_records->push_back (record);
        }
      else
        {
          if (UE->GetTargetNodeRecord() == record)
            {
              UE->SetTargetNodeRecord (NULL);
            }
          delete record;
        }
    }

  m_userEquipmentRecords->clear ();
  delete m_userEquipmentRecords;
  m_userEquipmentRecords = new_records;
}

int
GNodeB::GetNbOfUserEquipmentRecords (void)
{
  return GetUserEquipmentRecords ()->size();
}

void
GNodeB::CreateUserEquipmentRecords (void)
{
  m_userEquipmentRecords = new UserEquipmentRecords ();
}

void
GNodeB::DeleteUserEquipmentRecords (void)
{
  m_userEquipmentRecords->clear ();
  delete m_userEquipmentRecords;
}

GNodeB::UserEquipmentRecords*
GNodeB::GetUserEquipmentRecords (void)
{
  return m_userEquipmentRecords;
}

GNodeB::UserEquipmentRecord*
GNodeB::GetUserEquipmentRecord (int idUE)
{
  for (auto record : *GetUserEquipmentRecords())
    {
      if (record->GetUE ()->
          GetIDNetworkNode () == idUE)
        {
          return record;
        }
    }
  return nullptr;
}

GnbMacEntity*
GNodeB::GetMacEntity(void) const
{
  return (GnbMacEntity*)GetProtocolStack()->GetMacEntity();
}

GNodeB::UserEquipmentRecord::UserEquipmentRecord ()
{
  m_UE = nullptr;
  //Create initial CQI values:
  m_cqiAvailable = false;
  m_cqiFeedback.clear ();
  m_uplinkChannelStatusIndicator.clear ();
  m_schedulingRequest = 0;
  m_averageSchedulingGrants = 1;
  if (_harq_active_)
    SetHarqManager (new HarqManager ());
  else
    SetHarqManager (nullptr);
}

GNodeB::UserEquipmentRecord::~UserEquipmentRecord ()
{
  m_cqiFeedback.clear ();
  m_uplinkChannelStatusIndicator.clear();
  if (m_harqManager != nullptr)
    {
      delete m_harqManager;
    }
}

GNodeB::UserEquipmentRecord::UserEquipmentRecord (UserEquipment *UE)
{
  m_UE = UE;
  BandwidthManager *s = m_UE->GetPhy ()->GetBandwidthManager ();

  int nbRbs = (int) s->GetDlSubChannels ().size ();
  m_cqiFeedback.clear ();
  for (int i = 0; i < nbRbs; i++ )
    {
      m_cqiFeedback.push_back (10);
    }

  nbRbs = (int) s->GetUlSubChannels ().size ();
  m_uplinkChannelStatusIndicator.clear ();
  for (int i = 0; i < nbRbs; i++ )
    {
      m_uplinkChannelStatusIndicator.push_back (10.);
    }

  m_schedulingRequest = 0;
  m_averageSchedulingGrants = 1;
  m_DlTxMode = UE->GetTargetNode()->GetMacEntity ()->GetDefaultDlTxMode ();
  if (UE->GetNodeType() == NetworkNode::TYPE_MULTICAST_DESTINATION
      || !_harq_active_)
    {
      SetHarqManager (nullptr);
    }
  else
    {
      SetHarqManager (new HarqManager (UE));
    }
  if (FrameManager::Init()->MbsfnEnabled()==true && UE->GetNodeType()==NetworkNode::TYPE_MULTICAST_DESTINATION)
    {
      m_DlTxMode = 1;
    }
}

void
GNodeB::UserEquipmentRecord::SetUE (UserEquipment *UE)
{
  m_UE = UE;
}

UserEquipment*
GNodeB::UserEquipmentRecord::GetUE (void) const
{
  return m_UE;
}

bool
GNodeB::UserEquipmentRecord::CqiAvailable()
{
  return m_cqiAvailable;
}

void
GNodeB::UserEquipmentRecord::SetCQI (vector<int> cqi)
{
  m_cqiAvailable = true;
  m_cqiFeedback = cqi;
}

vector<int>
GNodeB::UserEquipmentRecord::GetCQI (void) const
{
  return m_cqiFeedback;
}

void
GNodeB::UserEquipmentRecord::SetRI(int ri)
{
  m_riFeedback = ri;
}

int
GNodeB::UserEquipmentRecord::GetRI (void) const
{
  return m_riFeedback;
}

void
GNodeB::UserEquipmentRecord::SetPMI(vector< vector<int> > pmi)
{
  m_pmiFeedback = pmi;
}

vector< vector<int> >
GNodeB::UserEquipmentRecord::GetPMI (void) const
{
  return m_pmiFeedback;
}

void
GNodeB::UserEquipmentRecord::SetChannelMatrix(vector< shared_ptr<arma::cx_fmat> > channelMatrix)
{
  m_channelMatrix = channelMatrix;
}

vector< shared_ptr<arma::cx_fmat> >
GNodeB::UserEquipmentRecord::GetChannelMatrix(void)
{
  return m_channelMatrix;
}


void
GNodeB::UserEquipmentRecord::SetDlTxMode(int txMode)
{
  m_DlTxMode = txMode;
}

int
GNodeB::UserEquipmentRecord::GetDlTxMode()
{
  return m_DlTxMode;
}

void
GNodeB::UserEquipmentRecord::SetHarqManager (HarqManager* harqManager)
{
  m_harqManager = harqManager;
}

HarqManager*
GNodeB::UserEquipmentRecord::GetHarqManager (void)
{
  return m_harqManager;
}

int
GNodeB::UserEquipmentRecord::GetSchedulingRequest (void)
{
  return m_schedulingRequest;
}

void
GNodeB::UserEquipmentRecord::SetSchedulingRequest (int r)
{
  m_schedulingRequest = r;
}

void
GNodeB::UserEquipmentRecord::UpdateSchedulingGrants (int b)
{
  m_averageSchedulingGrants = (0.9 * m_averageSchedulingGrants) + (0.1 * b);
}

int
GNodeB::UserEquipmentRecord::GetSchedulingGrants (void)
{
  return m_averageSchedulingGrants;
}

void
GNodeB::UserEquipmentRecord::SetUlMcs (int mcs)
{
  m_ulMcs = mcs;
}

int
GNodeB::UserEquipmentRecord::GetUlMcs (void)
{
  return m_ulMcs;
}

void
GNodeB::UserEquipmentRecord::SetUplinkChannelStatusIndicator (vector<double> vet)
{
  m_uplinkChannelStatusIndicator = vet;
}

vector<double>
GNodeB::UserEquipmentRecord::GetUplinkChannelStatusIndicator (void) const
{
  return m_uplinkChannelStatusIndicator;
}

void
GNodeB::SetDLScheduler (Simulator::SchedulerType type)
{
  GnbMacEntity *mac = GetMacEntity ();
  DownlinkPacketScheduler *scheduler;

  switch (type)
    {
    case Simulator::Scheduler_TYPE_DQN:
    	scheduler = new  DQN_PacketScheduler ();
    	scheduler->SetMacEntity (mac);
    	mac->SetDownlinkPacketScheduler (scheduler);      
      break;

    case Simulator::Scheduler_TYPE_PROPORTIONAL_FAIR:
      printf("SET SCHEDULER TO PF\n");
      scheduler = new  DL_PF_PacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    case Simulator::Scheduler_TYPE_FLS:
      scheduler = new  DL_FLS_PacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    case Simulator::Scheduler_TYPE_EXP:
      scheduler = new  DL_EXP_PacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    case Simulator::Scheduler_TYPE_MLWDF:
      scheduler = new  DL_MLWDF_PacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    case Simulator::Scheduler_EXP_RULE:
      scheduler = new  ExpRuleDownlinkPacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    case Simulator::Scheduler_LOG_RULE:
      scheduler = new  LogRuleDownlinkPacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    case Simulator::Scheduler_TYPE_MAXIMUM_THROUGHPUT:
      scheduler = new  DL_MT_PacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    case Simulator::Scheduler_TYPE_ROUND_ROBIN:
      scheduler = new  DL_RR_PacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    default:
      printf("ERROR: invalid scheduler type\n");
      exit(1);
      break;
    }
}

DownlinkPacketScheduler*
GNodeB::GetDLScheduler (void) const
{
  return GetMacEntity ()->GetDownlinkPacketScheduler ();
}

void
GNodeB::SetULScheduler (ULSchedulerType type)
{
  GnbMacEntity *mac = GetMacEntity ();
  UplinkPacketScheduler *scheduler;
  switch (type)
    {
    case GNodeB::ULScheduler_TYPE_MAXIMUM_THROUGHPUT:
      scheduler = new MaximumThroughputUplinkPacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetUplinkPacketScheduler (scheduler);
      break;
    case GNodeB::ULScheduler_TYPE_FME:
      scheduler = new EnhancedUplinkPacketScheduler();
      scheduler->SetMacEntity (mac);
      mac->SetUplinkPacketScheduler (scheduler);
      break;
    case GNodeB::ULScheduler_TYPE_ROUNDROBIN:
      scheduler = new RoundRobinUplinkPacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetUplinkPacketScheduler (scheduler);
      break;
    case GNodeB::ULScheduler_TYPE_NB_IOT_FIFO:
      scheduler = new nbFifoUplinkPacketScheduler (mac);
      mac->SetUplinkPacketScheduler (scheduler);
      break;
    case GNodeB::ULScheduler_TYPE_NB_IOT_ROUNDROBIN:
      scheduler = new nbRoundRobinUplinkPacketScheduler (mac);
      mac->SetUplinkPacketScheduler (scheduler);
      break;

    default:
      cout << "ERROR: invalid scheduler type" << endl;
      exit(1);
    }
}

UplinkPacketScheduler*
GNodeB::GetULScheduler (void) const
{
  return GetMacEntity ()->GetUplinkPacketScheduler ();
}

void
GNodeB::ResourceBlocksAllocation (void)
{
  DownlinkResourceBlockAllocation ();
  UplinkResourceBlockAllocation ();
}

void
GNodeB::UplinkResourceBlockAllocation (void)
{
  if (GetULScheduler () != nullptr && GetNbOfUserEquipmentRecords () > 0)
    {
      GetULScheduler ()->Schedule();
    }
}

void
GNodeB::DownlinkResourceBlockAllocation (void)
{
  if (GetDLScheduler () != nullptr && GetNbOfUserEquipmentRecords () > 0)
    {
      GetDLScheduler ()->Schedule();
    }
  else
    {
      //send only reference symbols
      //PacketBurst *pb = new PacketBurst ();
      //SendPacketBurst (pb);
    }
}

void
GNodeB::SetRandomAccessType(GnbRandomAccess::RandomAccessType type)
{
  GetMacEntity ()->SetRandomAccessType(type);
}


//Debug
void
GNodeB::Print (void)
{
  cout << " GNodeB object:"
            "\n\t m_idNetworkNode = " << GetIDNetworkNode () <<
            "\n\t m_idCell = " << GetCell ()->GetIdCell () <<
            "\n\t Served Users: " <<
            endl;

  for (auto record : *GetUserEquipmentRecords())
    {
      cout << "\t\t idUE = " << record->GetUE ()->
                GetIDNetworkNode () << endl;
    }
}



// //by HH: DQN
// void GNodeB::NumberToString(double number, std::string *target){
//   std::stringstream ss;
//   ss << number;
//   *target = ss.str();
// }

// void GNodeB::NumberToString(int number, std::string *target){
//   std::stringstream ss;
//   ss << number;
//   *target = ss.str();
// }

// void GNodeB::OpenSchedulerFifo(int *fd){
//   mkfifo(SCHE_FIFO, S_IFIFO|0640);
//   printf("LTESIM: SCHED Pipe Opened! Waiting for DQN..\n");
//   *fd = open(SCHE_FIFO, O_RDONLY);
//   close(*fd);
//   printf("LTESIM: DQN connected to SCHED Pipe.\n");
// }

// void GNodeB::ConnectCQIFifo(int *fd){
//   printf("LTESIM: Waiting for CQI_FIFO.\n");
//   *fd = open(CQI_FIFO, O_CREAT|O_WRONLY);
//   printf("LTESIM: Connected to CQI_FIFO.\n");
//   close(*fd);
// }

// void GNodeB::ConnectStateFifo(int *fd){
//   int noUEs = 0;
//   std::vector<GNodeB*> *gNBs = NetworkManager::Init()->GetGNodeBContainer ();
//   for (std::vector<GNodeB*>::iterator it = gNBs->begin(); it != gNBs->end(); ++it){
//     noUEs += (*it)->GetNbOfUserEquipmentRecords();
//     printf("noUES: %d\n", noUEs);
//   }
//   char noUEs_send[256]="\0";
//   printf("INSERTED noUES: %d\n", noUEs);
//   snprintf (noUEs_send, sizeof(noUEs), "%d", noUEs);
//   printf("noUES SEND: %d\n", noUEs_send);

//   printf("LTESIM: Waiting for STATE_FIFO.\n");
//   *fd = open(STATE_FIFO, O_CREAT|O_WRONLY);
//   printf("LTESIM: Connected to STATE_FIFO, Sending #UEs\n");
//    // get the total number of UEs;
//    // send size of message
//   write(*fd, noUEs_send, strlen(noUEs_send));
//   close(*fd);
//   printf("LTESIM: Sent #UEs: \"%s\\n", noUEs_send);

// }

// bool GNodeB::makeUEsStationary(){  
//   std::vector<UserEquipment*>* UEs = NetworkManager::Init()->GetUserEquipmentContainer();
//   UserEquipment* this_ue;
//   for (std::vector<UserEquipment*>::iterator it = UEs->begin(); it != UEs->end(); ++it){
//     this_ue = (*it);
//     this_ue->GetMobilityModel()->SetSpeed(0);
//   }

// }

// Simulator::SchedulerType GNodeB::FetchScheduler(int *fd){
//   char c_readbuf[80];
//   char *c_read_ptr;
//   int i_input_bytes;
//   Simulator::SchedulerType downlink_scheduler_type;

//   *fd = open(SCHE_FIFO, O_RDONLY);
//   i_input_bytes = read(*fd, c_readbuf, sizeof(c_readbuf));
//   close(*fd);

//   c_read_ptr = strtok(c_readbuf, "|");
//   d_dqn_output0 = 10*atoi(c_read_ptr);
//   c_read_ptr = strtok(NULL,"|");  
//   d_dqn_output1 = 10*atoi(c_read_ptr);
//   c_read_ptr = strtok(NULL,"|");  
//   d_dqn_output2 = 10*atoi(c_read_ptr);
//   c_read_ptr = strtok(NULL,"|");  
//   d_dqn_output3 = 10*atoi(c_read_ptr);

//   c_readbuf[i_input_bytes] = '\0';
// //  printf("LTESIM: Received scheduler: \"%f / %f / %f / %f\"\n", d_dqn_output0, d_dqn_output1, d_dqn_output2, d_dqn_output3);

//   if (strcmp(c_readbuf, "end") == 0)
//   {
//     Simulator *sim;
//     sim->m_stop = true;
//     downlink_scheduler_type = Simulator::Scheduler_TYPE_PROPORTIONAL_FAIR;
//     printf("LTESIM: Scheduler is PF_Fair.\n");
//     return downlink_scheduler_type;
//   }

//   if(d_dqn_output0 >= 0)
//   {
//         downlink_scheduler_type = Simulator::Scheduler_TYPE_DQN;
//         printf("LTESIM: Scheduler is DQN\n");
//   }
//   else
//   {
//     switch ((int)d_dqn_output1/10)
//       {
//         case 0:
//           downlink_scheduler_type = Simulator::Scheduler_TYPE_PROPORTIONAL_FAIR;
//           printf("LTESIM: Scheduler is PF_Fair.\n");
//           break;
//         case 1:
//           downlink_scheduler_type = Simulator::Scheduler_TYPE_MLWDF;
//           printf("LTESIM: Scheduler is MLWDF.\n");
//           break;
//         case 2:
//           downlink_scheduler_type = Simulator::Scheduler_TYPE_EXP;
//           printf("LTESIM: Scheduler is EXP.\n");
//           break;
//         case 3:
//           downlink_scheduler_type = Simulator::Scheduler_TYPE_FLS;
//           printf("LTESIM: Scheduler is FLS.\n");
//           break;
//         case 4:
//           downlink_scheduler_type = Simulator::Scheduler_EXP_RULE;
//           printf("LTESIM: Scheduler is EXP_RULE.\n");
//           break;
//         case 5:
//           downlink_scheduler_type = Simulator::Scheduler_LOG_RULE;
//           printf("LTESIM: Scheduler is LOG_RULE.\n");
//           break;
//         case 11:
//           downlink_scheduler_type = Simulator::Scheduler_TYPE_PROPORTIONAL_FAIR;
//           printf("LTESIM: SETTING UEs stationary.\n");
//           makeUEsStationary();
//           break;
//         default:
//           downlink_scheduler_type = Simulator::Scheduler_TYPE_PROPORTIONAL_FAIR;
//           break;
//       }
//   }

//     return downlink_scheduler_type;
// }

// void GNodeB::SendUESummary(int *fd){
//   std::vector<GNodeB*> *gNBs = NetworkManager::Init()->GetGNodeBContainer ();
//   // target message string
//   std::string UEsummaries;
//   for (std::vector<GNodeB*>::iterator it = gNBs->begin(); it != gNBs->end(); ++it){
//     // form UE summary of this eNB
//     FormUESummaryMessage(*it, &UEsummaries);
//   }
//   std::string::size_type size = UEsummaries.size();
//   printf("LTESIM: Size of UEsummaries: %d \n", (int)size);
//   *fd = open(STATE_FIFO, O_CREAT|O_WRONLY);
//   // send the cqi size
//   write(*fd, &size, sizeof(size));
//   // then the whole message
//   write(*fd, UEsummaries.c_str(), UEsummaries.size());
//   printf("LTESIM: Sent UEsummaries.\n");
//   close(*fd);
// }

// void  GNodeB::SendCQISummary(int *fd){
//   std::vector<GNodeB*> *gNBs = NetworkManager::Init()->GetGNodeBContainer ();
//   // cqi string to send
//   std::string CQIs;
//   for (std::vector<GNodeB*>::iterator it = gNBs->begin(); it != gNBs->end(); ++it){
//     // form cqi message of this eNB
//     FormCQIMessage(*it, &CQIs);
//   }

//   std::string::size_type size = CQIs.size();
//   printf("LTESIM: Size of cqis: %d \n", (int)size);
//   *fd = open(CQI_FIFO, O_CREAT|O_WRONLY);
//   // send the cqi size
//   write(*fd, &size, sizeof(size));
//   // then the whole message
//   write(*fd, CQIs.c_str(), CQIs.size());
//   // printf("LTESIM: Sent cqis.\n%s\n", CQIs.c_str());
//   printf("LTESIM: Sent cqis.\n");
//   close(*fd);
// }

// void GNodeB::SendState(int *fd, std::string state){
//   std::string::size_type size = state.size();
//   printf("LTESIM: Size of state: %d \n", (int)size);
//   *fd = open(STATE_FIFO, O_CREAT|O_WRONLY);
//   // send the size of message
//   write(*fd, &size, sizeof(size));
//   //  then the whole message
//   write(*fd, state.c_str(), state.size());
//   // printf("LTESIM: Sent state.\n%s\n", state.c_str());
//   printf("LTESIM: Sent state.\n");
//   close(*fd);
// }

// void GNodeB::FormUESummaryMessage(GNodeB *eNB, std::string *target_string){
//   int ideNB;
//   int UEid;
//   std::string UEid_str;
//   // collect all UE information
//   int noUEs = eNB->GetNbOfUserEquipmentRecords();
//   std::vector<GNodeB::UserEquipmentRecord*> *UErecords = eNB->GetUserEquipmentRecords();
//   GNodeB::UserEquipmentRecord* UErecord;
//   // collect all APP information
//   std::vector<Application*> *apps = FlowsManager::Init()->GetApplicationContainer();
//   Application* app;
//   int appID, appDST;
//   std::string appID_str;
//   //all QoS information
//   QoSParameters* appQoS;

//   double appGBR, appDelay, appPLR;
//   std::string appGBR_str, appDelay_str, appPLR_str;
//   // message string used for each Application
//   std::string message_string;
//   for (std::vector<GNodeB::UserEquipmentRecord*>::iterator it = UErecords->begin(); it != UErecords->end(); ++it){
//     UErecord = (*it);
//     UEid = UErecord->GetUE()->GetIDNetworkNode();
//     NumberToString(UEid, &UEid_str);
//     // for each app check which app belongs to which UE
//     for (std::vector<Application*>::iterator it = apps->begin(); it != apps->end(); ++it){
//       app = (*it);
//       appDST = app->GetDestination()->GetIDNetworkNode();
//       if(appDST == UEid){ // app belongs to this UE
//         appID = app->GetApplicationID();
//         NumberToString(appID, &appID_str);
//         // QoS Fetch
//         appQoS = app->GetQoSParameters();
//         appGBR = 0; //appQoS->GetGBR(); HH: need to fix
//         appDelay = 0; //appQoS->GetMaxDelay();
//         appPLR = 0; //appQoS->GetDropProbability();

//         // QoS to strings
//         NumberToString(appGBR, &appGBR_str);
//         NumberToString(appDelay, &appDelay_str);
//         NumberToString(appPLR, &appPLR_str);

//         // add to the message string
//         // {UE id} {APP id} {APP GBR} {APP delay} {APP PLR}
//         *target_string += UEid_str + " " + appID_str + " " + appGBR_str + " " + appDelay_str + " " + appPLR_str + "\n";
//       }
//     }
//   }
// }

// void GNodeB::UpdateAllScheduler(Simulator::SchedulerType new_scheduler){
//   // vector of gNBs
//   std::vector<GNodeB*> *gNBs = NetworkManager::Init()->GetGNodeBContainer ();
//   // vector of Apps
//   std::vector<Application*> *apps = FlowsManager::Init()->GetApplicationContainer();
//   // vector of rrc bearers
//   std::vector<RadioBearer*> *bearers;
//   // QoS for changing rrc bearers
//   QoSParameters* new_qos;
//   // Update the QoS for each Application AND it's bearer based on scheduler
//   printf("here7\n");
//   for (std::vector<Application*>::iterator it = apps->begin(); it != apps->end(); ++it){
//     new_qos = (*it)->UpdateQoSOnSchedulerChange(new_scheduler);
//   }
//   printf("here8\n");
//   // for each enobeB
//   for (std::vector<GNodeB*>::iterator it = gNBs->begin(); it != gNBs->end(); ++it){
//     printf("here12\n");
//     // Update Scheduler 
//     (*it)->SetDLScheduler(new_scheduler);
//     printf("here13\n");
//     // get the bearer list from this GNodeB's rrc 
//     bearers = (*it)->GetDLScheduler()->GetMacEntity()->GetDevice()->GetProtocolStack()->GetRrcEntity()->GetRadioBearerContainer();
//     // update each bearer in the rrc's list QoS based on scheduler
//     for (std::vector<RadioBearer*>::iterator it = bearers->begin(); it != bearers->end(); ++it){        
//       // (*it)->SetQoSParameters(new_qos); HH: need to fix
//     }
//     printf("here10\n");
//   }
//   printf("here11\n");
// }

// void GNodeB::FormCQIMessage(GNodeB *eNB, std::string *target_string){
//     std::vector<GNodeB::UserEquipmentRecord*> *UserEquipmentRecords = eNB->GetUserEquipmentRecords();
//     GNodeB::UserEquipmentRecord* UErecord;
//     // cqi vector for each UE
//     std::vector<int> cqi;
//     // cqi string for each UE
//     std::string cqiString;
//     // temp string
//     std::string temp;
//     for (std::vector<GNodeB::UserEquipmentRecord*>::iterator it = UserEquipmentRecords->begin(); it != UserEquipmentRecords->end(); ++it){
//       UErecord = (*it);
//       cqi = UErecord->GetCQI();
//       for (size_t i = 0; i < cqi.size(); ++i){
//         NumberToString(cqi[i], &temp);
//         cqiString += temp + " ";
//       }
//       *target_string += cqiString + "\n";
//       cqiString = "";
     
//     }
// }