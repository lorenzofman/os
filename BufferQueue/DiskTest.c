#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Disk.h"
#include "BufferQueue.h"
#include "BufferQueueThread.h"
#include "DiskScheduler.h"
#include "Message.h"
#define BLOCKS 1024
#define BLOCKSIZE 512
#define CYLINDERS 32
#define SUPERFICIES 4
#define SECTORS_PER_TRACK 16
#define RPM 7200
#define SEARCH_OVERHEAD_TIME 1
#define TRANSFER_TIME 1
#define CYLINDER_TIME 1
    
#define MESSAGES_WINDOW_SIZE 2048 /* One message uses only 16 bytes */

void UseDisk(struct DiskScheduler * scheduler, const char* filename)
{
    FILE * file = fopen(filename, "r");
    int checksum = 0;

    if(file == NULL)
    { 
        printf("File doesn't exist\n");
        return;
    }
    while (!feof(file) && !ferror(file)) 
    {
        char c = fgetc(file);
        checksum ^= c;
    }
    int size = ftell(file);

    
    rewind(file);

    int blockSize = scheduler->disk->blockSize;
    int blocks = (int) ceil ((double) size / blockSize);
    for(int i = 0; i < blocks; i++)
    {
        byte* buf = (byte*) malloc(blockSize);
        int result = fread(buf, blockSize, 1, file);
        struct Message *message = malloc(sizeof(struct Message));
        message->buffer = buf;
        message->diskBlock = i + 1; /* Don't write in the first block */
        message->id = i;
        message->messageType = WriteMessageType;
        EnqueueThread_B(scheduler->receiver, message, sizeof(struct Message));
    }
    int secondCheckum = 0;
    for(int i = 0; i < blocks; i++)
    {
        byte* msgBuf = (byte*) malloc(sizeof(struct Message));
        DequeueThread_B(scheduler->sender, msgBuf, sizeof(struct Message)); 
        struct Message* msg = (struct Message*) msgBuf;
        for (int j = 0; j < blockSize; j++)
        {
            secondCheckum ^= *(msg->buffer + j);
        }
    }

    if(checksum == secondCheckum)
    {
        printf("Urray\n");
    }
    else 
    {
        printf("Debugging: here we go again\n");
    }

}


int main()
{
    struct Disk* disk = CreateDisk(BLOCKS, BLOCKSIZE, CYLINDERS, SUPERFICIES, SECTORS_PER_TRACK, RPM, SEARCH_OVERHEAD_TIME, TRANSFER_TIME, CYLINDER_TIME);
    struct BufferQueue* receiverBufferQueue = CreateBufferThreaded(MESSAGES_WINDOW_SIZE, "Receiver");
    struct BufferQueue* senderBufferQueue = CreateBufferThreaded(MESSAGES_WINDOW_SIZE, "Sender");
    struct DiskScheduler* diskScheduler = CreateDiskScheduler(disk, receiverBufferQueue, senderBufferQueue);
    StartDiskScheduler(diskScheduler);
    UseDisk(diskScheduler, "Samples/sample.txt");
}