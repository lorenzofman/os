#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "CSVGen.h"
#include "Benchmark.h"
#include "Disk.h"
#include "BufferQueueThread.h"
#include "DiskScheduler.h"
#include "Client.h"
#include "Utils.h"
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


/* Convert string to integer (using base 10)*/
int ExtractInt(char* str)
{
    return(strtol(str, NULL, 10));
}
char* ExecuteDisk(int argc, char *argv[])
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
    return "Ok\n";
}
char* ExecuteBenchmark(int argc, char *argv[])
{
    if(argc != 5)
    {
        return "Invalid arguments to benchmark. Please Use -t (threaded) or -nt (non-threaded) and inform BufferSize, BlockSize";
    }
    
    char* firstArg = argv[0];

    int bufferSize = ExtractInt(argv[1]);
    int blockSize = ExtractInt(argv[2]);
    int readers = ExtractInt(argv[3]);
    int writers = ExtractInt(argv[4]);
    if(strcmp(firstArg, "-t") == 0)
    {
        int throughput = ThreadBenchmark(bufferSize, blockSize - 4, readers, writers);
        if(throughput == 0)
        {
            return "Error\n";
        }
        char bufferSizeBuf[10], blockSizeBuf[10], throughputBuf[10];
        FormattedBytes(bufferSize, bufferSizeBuf);
        FormattedBytes(blockSize, blockSizeBuf);
        FormattedBytes(throughput, throughputBuf);

        printf("Buffersize: %s\n", bufferSizeBuf);
        printf("Blocksize: %s\n", blockSizeBuf);
        printf("R:%i W:%i\n", readers, writers);
        printf("Throughput: %s/s\n", throughputBuf);
        return "Threaded Benchmark succesfull\n";
    }

    if(strcmp(firstArg, "-nt") == 0)
    {
        int throughput = Benchmark(bufferSize, blockSize - 4);
        if(throughput == 0)
        {
            return "Error\n";
        }
        char bufferSizeBuf[10], blockSizeBuf[10], throughputBuf[10];
        FormattedBytes(bufferSize, bufferSizeBuf);
        FormattedBytes(blockSize, blockSizeBuf);
        FormattedBytes(throughput, throughputBuf);

        printf("Buffersize: %s Blocksize: %s Throughput: %s/s\n", bufferSizeBuf, blockSizeBuf, throughputBuf);
        return "Threaded Benchmark succesfull\n";
    }
    
    return "Invalid parameter benchmark. Rerun with -t for threaded and -nt for non-threaded \n";
   
}

char* GenerateCSV(int argc, char *argv[])
{
    if(argc != 6)
    {
        return "Invalid arguments to generate CSV. Please Use -t or -nt and inform MinBufferSize, MaxBufferSize, MinBlockSize, Reader threads and Writer threads\n";
    }
    int minBufSize = ExtractInt(argv[1]);
    int maxBufSize = ExtractInt(argv[2]);
    int minBlockSize = ExtractInt(argv[3]);
    int readers = ExtractInt(argv[4]);
    int writers = ExtractInt(argv[5]);
    char* firstArg = argv[0];

    if(strcmp(firstArg, "-t") == 0)
    {
        CSVThreaded(minBufSize, maxBufSize, minBlockSize, readers, writers);
        return "Threaded CSV generated with success\n";
    }

    if(strcmp(firstArg, "-nt") == 0)
    {
        CSVNonThreaded(minBufSize, maxBufSize, minBlockSize);
        return "CSV generated with success\n";
    }
    
    return "Invalid parameter for CSV. Rerun with -t for threaded and -nt for non-threaded \n";
}

char* Execute(int argc, char *argv[])
{
    if(argc == 0)
    {
        return "No arguments passed to program\nYou can use -csv to generate a csv with benchmarks or -benchmark to run only one time. Also check -disk to copy files\n";
    }

    char* firstArg = argv[0];

    if(strcmp(firstArg, "-csv") == 0)
    {
        return GenerateCSV(argc - 1, argv + 1);
    }

    if(strcmp(firstArg, "-benchmark") == 0)
    {
        return ExecuteBenchmark(argc - 1, argv + 1);
    }

    if(strcmp(firstArg, "-disk") == 0)
    {
        return ExecuteDisk(argc - 1, argv + 1);
    }
    return "Invalid arguments\n";
}

int main(int argc, char *argv[])
{
    /* First argument is the program name itself */
    char* returnedMessage = Execute(argc - 1, argv + 1);

    printf("%s", returnedMessage);
    return 0;
}