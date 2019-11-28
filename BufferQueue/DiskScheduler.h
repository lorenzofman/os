#ifndef DISK_SCHEDULER
#define DISK_SCHEDULER
struct DiskScheduler
{
    struct Disk * disk;
    struct BufferQueue * receiver;
};

struct DiskScheduler *CreateDiskScheduler(struct Disk* disk, struct BufferQueue* receiver);

void StartDiskScheduler(struct DiskScheduler* diskScheduler);

#endif