#include <stdio.h>
#include <stdlib.h>
#include "Disk.h"
#include "BufferQueueThread.h"
#include "DiskScheduler.h"
#include "Client.h"

/* Disk hardware */
#define CYLINDERS 32
#define SURFACES 4
#define SECTORS_PER_TRACK 16
#define BLOCKSIZE 4096

/* Disk metrics */
#define RPM 7200 /* Rotations per minute*/
#define SEARCH_OVERHEAD_TIME 100 /* microseconds  */
#define TRANSFER_TIME 600 /* microseconds */

/* 
    According to wikipedia: The fastest high-end server drives today have a seek time around 4 ms. 
    Some mobile devices have 15 ms drives, with the most common mobile drives at about 12 ms and the most common desktop drives typically being around 9 ms.
    The average seek time is strictly the time to do all possible seeks divided by the number of all possible seeks
    The mean cylinder offset is going to be Cylinders/2
*/
#define AVERAGE_SEEK_TIME 10000 // 10 ms
#define CYLINDER_TIME (2 * AVERAGE_SEEK_TIME / CYLINDERS)
    
/* Optimization parameters */
#define SECTOR_INTERLEAVING 2
#define MESSAGES_WINDOW_SIZE 2048 /* One message uses only 32 bytes */
#define ELEVATOR_MESSAGES_WINDOW_SIZE 96 /* Elevator will look 96 requests before choosing the best one */

int main()
{
    struct Disk* disk = CreateDisk(SECTORS_PER_TRACK * CYLINDERS * SURFACES, BLOCKSIZE, CYLINDERS, SURFACES, SECTORS_PER_TRACK, RPM, SEARCH_OVERHEAD_TIME, TRANSFER_TIME, CYLINDER_TIME);

    struct BufferQueue* schedulerBufferQueue = CreateBufferThreaded(MESSAGES_WINDOW_SIZE, "Receiver");
    struct DiskScheduler* diskScheduler = CreateDiskScheduler(disk, schedulerBufferQueue, SECTOR_INTERLEAVING, ELEVATOR_MESSAGES_WINDOW_SIZE, true);

    struct BufferQueue* clientBufferQueue = CreateBufferThreaded(MESSAGES_WINDOW_SIZE, "Receiver");    
    struct Client* client = CreateClient(clientBufferQueue);

    pthread_t scheduler = StartDiskScheduler(diskScheduler);
    CopyFileToDisk(client, diskScheduler, "Samples/sample.txt", false);
    StopDiskScheduler(diskScheduler);
    pthread_join(scheduler, NULL);
    DestroyClient(client);
    pthread_detach(scheduler);
    DestroyDiskScheduler(diskScheduler);
}