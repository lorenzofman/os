#include "BufferQueue.h"
#include "BufferQueueThread.h"
#include <pthread.h>
#include "Disk.h"
#include "Types.h"
#include "Message.h"
struct DiskScheduler
{
    struct Disk * disk;
    struct BufferQueue * receiver;
    struct BufferQueue * sender;
};

struct DiskScheduler *CreateDiskScheduler(struct Disk* disk, struct BufferQueue* receiver, struct BufferQueue* sender)
{
    struct DiskScheduler* diskScheduler = (struct DiskScheduler*)malloc(sizeof(struct DiskScheduler));
    diskScheduler->disk = disk;
    diskScheduler->receiver = receiver;
    diskScheduler->sender = sender;
    return diskScheduler;
}

void StartDiskScheduler(struct DiskScheduler* diskScheduler)
{
    pthread_t diskThread;
    pthread_create(&diskThread, NULL, Schedule, (void*)diskScheduler);
}


void ProcessReadRequest(struct DiskScheduler * scheduler, struct Message* message)
{
    Read(scheduler->disk, message->diskBlock, message->buffer);
}

void ProcessWriteRequest(struct DiskScheduler * scheduler, struct Message* message)
{
    Write(scheduler->disk, message->diskBlock, message->buffer);
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

void Schedule(void* arg)
{
    struct DiskScheduler* diskScheduler = (struct DiskScheduler*) arg;
    uint messageSize = diskScheduler->disk->blockSize;
    byte* block = (byte*)malloc(messageSize);
    while(true)
    {
        struct Message* message = DequeueThread_B(diskScheduler->receiver, block, messageSize);
        ProcessMessage(diskScheduler, message);
    }
}

