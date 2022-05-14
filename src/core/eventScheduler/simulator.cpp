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
 */



#include "simulator.h"
#include "make-event.h"
#include "../../componentManagers/FrameManager.h"

#include <math.h>
#include <fstream>
#include <list>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

// by HH
#include "../../../src/shared-memory.h"
#include "../../protocolStack/mac/packet-scheduler/downlink-packet-scheduler.h"
#include "../../device/UserEquipment.h"
#include "../../flows/QoS/QoSParameters.h"
#include "../../flows/radio-bearer.h"

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

//by HH
#include "../../componentManagers/FlowsManager.h"

Simulator* Simulator::ptr=nullptr;

#define STATE_FIFO "state_fifo"
#define SCHE_FIFO "sched_fifo"
#define CQI_FIFO   "cqi_fifo"

double d_dqn_output0, d_dqn_output1, d_dqn_output2, d_dqn_output3; 

Simulator::Simulator ()
{
  m_stop = false;
  m_currentUid = 0;
  m_currentTs = 0;
  m_unscheduledEvents = 0;
  m_calendar = new Calendar;
  m_uid = 0;
  m_lastAssignedUid = 0;
}

Simulator::~Simulator ()
{
  while (!m_calendar->IsEmpty ())
    {
      m_calendar->RemoveEvent ();
    }
  delete m_calendar;
}

double
Simulator::Now (void)
{
  return m_currentTs;
}

//by HH: DQN
void Simulator::NumberToString(double number, std::string *target){
  std::stringstream ss;
  ss << number;
  *target = ss.str();
}

void Simulator::NumberToString(int number, std::string *target){
  std::stringstream ss;
  ss << number;
  *target = ss.str();
}

void Simulator::OpenSchedulerFifo(int *fd){
  mkfifo(SCHE_FIFO, S_IFIFO|0640);
  printf("LTESIM: SCHED Pipe Opened! Waiting for DQN..\n");
  *fd = open(SCHE_FIFO, O_RDONLY);
  close(*fd);
  printf("LTESIM: DQN connected to SCHED Pipe.\n");
}

void Simulator::ConnectCQIFifo(int *fd){
  printf("LTESIM: Waiting for CQI_FIFO.\n");
  *fd = open(CQI_FIFO, O_CREAT|O_WRONLY);
  printf("LTESIM: Connected to CQI_FIFO.\n");
  close(*fd);
}

void Simulator::ConnectStateFifo(int *fd){
  int noUEs = 0;
  std::vector<GNodeB*> *gNBs = NetworkManager::Init()->GetGNodeBContainer ();
  for (std::vector<GNodeB*>::iterator it = gNBs->begin(); it != gNBs->end(); ++it){
    noUEs += (*it)->GetNbOfUserEquipmentRecords();
  }
  char noUEs_send[256]="\0";

  do{
    snprintf (noUEs_send, sizeof(noUEs_send), "%d", noUEs);
  }while( atoi(noUEs_send) > 10000 || atoi(noUEs_send) < 0 );

  printf("LTESIM: Waiting for STATE_FIFO.\n");
  *fd = open(STATE_FIFO, O_CREAT|O_WRONLY);
  printf("LTESIM: Connected to STATE_FIFO, Sending #UEs\n");
   // get the total number of UEs;
   // send size of message
  write(*fd, noUEs_send, strlen(noUEs_send));
  close(*fd);
  printf("LTESIM: Sent #UEs: \"%s\\n", noUEs_send);

}

// bool Simulator::makeUEsStationary(){  
//   std::vector<UserEquipment*>* UEs = NetworkManager::Init()->GetUserEquipmentContainer();
//   UserEquipment* this_ue;
//   for (std::vector<UserEquipment*>::iterator it = UEs->begin(); it != UEs->end(); ++it){
//     this_ue = (*it);
//     this_ue->GetMobilityModel()->SetSpeed(0);
//   }
// }

