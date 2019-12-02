#include "BufferQueue.h"
#include "BufferQueueThread.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "DiskScheduler.h"
#include "Disk.h"
#include "Types.h"
#include "Message.h"
#include "Elevator.h"
#include <stdarg.h>
#include "Constants.h"
#include "Sleep.h"

struct DiskScheduler *CreateDiskScheduler(struct Disk* disk, struct BufferQueue* receiver, int sectorInterleaving, int messageRequestsBufferSize, bool useElevator)
{
    struct DiskScheduler* diskScheduler = (struct DiskScheduler*)malloc(sizeof(struct DiskScheduler));
    diskScheduler->disk = disk;
    diskScheduler->receiver = receiver;
    diskScheduler->sectorInterleaving = sectorInterleaving;
    diskScheduler->messageRequests = (struct Message*) malloc(messageRequestsBufferSize * sizeof(struct Message));
    diskScheduler->useElevator = useElevator;
    diskScheduler->keepScheduling = true;
    if(useElevator)
    {
        diskScheduler->elevator = CreateElevator();
    }
    return diskScheduler;
}
int SectorInterleaving(struct DiskScheduler * scheduler, int block)
{
    int sectorsPerTrack = scheduler->disk->sectorsPerTrack;

    int track = block / sectorsPerTrack;

    int blockInTrack = block % sectorsPerTrack;

    int n = scheduler->sectorInterleaving;

    int interleaved = (blockInTrack * n) % sectorsPerTrack + (blockInTrack * n) / sectorsPerTrack;

    int final = interleaved + track * scheduler->disk->sectorsPerTrack; 

    return final; 
}

void ProcessReadRequest(struct DiskScheduler * scheduler, struct Message* message)
{
    int interceptedBlock = SectorInterleaving(scheduler, message->diskBlock);
    Read(scheduler->disk, interceptedBlock, message->buf);
    EnqueueThread_B(message->clientBuffer, (byte*) message, sizeof(struct Message));
}

void ProcessWriteRequest(struct DiskScheduler * scheduler, struct Message* message)
{
    int interceptedBlock = SectorInterleaving(scheduler, message->diskBlock);
    Write(scheduler->disk, interceptedBlock, message->buf);
    EnqueueThread_B(message->clientBuffer, (byte*) message, sizeof(struct Message));
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
    while(diskScheduler->keepScheduling)
    {
        if(Empty(diskScheduler->receiver) && diskScheduler->elevator->pendingMessages == 0)
        {
            Sleep(100000);
            continue;
        }
        struct Message message;
        if(diskScheduler->useElevator)
        {
            message = Escalonate(diskScheduler, diskScheduler->elevator);
        }
        else
        {
            DequeueThread_B(diskScheduler->receiver, &message, sizeof(struct Message));
        }
        ProcessMessage(diskScheduler, &message);
    }
    return NULL;
}

pthread_t StartDiskScheduler(struct DiskScheduler* diskScheduler)
{
    pthread_t diskThread;
    pthread_create(&diskThread, NULL, Schedule, (void*)diskScheduler);
    return diskThread;
}

void DestroyDiskScheduler(struct DiskScheduler* diskScheduler)
{
    DestroyBufferQueueThreaded(diskScheduler->receiver);
    DestroyDisk(diskScheduler->disk);
    if(diskScheduler->useElevator)
    {
        DestroyElevator(diskScheduler->elevator);
    }
    free(diskScheduler->messageRequests);
    free(diskScheduler);
}

void StopDiskScheduler(struct DiskScheduler* scheduler)
{
    scheduler->keepScheduling = false;
}