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
#include "Types.h"
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <getopt.h>
/* Optimization parameters */
#define MESSAGES_WINDOW_SIZE 2048 /* One message uses only 32 bytes */
#define ELEVATOR_MESSAGES_WINDOW_SIZE 96 /* Elevator will look 96 requests before choosing the best one */

/* Convert string to integer (using base 10)*/
int ExtractInt(char* str)
{
    char *endptr = NULL;
    long number = strtol(str, &endptr, 10);
    if (str == endptr ||
        (errno == ERANGE && number == LONG_MIN) ||
        (errno == ERANGE && number == LONG_MAX) ||
        errno == EINVAL ||
        (errno != 0 && number == 0))
    {
        printf("Error parsing input: \"%s\"\n", str);
        exit(EXIT_FAILURE);
    }
    return number;
}

enum DiskMode {UnassignedDiskMode, CreateDiskMode, LoadDiskMode};
enum OPType {UnassignedOpType, ReadOpMode, WriteOpMode};
struct DiskOperation
{
    enum DiskMode diskMode;
    enum OPType opType;
    char* readFilePath;
    char* saveFilePath;
    char* filePath;
    bool useElevator;
    bool randomAccess;

    int cylinders;
    int surfaces;
    int sectorsPerTrack;
    int blockSize;
    int RPM;
    int overheadTime;
    int transferTime;
    int seekTime;
    int sectorInterleaving;
    int seed;
    int fileSize;

};

