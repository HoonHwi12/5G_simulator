#include "shared-memory.h"

//int shmid;
int lstm_packet_size[10] = {0};
int tti_packet_size = 0;

int SharedMemoryInit(key_t KEY_NUM)
{
    // memory address
    int shmid;
    void *shmaddr;

    // request shared memory to kernel
    if((shmid = shmget((key_t)KEY_NUM, 0, 0)) == -1)
    {
        //perror("Shmid key failed");
        return FAIL;
    }
    return shmid;
}

int SharedMemoryCreate(key_t KEY_NUM)
{
    int shmid;
    if((shmid = shmget((key_t)KEY_NUM, SHARED_SIZE, IPC_CREAT| IPC_EXCL | 0666)) == -1)
    {
        printf("Redefinition of Shared-Key, reset key\n");

        // if fail to create shared memory 
        shmid = shmget((key_t)KEY_NUM, SHARED_SIZE, IPC_CREAT| 0666);
        if(shmid == -1)
        {
            //perror("Shared memory resest fail");
            return FAIL;
        }
        else
        {
            // delete shared memory
            if(shmctl(shmid, IPC_RMID, 0) == -1) 
            {
                //perror("Shared memory delete fail");
                return FAIL;
            }

            // create shared memory
            shmid = shmget((key_t)KEY_NUM, SHARED_SIZE, IPC_CREAT| 0666);
            if(shmid == -1)
            {
                //perror("Shared memory create fail");
                return FAIL;
            }
        }
    }
    return shmid;
}

int SharedMemoryRead(int shmid, char *buffer)
{
    // memory address
    void *shmaddr;
    char msize[SHARED_SIZE] = {0};

    // attach shmid
    if((shmaddr = shmat(shmid, (void *)0, 0)) == (void *)-1)
    {
        //perror("Shmat shmid failed");
        return FAIL;
    }
    // read
    memcpy(buffer, (char *)shmaddr, sizeof(msize));

    // detach shmaddr
    if(shmdt(shmaddr) == -1)
    {
        //perror("Shmdt shmaddr failed");
        return FAIL;
    }
    return SUCCESS;
}

int SharedMemoryWrite(int shmid, char *buffer)
{
    void *shmaddr;
    if(sizeof(buffer) > SHARED_SIZE)
    {
        printf("Shared memory size over");
        return FAIL;
    }
    
    // attach memory
    if((shmaddr = shmat(shmid, (void *)0, 0)) == (void *)-1) 
    {
        perror("Shmat failed");
        return FAIL;
    }

    // buffer to memory
    memcpy((char *)shmaddr, buffer, SHARED_SIZE);
    
    // detach memory
    if(shmdt(shmaddr) == -1) 
    {
        perror("Shmdt failed");
        return FAIL;
    }

    return SUCCESS;
}

int SharedMemoryFree(int shmid)
{
    if(shmctl(shmid, IPC_RMID, 0) == -1) 
    {
        perror("Remove shared memory failed");
        return FAIL;
    }

    printf("Shared memory removed");
    return SUCCESS;
}