Simulator::SchedulerType Simulator::FetchScheduler(int *fd){
  char c_readbuf[80];
  char *c_read_ptr;
  int i_input_bytes;
  Simulator::SchedulerType downlink_scheduler_type;

  *fd = open(SCHE_FIFO, O_RDONLY);
  i_input_bytes = read(*fd, c_readbuf, sizeof(c_readbuf));
  close(*fd);

  c_read_ptr = strtok(c_readbuf, "|");
  d_dqn_output0 = 10*atoi(c_read_ptr);
  c_read_ptr = strtok(NULL,"|");  
  d_dqn_output1 = 10*atoi(c_read_ptr);
  c_read_ptr = strtok(NULL,"|");  
  d_dqn_output2 = 10*atoi(c_read_ptr);
  c_read_ptr = strtok(NULL,"|");  
  d_dqn_output3 = 10*atoi(c_read_ptr);

  c_readbuf[i_input_bytes] = '\0';
//  printf("LTESIM: Received scheduler: \"%f / %f / %f / %f\"\n", d_dqn_output0, d_dqn_output1, d_dqn_output2, d_dqn_output3);

  if (strcmp(c_readbuf, "end") == 0)
  {
    Simulator *sim;
    sim->m_stop = true;
    downlink_scheduler_type = Simulator::Scheduler_TYPE_PROPORTIONAL_FAIR;
    printf("5G_SIM: Scheduler is PF_Fair.\n");
    return downlink_scheduler_type;
  }

  if(d_dqn_output0 >= 0)
  {
        downlink_scheduler_type = Simulator::Scheduler_TYPE_DQN;
        printf("5G_SIM: Scheduler is DQN\n");
  }
  else
  {
    switch ((int)d_dqn_output1/10)
      {
        case 0:
          downlink_scheduler_type = Simulator::Scheduler_TYPE_PROPORTIONAL_FAIR;
          printf("5G_SIM: Scheduler is PF_Fair.\n");
          break;
        case 1:
          downlink_scheduler_type = Simulator::Scheduler_TYPE_MLWDF;
          printf("5G_SIM: Scheduler is MLWDF.\n");
          break;
        case 2:
          downlink_scheduler_type = Simulator::Scheduler_TYPE_EXP;
          printf("5G_SIM: Scheduler is EXP.\n");
          break;
        case 3:
          downlink_scheduler_type = Simulator::Scheduler_TYPE_FLS;
          printf("5G_SIM: Scheduler is FLS.\n");
          break;
        case 4:
          downlink_scheduler_type = Simulator::Scheduler_EXP_RULE;
          printf("5G_SIM: Scheduler is EXP_RULE.\n");
          break;
        case 5:
          downlink_scheduler_type = Simulator::Scheduler_LOG_RULE;
          printf("5G_SIM: Scheduler is LOG_RULE.\n");
          break;
        case 11:
          downlink_scheduler_type = Simulator::Scheduler_TYPE_PROPORTIONAL_FAIR;
          printf("5G_SIM: SETTING UEs stationary.\n");
          //makeUEsStationary();
          break;
        default:
          downlink_scheduler_type = Simulator::Scheduler_TYPE_PROPORTIONAL_FAIR;
          break;
      }
  }

    return downlink_scheduler_type;
}

void Simulator::SendUESummary(int *fd){
  std::vector<GNodeB*> *gNBs = NetworkManager::Init()->GetGNodeBContainer ();
  // target message string
  std::string UEsummaries;
  for (std::vector<GNodeB*>::iterator it = gNBs->begin(); it != gNBs->end(); ++it){
    // form UE summary of this eNB
    FormUESummaryMessage(*it, &UEsummaries);
  }
  std::string::size_type size = UEsummaries.size();
  printf("LTESIM: Size of UEsummaries: %d \n", (int)size);
  *fd = open(STATE_FIFO, O_CREAT|O_WRONLY);
  // send the cqi size
  write(*fd, &size, sizeof(size));
  // then the whole message
  write(*fd, UEsummaries.c_str(), UEsummaries.size());
  printf("LTESIM: Sent UEsummaries.\n");
  close(*fd);
}

void  Simulator::SendCQISummary(int *fd){
  std::vector<GNodeB*> *gNBs = NetworkManager::Init()->GetGNodeBContainer ();
  // cqi string to send
  std::string CQIs;
  for (std::vector<GNodeB*>::iterator it = gNBs->begin(); it != gNBs->end(); ++it){
    // form cqi message of this eNB
    FormCQIMessage(*it, &CQIs);
  }

  std::string::size_type size = CQIs.size();
  printf("LTESIM: Size of cqis: %d \n", (int)size);
  *fd = open(CQI_FIFO, O_CREAT|O_WRONLY);
  // send the cqi size
  write(*fd, &size, sizeof(size));
  // then the whole message
  write(*fd, CQIs.c_str(), CQIs.size());
  // printf("LTESIM: Sent cqis.\n%s\n", CQIs.c_str());
  printf("LTESIM: Sent cqis.\n");
  close(*fd);
}

