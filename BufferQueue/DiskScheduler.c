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


struct DiskScheduler *CreateDiskScheduler(struct Disk* disk, struct BufferQueue* receiver, int sectorInterleaving, int messageRequestsBufferSize)
{
    struct DiskScheduler* diskScheduler = (struct DiskScheduler*)malloc(sizeof(struct DiskScheduler));
    diskScheduler->disk = disk;
    diskScheduler->receiver = receiver;
    diskScheduler->sectorInterleaving = sectorInterleaving;
    diskScheduler->messageRequests = (struct Message*) malloc(messageRequestsBufferSize * sizeof(struct Message));
    return diskScheduler;
}
int SectorInterleaving(struct DiskScheduler * scheduler, int block)
{
    int tracks = scheduler->disk->cylinders;
    // printf("Tracks: %i\n", tracks);

    int sectorsPerTrack = scheduler->disk->sectorsPerTrack;
    // printf("Sectors per track: %i\n", sectorsPerTrack);

    int track = block / sectorsPerTrack;
    // printf("Current track: %i\n", track);

    // printf("Sectors per track: %i\n", sectorsPerTrack);
    // printf("Current track sectors: %i - %i\n", track *  sectorsPerTrack, (track+1) * sectorsPerTrack - 1);

    int blockInTrack = block % sectorsPerTrack;
    // printf("Sector in track: %i\n", blockInTrack);

    int n = scheduler->sectorInterleaving;
    // printf("Sector interleaving: %i\n", n);


    int interleaved = (blockInTrack * n) % sectorsPerTrack + (blockInTrack * n) / sectorsPerTrack;
    // printf("Interleaved: %i\n", interleaved);

    int final = interleaved + track * scheduler->disk->sectorsPerTrack; 
    // printf("Original block: %i, resulted block: %i\n", block, final);

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
    uint messageSize = sizeof(struct Message);
    byte* block = (byte*)malloc(messageSize);
    while(true)
    {
        DequeueThread_B(diskScheduler->receiver, block, messageSize);
        struct Message* message = (struct Message*)block;
        ProcessMessage(diskScheduler, message);
    }
}

void StartDiskScheduler(struct DiskScheduler* diskScheduler)
{
    pthread_t diskThread;
    pthread_create(&diskThread, NULL, Schedule, (void*)diskScheduler);
}

