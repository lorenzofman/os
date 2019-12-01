#include <stdio.h>
#include <stdlib.h>
#include "Disk.h"
#include "BufferQueueThread.h"
#include "DiskScheduler.h"
#include "Client.h"

#define BLOCKS 1024
#define BLOCKSIZE 512
#define CYLINDERS 32
#define SURFACES 4
#define SECTORS_PER_TRACK 16
#define RPM 5400
#define SEARCH_OVERHEAD_TIME 1
#define TRANSFER_TIME 600 // 690 microseconds

/* 
    According to wikipedia: The fastest high-end server drives today have a seek time around 4 ms. 
    Some mobile devices have 15 ms drives, with the most common mobile drives at about 12 ms and the most common desktop drives typically being around 9 ms.
    The average seek time is strictly the time to do all possible seeks divided by the number of all possible seeks
    The mean cylinder offset is going to be Cylinders/2
*/
#define AVERAGE_SEEK_TIME 10000 // 10 ms
#define CYLINDER_TIME (2 * AVERAGE_SEEK_TIME / CYLINDERS)
    
#define SECTOR_INTERLEAVING 2
extern double allRotationalWaits;
#define MESSAGES_WINDOW_SIZE 2048 /* One message uses only 28 bytes */

#define ELEVATOR_MESSAGES_WINDOW_SIZE 96 /* Buffer will look 96 request before choosing the best one */
int main()
{
    struct Disk* disk = CreateDisk(BLOCKS, BLOCKSIZE, CYLINDERS, SURFACES, SECTORS_PER_TRACK, RPM, SEARCH_OVERHEAD_TIME, TRANSFER_TIME, CYLINDER_TIME);
    struct BufferQueue* schedulerBufferQueue = CreateBufferThreaded(MESSAGES_WINDOW_SIZE, "Receiver");
    struct DiskScheduler* diskScheduler = CreateDiskScheduler(disk, schedulerBufferQueue, SECTOR_INTERLEAVING, ELEVATOR_MESSAGES_WINDOW_SIZE);
    struct BufferQueue* clientBufferQueue = CreateBufferThreaded(MESSAGES_WINDOW_SIZE, "Receiver");    
    struct Client* client = CreateClient(clientBufferQueue);

    StartDiskScheduler(diskScheduler);
    CopyFileToDisk(client, diskScheduler, "Samples/sample.txt", true);
    printf("Operation succeded\n");
}