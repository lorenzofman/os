#ifndef DISK_SCHEDULER
#define DISK_SCHEDULER
struct DiskScheduler
{
    struct Disk * disk;
    struct BufferQueue * receiver;
    struct Message * messageRequests;
    int sectorInterleaving;
};

struct DiskScheduler *CreateDiskScheduler(struct Disk* disk, struct BufferQueue* receiver, int sectorInterleaving, int messageRequestsBufferSize);

void StartDiskScheduler(struct DiskScheduler* diskScheduler);

#endif