char* ExecuteDisk(int argc, char *argv[])
{
    struct DiskOperation diskOperation;
    diskOperation.diskMode = UnassignedDiskMode;
    diskOperation.opType = UnassignedOpType;
    diskOperation.saveFilePath = NULL;
    diskOperation.readFilePath = NULL;
    diskOperation.filePath = NULL;
    diskOperation.useElevator = false;
    diskOperation.randomAccess = false;
    diskOperation.seed = 42;
    
    diskOperation.fileSize =
    diskOperation.cylinders = 
    diskOperation.surfaces = 
    diskOperation.sectorsPerTrack =
    diskOperation.blockSize =
    diskOperation.RPM = 
    diskOperation.overheadTime =
    diskOperation.transferTime = 
    diskOperation.seekTime = 
    diskOperation.sectorInterleaving = -1;
    
    static struct option long_options[] = 
    {
        {"load", required_argument, NULL, 'l' },
        {"save", required_argument, NULL, 's' },
        {"read", no_argument, NULL, 'r' },
        {"write", no_argument, NULL, 'w' },        
        {"create", no_argument, NULL, 'c' },
        {"elevator", no_argument, NULL, 'e' },
        {"random", no_argument, NULL, '0' },
        {"cylinders", required_argument, NULL, '1'},
        {"surfaces", required_argument, NULL, '2'},
        {"sectorsPerTrack", required_argument, NULL, '3'},
        {"blockSize", required_argument, NULL, '4'},
        {"RPM", required_argument, NULL, '5'},
        {"overheadTime", required_argument, NULL, '6'},
        {"transferTime", required_argument, NULL, '7'},
        {"seekTime", required_argument, NULL, '8'},
        {"sectorInterleaving", required_argument, NULL, '9'},
        {"filePath", required_argument, NULL, '!'},
        {"seed", required_argument, NULL, '@'},
        {"size", required_argument, NULL, '#'},
        {NULL, 0, NULL,  0 }
    };
    int c;
    int option_index = 0;
    while ((c = getopt_long(argc, argv, "l:s:rwce01:2:3:4:5:6:7:8:9:!:#:@:#:", long_options, &option_index)) != -1)
    {       
        switch (c)
        {
            case 'l':
                if(diskOperation.diskMode != UnassignedDiskMode)
                {
                    printf("Option already defined, use load or create, not both\n");
                    exit(EXIT_FAILURE);
                }
                diskOperation.readFilePath = optarg;
                diskOperation.diskMode = LoadDiskMode;
                break;
            case 's':
                diskOperation.saveFilePath = optarg;
                break;
            case 'r':
                if(diskOperation.opType != UnassignedOpType)
                {
                    printf("Option already defined, use read or write, not both\n");
                }
                diskOperation.opType = ReadOpMode;
                break;
            case 'w':
                if(diskOperation.opType != UnassignedOpType)
                {
                    printf("Option already defined, use read or write, not both\n");
                }
                diskOperation.opType = WriteOpMode;
                break;
            case 'c':
                if(diskOperation.diskMode != UnassignedDiskMode)
                {
                    printf("Option already defined, use load or create, not both\n");
                }
                diskOperation.diskMode = CreateDiskMode;
                break;
            case 'e':
                diskOperation.useElevator = true;
                break;
            case '0':
                diskOperation.randomAccess = true;
                break;
            case '1':
                diskOperation.cylinders = ExtractInt(optarg);
                break;
            case '2':
                diskOperation.surfaces = ExtractInt(optarg);
                break;
            case '3':
                diskOperation.sectorsPerTrack = ExtractInt(optarg);
                break;
            case '4':
                diskOperation.blockSize = ExtractInt(optarg);
                break;
            case '5':
                diskOperation.RPM = ExtractInt(optarg);
                break;
            case '6':
                diskOperation.overheadTime = ExtractInt(optarg);
                break;
            case '7':
                diskOperation.transferTime = ExtractInt(optarg);
                break;
            case '8':
                diskOperation.seekTime = ExtractInt(optarg);      
                break;
            case '9':
                diskOperation.sectorInterleaving = ExtractInt(optarg);
                break;
            case '!':
                diskOperation.filePath = optarg;
                break;
            case '@':
                diskOperation.seed = ExtractInt(optarg);
                break;
            case '#':
                diskOperation.fileSize = ExtractInt(optarg);
                break;
            default:
                printf("Other: %c, %s\n", c, optarg);
                break;
        }
    }

    struct Disk* disk = NULL;
    if(diskOperation.diskMode == LoadDiskMode)
    {
        disk = CreateDiskFromFile(diskOperation.readFilePath);
    }
    else if (diskOperation.diskMode == CreateDiskMode)
    {
        if(diskOperation.blockSize == -1 || diskOperation.cylinders == -1 || diskOperation.surfaces == -1 || diskOperation.sectorsPerTrack == -1 || diskOperation.RPM == -1|| diskOperation.overheadTime == -1|| diskOperation.transferTime == -1)
        {
            printf("Missing arguments\n");
            exit(EXIT_FAILURE);
        }
        int blocks = diskOperation.sectorsPerTrack * diskOperation.cylinders * diskOperation.surfaces;
        disk = CreateDisk(blocks, diskOperation.blockSize, diskOperation.cylinders, diskOperation.surfaces, diskOperation.sectorsPerTrack, diskOperation.RPM, diskOperation.overheadTime, diskOperation.transferTime,(diskOperation.seekTime * 2)/diskOperation.cylinders);
    }
    if(diskOperation.sectorInterleaving == -1 || diskOperation.filePath == NULL || diskOperation.opType == UnassignedOpType || (diskOperation.fileSize == -1 && diskOperation.opType == ReadOpMode) || diskOperation.diskMode == UnassignedDiskMode)
    {
        printf("Missing arguments\n");
        exit(EXIT_FAILURE);
    }
   
    struct BufferQueue* schedulerBufferQueue = CreateBufferThreaded(MESSAGES_WINDOW_SIZE);
    struct DiskScheduler* diskScheduler = CreateDiskScheduler(disk, schedulerBufferQueue, diskOperation.sectorInterleaving, ELEVATOR_MESSAGES_WINDOW_SIZE, diskOperation.useElevator);

    struct BufferQueue* clientBufferQueue = CreateBufferThreaded(MESSAGES_WINDOW_SIZE);    
    struct Client* client = CreateClient(clientBufferQueue);

    pthread_t scheduler = StartDiskScheduler(diskScheduler);
    if(diskOperation.randomAccess == true)
    {
        srand(diskOperation.seed);
    }
    if(diskOperation.opType == WriteOpMode)
    {
        CopyFileToDisk(client, diskScheduler, diskOperation.filePath, !diskOperation.randomAccess);
    }
    else
    {
        CopyFileFromDisk(client, diskScheduler, diskOperation.filePath, !diskOperation.randomAccess, diskOperation.fileSize);
    }
    StopDiskScheduler(diskScheduler);
    if(diskOperation.saveFilePath != NULL)
    {
        WriteDiskToFile(disk, diskOperation.saveFilePath);
    }
    pthread_join(scheduler, NULL);
    DestroyClient(client);
    pthread_detach(scheduler);
    DestroyDiskScheduler(diskScheduler);
    return "";
}