void Simulator::SendState(int *fd, std::string state){
  std::string::size_type size = state.size();
  printf("LTESIM: Size of state: %d \n", (int)size);
  *fd = open(STATE_FIFO, O_CREAT|O_WRONLY);
  // send the size of message
  write(*fd, &size, sizeof(size));
  //  then the whole message
  write(*fd, state.c_str(), state.size());
  // printf("LTESIM: Sent state.\n%s\n", state.c_str());
  printf("LTESIM: Sent state.\n");
  close(*fd);
}

void Simulator::FormUESummaryMessage(GNodeB *eNB, std::string *target_string){
  int ideNB;
  int UEid;
  std::string UEid_str;
  // collect all UE information
  int noUEs = eNB->GetNbOfUserEquipmentRecords();
  std::vector<GNodeB::UserEquipmentRecord*> *UErecords = eNB->GetUserEquipmentRecords();
  GNodeB::UserEquipmentRecord* UErecord;
  // collect all APP information
  std::vector<Application*> *apps = FlowsManager::Init()->GetApplicationContainer();
  Application* app;
  int appID, appDST;
  std::string appID_str;
  //all QoS information
  QoSParameters* appQoS;

  double appGBR, appDelay, appPLR;
  std::string appGBR_str, appDelay_str, appPLR_str;
  // message string used for each Application
  std::string message_string;
  for (std::vector<GNodeB::UserEquipmentRecord*>::iterator it = UErecords->begin(); it != UErecords->end(); ++it){
    UErecord = (*it);
    UEid = (int)UErecord->GetUE()->GetIDNetworkNode();
    NumberToString(UEid, &UEid_str);
    // for each app check which app belongs to which UE
    for (std::vector<Application*>::iterator it = apps->begin(); it != apps->end(); ++it){
      app = (*it);
      appDST = app->GetDestination()->GetIDNetworkNode();
      if(appDST == UEid){ // app belongs to this UE
        appID = app->GetApplicationID();
        NumberToString(appID, &appID_str);
        // QoS Fetch
        appQoS = app->GetQoSParameters();
        appGBR = appQoS->GetGBR(); // HH: need fix 2
        appDelay = 0; //appQoS->GetMaxDelay();
        appPLR = 0; //appQoS->GetDropProbability();

        // QoS to strings
        NumberToString(appGBR, &appGBR_str);
        NumberToString(appDelay, &appDelay_str);
        NumberToString(appPLR, &appPLR_str);

        // add to the message string
        // {UE id} {APP id} {APP GBR} {APP delay} {APP PLR}
        *target_string += UEid_str + " " + appID_str + " " + appGBR_str + " " + appDelay_str + " " + appPLR_str + "\n";
      }
    }
  }
}

void Simulator::UpdateAllScheduler(Simulator::SchedulerType new_scheduler){
  // vector of gNBs
  std::vector<GNodeB*> *gNBs = NetworkManager::Init()->GetGNodeBContainer ();
  // vector of Apps
  std::vector<Application*> *apps = FlowsManager::Init()->GetApplicationContainer();
  // vector of rrc bearers
  std::vector<RadioBearer*> *bearers;
  // QoS for changing rrc bearers
  QoSParameters* new_qos;
  // Update the QoS for each Application AND it's bearer based on scheduler

  for (std::vector<Application*>::iterator it = apps->begin(); it != apps->end(); ++it){
    new_qos = (*it)->UpdateQoSOnSchedulerChange(new_scheduler);
  }
  // for each enobeB
  for (std::vector<GNodeB*>::iterator it = gNBs->begin(); it != gNBs->end(); ++it){
    // Update Scheduler 
    (*it)->SetDLScheduler(new_scheduler);
    // get the bearer list from this GNodeB's rrc 
    bearers = (*it)->GetDLScheduler()->GetMacEntity()->GetDevice()->GetProtocolStack()->GetRrcEntity()->GetRadioBearerContainer();
    // update each bearer in the rrc's list QoS based on scheduler
    for (std::vector<RadioBearer*>::iterator it = bearers->begin(); it != bearers->end(); ++it){        
      (*it)->SetQoSParameters(new_qos); // HH: need fix 1
    }
  }
}

void Simulator::FormCQIMessage(GNodeB *eNB, std::string *target_string){
    std::vector<GNodeB::UserEquipmentRecord*> *UserEquipmentRecords = eNB->GetUserEquipmentRecords();
    GNodeB::UserEquipmentRecord* UErecord;
    // cqi vector for each UE
    std::vector<int> cqi;
    // cqi string for each UE
    std::string cqiString;
    // temp string
    std::string temp;
    for (std::vector<GNodeB::UserEquipmentRecord*>::iterator it = UserEquipmentRecords->begin(); it != UserEquipmentRecords->end(); ++it){
      UErecord = (*it);
      cqi = UErecord->GetCQI();
      for (size_t i = 0; i < cqi.size(); ++i){
        NumberToString(cqi[i], &temp);
        cqiString += temp + " ";
      }
      *target_string += cqiString + "\n";
      cqiString = "";
     
    }
}


