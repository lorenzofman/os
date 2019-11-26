#include "BufferQueue.h"
#include "BufferQueueThread.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "DiskScheduler.h"
#include "Disk.h"
#include "Types.h"
#include "Message.h"
#include <stdarg.h>
#include "Constants.h"


struct DiskScheduler *CreateDiskScheduler(struct Disk* disk, struct BufferQueue* receiver, struct BufferQueue* sender)
{
    struct DiskScheduler* diskScheduler = (struct DiskScheduler*)malloc(sizeof(struct DiskScheduler));
    diskScheduler->disk = disk;
    diskScheduler->receiver = receiver;
    diskScheduler->sender = sender;
    return diskScheduler;
}

void ProcessReadRequest(struct DiskScheduler * scheduler, struct Message* message)
{
    Read(scheduler->disk, message->diskBlock, message->buffer);
    EnqueueThread_B(scheduler->sender, (byte*) message, sizeof(struct Message));
}

void ProcessWriteRequest(struct DiskScheduler * scheduler, struct Message* message)
{
    Write(scheduler->disk, message->diskBlock, message->buffer);
    EnqueueThread_B(scheduler->sender, (byte*) message, sizeof(struct Message));
}

void ProcessMessage(struct DiskScheduler* scheduler, struct Message* message)
{
    switch (message->messageType)
    {
        case ReadMessageType:
            ProcessReadRequest(scheduler, message);
            break;
        case WriteMessageType:
            ProcessWriteRequest(scheduler, message);
            break;
        default: 
            printf("Invalid message type\n");
            return;
    }
}

void* Schedule(void* varg)
{
    struct DiskScheduler* diskScheduler = (struct DiskScheduler*) varg;
    printf("ID: %x\n", diskScheduler->disk->diskIdentifier);
    uint messageSize = sizeof(struct Message);
    byte* block = (byte*)malloc(messageSize);
    while(true)
    {
        DequeueThread_B(diskScheduler->receiver, block, messageSize);
        struct Message* message = (struct Message*)block;
        if(message->safe != DISK_ID)
        {
            printf("Holy mother (%x), DISK_ID = %x\n", message->safe, DISK_ID);
        }
        printf("Message %i: Type - %i, DiskBlock: %i\n", message->id, message->messageType, message->diskBlock);
        ProcessMessage(diskScheduler, message);
    }
}

void StartDiskScheduler(struct DiskScheduler* diskScheduler)
{
    pthread_t diskThread;
    pthread_create(&diskThread, NULL, Schedule, (void*)diskScheduler);
}

