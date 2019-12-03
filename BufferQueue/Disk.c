#include "Disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#ifndef __USE_POSIX199309
#define __USE_POSIX199309
#endif 
#include <time.h>
#include "Types.h"
#include "Utils.h"
#include "Constants.h"
#include "Sleep.h"

struct Disk* CreateDisk(uint blocks, uint blockSize, uint cylinders, uint surfaces, uint sectorsPerTrack, uint rpm, uint searchOverheadTime, uint transferTime, uint cylinderTime)
{
    uint size = blocks * blockSize;
    struct Disk *disk = (struct Disk *)malloc(size);
    disk->diskIdentifier = DISK_ID;
    disk->blocks = blocks;
    disk->blockSize = blockSize;
    disk->cylinders = cylinders;
    disk->surfaces = surfaces;
    disk->sectorsPerTrack = sectorsPerTrack;
    disk->rpm = rpm;
    disk->searchOverheadTime = searchOverheadTime;
    disk->transferTime = transferTime;
    disk->cylinderTime = cylinderTime;
    disk->currentCylinder = 0;
    return disk;
}

void DestroyDisk(struct Disk* disk)
{
    free(disk);
}

double Now()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec + ts.tv_nsec/1e9;
}

double WaitTill(struct Disk* disk, int block)
{
    double start = Now();
    double totalSearchTime = 0;
    int cylinder = block / (disk->surfaces * disk->sectorsPerTrack);
    if(cylinder != disk->currentCylinder)
    {
        uint seekTime = disk->searchOverheadTime;
        int target = cylinder;
        int current = disk->currentCylinder;
        int cylinderDif = target - current;
        int absCylinderDif = ABS(cylinderDif);
        seekTime += absCylinderDif * disk->cylinderTime;
        totalSearchTime = seekTime / 1e6;
    }

    double timeAfterSeek = start + totalSearchTime; 
    double rps = disk->rpm / 60.0;
    double fullRotationTime = 1.0 / rps;
    long long completedRotations = timeAfterSeek / fullRotationTime;
    double rotationTime = timeAfterSeek - completedRotations * fullRotationTime;
    
    double oneSectorTime = fullRotationTime / disk->sectorsPerTrack; 
    int sector = block % disk->sectorsPerTrack;
    double sectorTime = sector * oneSectorTime;
    double rotationalWait = sectorTime - rotationTime;

    if(rotationalWait < 0)
    {
        rotationalWait += rotationTime;
    }

    double afterWaitRot = timeAfterSeek + rotationalWait;
    return afterWaitRot + disk->transferTime/1e6;
}

void UpdateDiskCylinder(struct Disk * disk, int block)
{
    int cylinder = block / (disk->surfaces * disk->sectorsPerTrack);
    disk->currentCylinder = cylinder;
}

void *BlockEnd(struct Disk* disk, int block)
{
    return (void*) ((char*) disk + block * disk->blockSize);
}

void Read(struct Disk* disk, int block, void* buf)
{
    double end = WaitTill(disk, block);
    double time = (end - Now()) * 1e9;
    Sleep(time);
    UpdateDiskCylinder(disk, block);
    memcpy(buf, BlockEnd(disk, block), disk->blockSize);
}

void Write(struct Disk* disk, int block, void *buf)
{
    double end = WaitTill(disk, block);  
    double time = (end - Now()) * 1e9;
    Sleep(time);
    UpdateDiskCylinder(disk, block);
    memcpy(BlockEnd(disk, block), buf, disk->blockSize);
}