void
Simulator::Run (void)
{
  /*
   * This method start the whole simulation
   * The simulation will end when no events are into the
   * calendar list.
   */
  // while (!m_calendar->IsEmpty () && !m_stop)
  //   {
  //     ProcessOneEvent ();
  //   }
 
  // scheduler type object
  Simulator::SchedulerType scheduler;
  // Open, connect to pipes
  int sh_fd, st_fd, cqi_fd;
  OpenSchedulerFifo(&sh_fd);
  // open CQI Fifo
  ConnectCQIFifo(&cqi_fd);
  // connect and forward #UEs
  ConnectStateFifo(&st_fd);
  // form information about each eNB's UE's application QoS and send!
  SendUESummary(&st_fd);



	// by HH
  char *shared_buffer = (char*)malloc(SHARED_SIZE);
  char *lstm_buffer = (char*)malloc(SHARED_SIZE);
  char *dqn_buffer = (char*)malloc(SHARED_SIZE);
  int use_lstm = 0;
  int PID = -1;

  int lte_shmid = SharedMemoryCreate(LTE_KEY);
  int dqn_shmid;
  int lstm_shmid;

  int packet_index;
  int buffer_value=0;
  int index;


  // DQN에서 LSTM 사용여부 확인
  while(use_lstm != -1 && use_lstm != 0X89)
  {     
    dqn_shmid = SharedMemoryInit(DQN_KEY);  
    if(dqn_shmid != -1)
    {
      SharedMemoryRead(dqn_shmid, dqn_buffer);
      use_lstm = atoi(dqn_buffer);
    }
    sleep(0.001);
  }

  // LSTM 대기
  if(use_lstm==0x89)
  {
    // LSTM에 PID 전송
    sprintf(shared_buffer,"%d", getpid());
    if( SharedMemoryWrite(lte_shmid, shared_buffer) == -1 )
    {
      printf("shared memory write failed\n");
    }

    printf("waiting for LSTM\n");
    
    while(buffer_value != atoi(shared_buffer))
    {     
      lstm_shmid = SharedMemoryInit(LSTM_KEY);  
      if(lstm_shmid != -1)
      {
        SharedMemoryRead(lstm_shmid, lstm_buffer);
        buffer_value = atoi(lstm_buffer);
      }
      sleep(0.001);
    }

     // LSTM ready 이후 LSTM에 -1 전송
    sprintf(shared_buffer,"%d", -1);
    if( SharedMemoryWrite(lte_shmid, shared_buffer) == -1 )
    {
      printf("shared memory write failed\n");
    }  
  } 


  // tti trackers
  unsigned long tti_tr1, tti_tr2;
  // buffer contains the simulation output for the last TTI
  // buffer is written to the bigbig and then cleared each TTI
  std::stringstream buffer;
  // cout buffer redirect into the buffer
  std::streambuf * old = std::cout.rdbuf(buffer.rdbuf());
  // bigbuff is the whole trace of the simulation
  // run the simulation until first packets are sent and update tti trackers
  std::string bigbuf; 
  m_stop = false;
  while (!m_calendar->IsEmpty () && !m_stop){
    tti_tr1 = FrameManager::Init()->GetTTICounter();
    ProcessOneEvent ();
    tti_tr2 = FrameManager::Init()->GetTTICounter();
    // after each TTI change
    if(tti_tr1 != tti_tr2){
      printf("\nWaiting for first packets.. LTESIM: TTI Change! Now in TTI # %ld.\n", tti_tr2);
      // append onto big buffer
      bigbuf = bigbuf + buffer.str();
      // if we are past empty TTIs
      if(!bigbuf.empty()){
        printf("bigbuf: %s\n", bigbuf.c_str() );
        // send the update TTI
        SendState(&st_fd, buffer.str().c_str());
        SendCQISummary(&cqi_fd);
        printf("Waiting for first packets..  LTESIM: Waiting for new Scheduler. \n");
        m_stop = true;
        tti_tr1 = tti_tr2;
        buffer.str("");
      }  

      if(use_lstm==0x89)
      {
        // Send packet size to LSTM every 10 TTI
        packet_index = tti_tr2 %10;
        lstm_packet_size[packet_index] = tti_packet_size;
        if(packet_index == 9 && tti_tr2<19999)
        {
          for(index=0; index < 10; index++)
          {
            //send signal to LSTM
            sprintf(shared_buffer, "%ld", tti_tr2);
            SharedMemoryWrite(lstm_shmid, shared_buffer);

            // send SIZE to LSTM
            sprintf(shared_buffer, "%d", lstm_packet_size[index]);
            SharedMemoryWrite(lte_shmid, shared_buffer);
            //printf("Write %s to lte_shmid/ size:%d\n", shared_buffer, sizeof(shared_buffer));

            //check receive
            //printf("waiting for LSTM receive data\n");
            do{
              SharedMemoryRead(lstm_shmid, lstm_buffer);
              buffer_value = atoi(lstm_buffer);
            }while( buffer_value != -1);
          }
        }
      }
      buffer.str("");
    }
  }

  // continue with remainder
  m_stop = false;

  while (!m_calendar->IsEmpty () && !m_stop){
    // fetch the new scheduler
    scheduler = FetchScheduler(&sh_fd);
    // Update everything needed for scheduler changes
    UpdateAllScheduler(scheduler);
    // execute "action"
    while(tti_tr1 == tti_tr2 && !m_calendar->IsEmpty()){
        ProcessOneEvent ();
        tti_tr2 = FrameManager::Init()->GetTTICounter();
    }
    tti_tr1 = tti_tr2;
    printf("\nLTESIM: TTI Change! Now in TTI # %ld.\n", tti_tr2);
    // append onto big buffer
    bigbuf = bigbuf + buffer.str();
    // send the last TTI
    SendState(&st_fd, buffer.str().c_str());
    SendCQISummary(&cqi_fd);

    if(use_lstm == 0x89)
    {
      // Send packet size to LSTM every 10 TTI
      packet_index = tti_tr2 %10;
      lstm_packet_size[packet_index] = tti_packet_size;
      if(packet_index == 9 && tti_tr2<19999)
      {
        printf("waiting for LSTM\n");
        for(int index=0; index < 10; index++)
        {
          //send signal to LSTM
          sprintf(shared_buffer, "%ld", tti_tr2);
          SharedMemoryWrite(lstm_shmid, shared_buffer);

          // send SIZE to LSTM
          sprintf(shared_buffer, "%d", lstm_packet_size[index]);
          SharedMemoryWrite(lte_shmid, shared_buffer);
          //printf("Write %s to lte_shmid/ size:%d\n", shared_buffer, sizeof(shared_buffer));

          //check receive
          //printf("waiting for LSTM receive data\n");
          do{
            SharedMemoryRead(lstm_shmid, lstm_buffer);
            buffer_value = atoi(lstm_buffer);
          }while( buffer_value != -1);
        }
      }
    }

    printf("LTESIM: Waiting for new Scheduler. \n");
    // clear stream and output capture
    buffer.str("");
  }

  // close the streams
  scheduler = FetchScheduler(&sh_fd);
  SendState(&st_fd, "end");
  close(sh_fd);
  close(st_fd);
  close(cqi_fd);
  printf("Closed all pipes.\n");  
}

