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

void UseDisk(struct DiskScheduler * scheduler, char* filename)
{
    printf("Wooo\n");
    FILE * file = fopen(filename, "r");
    int checksum = 0;
    while (!feof(file) && !ferror(file)) 
    {
        checksum ^= fgetc(file);
    }
    int size = ftell(file);

    printf("File size: %i", size);
    
    rewind(file);

    int blockSize = scheduler->disk->blockSize;
    int blocks = (int) ceil ((double) size / blockSize);
    for(int i = 0; i < blocks; i++)
    {
        byte* buf = (byte*) malloc(blockSize);
        int result = fread(buf, 1, blockSize, file);
        struct Message *message = malloc(sizeof(struct Message));
        message->buffer = buf;
        message->diskBlock = i + 1; /* Don't write in the first block */
        message->id = i;
        message->messageType = WriteMessageType;
        EnqueueThread_B(scheduler->receiver, buf, sizeof(struct Message));
    }

    int secondCheckum = 0;
    for(int i = 0; i < blocks; i++)
    {
        byte* buf = (byte*) malloc(blockSize);
        DequeueThread_B(scheduler->sender, buf, sizeof(struct Message));
        for (int j = 0; j < blockSize; j++)
        {
            secondCheckum ^= *(buf + j);
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
    struct BufferQueue* receiverBufferQueue = CreateBufferThreaded(MESSAGES_WINDOW_SIZE);
    struct BufferQueue* senderBufferQueue = CreateBufferThreaded(MESSAGES_WINDOW_SIZE);
    struct DiskScheduler* diskScheduler = CreateDiskScheduler(disk, receiverBufferQueue, senderBufferQueue);
    StartDiskScheduler(diskScheduler);
    UseDisk(diskScheduler, "Samples/sample.txt");
}