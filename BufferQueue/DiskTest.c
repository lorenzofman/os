#include <stdio.h>
#include <stdlib.h>
#include "Disk.h"
#include "BufferQueueThread.h"
#include "DiskScheduler.h"
#include "Client.h"

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



int main()
{
    struct Disk* disk = CreateDisk(BLOCKS, BLOCKSIZE, CYLINDERS, SUPERFICIES, SECTORS_PER_TRACK, RPM, SEARCH_OVERHEAD_TIME, TRANSFER_TIME, CYLINDER_TIME);
    struct BufferQueue* schedulerBufferQueue = CreateBufferThreaded(MESSAGES_WINDOW_SIZE, "Receiver");
    struct DiskScheduler* diskScheduler = CreateDiskScheduler(disk, schedulerBufferQueue);
    struct BufferQueue* clientBufferQueue = CreateBufferThreaded(MESSAGES_WINDOW_SIZE, "Receiver");    
    struct Client* client = CreateClient(clientBufferQueue);

    StartDiskScheduler(diskScheduler);
    CopyFileToDisk(client, diskScheduler, "Samples/sample.txt", false);
    printf("Operation succeded\n");
}