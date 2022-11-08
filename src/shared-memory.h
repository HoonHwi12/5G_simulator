/*
    Hoonhwi
*/
#ifndef SHARED_h
#define SHARED_h

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/ipc.h>

#define SUCCESS 0
#define FAIL -1

#define shlog(fmt, ...) \
    do { fprintf(stderr, "[SHARED LOG] " fmt, ## __VA_ARGS__); } while (0)

static int  LTE_KEY =  0x101;
static int  LSTM_KEY =  0x102;
static int  DQN_KEY =  0x103;
static int  SHARED_SIZE = 4096;

//char buffer[SHARED_SIZE] = {0,};
//int buffer_type;

// defined memory
//extern int shmid;
extern int lstm_packet_size[10];
extern int tti_packet_size;
  
extern int SharedMemoryInit(key_t KEY_NUM);
extern int SharedMemoryCreate(key_t KEY_NUM);
extern int SharedMemoryRead(int shmid, char *buffer);
extern int SharedMemoryWrite(int shmid, char *buffer);
extern int SharedMemoryFree(int shmid);


#endif
