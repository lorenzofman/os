#ifndef DISK_SCHEDULER
#define DISK_SCHEDULER
#include <stdbool.h>
#include <pthread.h>
struct DiskScheduler
{
    struct Disk * disk;
    struct BufferQueue * receiver;
    struct Message * messageRequests;
    struct Elevator * elevator;
    int sectorInterleaving;
    bool useElevator;
    bool keepScheduling;
};


struct DiskScheduler *CreateDiskScheduler(struct Disk* disk, struct BufferQueue* receiver, int sectorInterleaving, int messageRequestsBufferSize, bool useElevator);

void StopDiskScheduler(struct DiskScheduler* scheduler);

void DestroyDiskScheduler(struct DiskScheduler* diskScheduler);

pthread_t StartDiskScheduler(struct DiskScheduler* diskScheduler);

#endif