char* ExecuteBenchmark(int argc, char *argv[])
{
    static struct option long_options[] = 
    {
        {"thread", no_argument, NULL, 't'},
        {"bufferSize", required_argument, NULL, '0'},
        {"blockSize", required_argument, NULL,  '1'},
        {"readers", required_argument, NULL, 'r'},
        {"writers", required_argument, NULL, 'w'},        
    };

    int bufferSize = 0, blockSize = 0, readers = 0, writers = 0;
    bool useThreads = false;
    int c, option_index = 0;
    while ((c = getopt_long(argc, argv, "t0:1:r:w:", long_options, &option_index)) != -1)
    {       
        switch (c)
        {
            case 't':
                useThreads = true;
                break;
            case '0':
                bufferSize = ExtractInt(optarg);
                break;
            case '1':
                blockSize = ExtractInt(optarg);
                break;
            case 'r': 
                readers = ExtractInt(optarg);
                break;
            case 'w': 
                writers = ExtractInt(optarg);
                break;
            default:
                break;
        }
    }
    if(useThreads)
    {
        if(blockSize == 0 || bufferSize == 0 || readers == 0 || writers == 0)
        {
            printf("Missing arguments\n");
            exit(EXIT_FAILURE);
        }
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
    else
    {
        if(bufferSize == 0 || blockSize == 0)
        {
            printf("Missing arguments\n");
            exit(EXIT_FAILURE);
        }
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
    static struct option long_options[] = 
    {
        {"thread", no_argument, NULL, 't'},
        {"minBufSize", required_argument, NULL, '0'},
        {"maxBufSize", required_argument, NULL,  '1'},
        {"minBlockSize", required_argument, NULL, '2' },
        {"readers", required_argument, NULL, 'r'},
        {"writers", required_argument, NULL, 'w'}
    };

    int minBufSize = 0, maxBufSize = 0, minBlockSize = 0, readers = 0, writers = 0;
    bool useThreads = false;
    int c, option_index = 0;
    while ((c = getopt_long(argc, argv, "t0:1:2:r:w:", long_options, &option_index)) != -1)
    {       
        switch (c)
        {
            case 't':
                useThreads = true;
                break;
            case '0':
                minBufSize = ExtractInt(optarg);
                break;
            case '1':
                maxBufSize = ExtractInt(optarg);
                break;
            case '2':
                minBlockSize = ExtractInt(optarg);
                break;
            case 'r': 
                readers = ExtractInt(optarg);
                break;
            case 'w': 
                writers = ExtractInt(optarg);
                break;
            default:
                break;
        }
    }
    if(useThreads)
    {
        if(minBufSize == 0 || maxBufSize == 0 || minBlockSize == 0 || readers == 0 || writers == 0)
        {
            printf("Missing arguments\n");
            exit(EXIT_FAILURE);
        }
        CSVThreaded(minBufSize, maxBufSize, minBlockSize, readers, writers);
    }
    else
    {
        if(minBufSize == 0 || maxBufSize == 0 || minBlockSize == 0)
        {
            printf("Missing arguments\n");
            exit(EXIT_FAILURE);
        }
        CSVNonThreaded(minBufSize, maxBufSize, minBlockSize);
    }
    return "";
}

char* Execute(int argc, char *argv[])
{
    static struct option long_options[] = 
    {
        {"csv", no_argument, NULL, 'z'},
        {"benchmark", no_argument, NULL,  'b'},
        {"disk", no_argument, NULL, 'd'}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "zbd", long_options, &option_index);
    switch (c)
    {
        case 'z':
            return GenerateCSV(argc, argv);
        case 'b':
            return ExecuteBenchmark(argc - 1, argv + 1);
        case 'd':
            return ExecuteDisk(argc, argv);
        default:
            break;
    }
    return "No arguments passed to program\nYou can use --csv to generate a csv with benchmarks or --benchmark to run only one time. Also check --disk to copy files\n";
}

int main(int argc, char *argv[])
{
    char* returnedMessage = Execute(argc, argv);
    printf("%s", returnedMessage);
    return 0;
}