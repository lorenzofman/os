#ifndef DISK
#define DISK
#include "Types.h"

struct Disk
{
    int diskIdentifier; // Number to validate disk
    uint blocks;
    uint blockSize;
    uint cylinders;
    uint superficies;
    uint sectorsPerTrack;
    uint rpm;
    uint searchOverheadTime; /* (μs) */
    uint transferTime; /* (μs) */
    uint cylinderTime; /* (μs) */
    uint currentCylinder;
};

struct Disk* CreateDiskFromFile(char *fileName);

void WriteDiskToFile(struct Disk* disk, char* filename);

void DestroyDisk(struct Disk* disk);

static void Read(struct Disk* disk, int block, void* buf);

static void Write(struct Disk* disk, int block, void *buf);
#endif