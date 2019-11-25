#ifndef DISK_SCHEDULER
#define DISK_SCHEDULER
struct DiskScheduler
{
    struct Disk * disk;
    struct BufferQueue * receiver;
    struct BufferQueue * sender;
};

struct DiskScheduler *CreateDiskScheduler(struct Disk* disk, struct BufferQueue* receiver, struct BufferQueue* sender);

void StartDiskScheduler(struct DiskScheduler* diskScheduler);

#endif