void
Simulator::ProcessOneEvent(void)
{
  shared_ptr<Event> next = m_calendar->GetEvent();

  m_unscheduledEvents--;
  m_currentTs = next->GetTimeStamp()/1000.0;
  m_currentUid = next->GetUID();

  if (!next->IsDeleted())
    {
      next->RunEvent();
    }
  m_calendar->RemoveEvent();
}

int
Simulator::GetUID (void)
{
  m_uid++;
  return (m_uid-1);
}

void
Simulator::Stop (void)
{
  cout << " SIMULATOR_DEBUG: Stop (" << m_lastAssignedUid << " events)" << endl;
  m_stop = true;
}

void
Simulator::SetStop (double time)
{
  DoSchedule (time,
              MakeEvent (&Simulator::Stop, this));
}


shared_ptr<Event>
Simulator::DoSchedule (double time,
                       shared_ptr<Event> event)
{
  int timeStamp = round( (time + Now())*1000 );
  event->SetTimeStamp(timeStamp);
  event->SetUID(m_lastAssignedUid);

  m_lastAssignedUid++;
  m_unscheduledEvents++;

  m_calendar->InsertEvent(event);
  return event;
}

void
Simulator::PrintMemoryUsage (void)
{
  system("pmap `ps aux | grep 5G  | grep -v grep | awk '{print $2}'` | grep total");
}
