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
#include "../../device/GNodeB.h"

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
  GNodeB::DLSchedulerType scheduler;
  // Open, connect to pipes
  int sh_fd, st_fd, cqi_fd;
  GNodeB::OpenSchedulerFifo(&sh_fd);
  // open CQI Fifo
  GNodeB::ConnectCQIFifo(&cqi_fd);
  // connect and forward #UEs
  GNodeB::ConnectStateFifo(&st_fd);
  // form information about each eNB's UE's application QoS and send!
  GNodeB::SendUESummary(&st_fd);



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
      printf("\nLTESIM: TTI Change! Now in TTI # %ld.\n", tti_tr2);
      // append onto big buffer
      bigbuf = bigbuf + buffer.str();
      // if we are past empty TTIs
      if(!bigbuf.empty()){
        printf("bigbuf: %s\n", bigbuf.c_str() );
        // send the update TTI
        GNodeB::SendState(&st_fd, buffer.str().c_str());
        GNodeB::SendCQISummary(&cqi_fd);
        printf("LTESIM: Waiting for new Scheduler. \n");
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
    scheduler = GNodeB::FetchScheduler(&sh_fd);

    // Update everything needed for scheduler changes
    GNodeB::UpdateAllScheduler(scheduler);
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
    GNodeB::SendState(&st_fd, buffer.str().c_str());
    GNodeB::SendCQISummary(&cqi_fd);

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
  scheduler = GNodeB::FetchScheduler(&sh_fd);
  GNodeB::SendState(&st_fd, "end");
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
