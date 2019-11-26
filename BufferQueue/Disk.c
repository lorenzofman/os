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

struct Disk* CreateDisk(uint blocks, uint blockSize, uint cylinders, uint superficies, uint sectorsPerTrack, uint rpm, uint searchOverheadTime, uint transferTime, uint cylinderTime)
{
    uint size = blocks * blockSize;
    struct Disk *disk = (struct Disk *)malloc(size);
    disk->diskIdentifier = DISK_ID;
    disk->blocks = blocks;
    disk->blockSize = blockSize;
    disk->cylinders = cylinders;
    disk->superficies = superficies;
    disk->sectorsPerTrack = sectorsPerTrack;
    disk->rpm = rpm;
    disk-> searchOverheadTime = searchOverheadTime;
    disk->transferTime = transferTime;
    disk->cylinderTime = cylinderTime;
    disk->currentCylinder = 0;
    return disk;
}

struct Disk* CreateDiskFromFile(char *fileName)
{
    FILE *file = fopen(fileName, "r");
    if (file == NULL)
    {
        return NULL;
    }
    struct Disk diskHeader;
    if (fread(&diskHeader, sizeof(diskHeader), 1, file) != 1) 
    {
        fclose(file);
        return NULL;
    }
    if (diskHeader.diskIdentifier != DISK_ID) 
    {
        fclose(file);
        return NULL;
    }
    struct Disk *disk = malloc(diskHeader.blocks * diskHeader.blockSize);
    if (disk == NULL) 
    {
        fclose(file);
        return NULL;
    }
    rewind(file);
    if (fread(disk, diskHeader.blockSize, diskHeader.blocks, file) != diskHeader.blocks) 
    {
        free(disk);
        fclose(file);
        return NULL;
    }
    fclose(file);
    return disk;
}

void WriteDiskToFile(struct Disk* disk, char* filename)
{
    if(disk->diskIdentifier == DISK_ID)
    {
        return;
    }
    FILE* file = fopen(filename, "w");
    if(file == NULL)
    {
        return;
    }
    fwrite(disk, disk->blockSize, disk->blocks, file);
    fclose(file);
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
    int cylinder = block / (disk->superficies * disk->sectorsPerTrack);
    
    if(cylinder != disk->currentCylinder)
    {
        uint seekTime = disk->searchOverheadTime;
        seekTime += ABS(cylinder - disk->currentCylinder) * disk->cylinderTime;
        totalSearchTime = seekTime / 1e6;
    }

    double timeAfterSeek = start + totalSearchTime;
    double rps = disk->rpm / 60.0;
    double fullRotationTime = 1.0 / rps; // seconds per full rotation
    long completedRotations = timeAfterSeek / fullRotationTime; 
    double rotationTime = timeAfterSeek - completedRotations * fullRotationTime; 
    double oneSectorTime = rps / disk->sectorsPerTrack; // seconds per sector rotation
    int sector = block % disk->sectorsPerTrack;
    double sectorTime = sector * oneSectorTime;
    double rotationalWait = sectorTime - rotationTime;
    
    if(rotationalWait < 0)
    {
        rotationalWait += rotationTime;
    }

    double afterWaitRot = timeAfterSeek + rotationalWait;

    return afterWaitRot + disk->transferTime;
}

void UpdateDiskCylinder(struct Disk * disk, int block)
{
    int cylinder = block / (disk->superficies * disk->sectorsPerTrack);
    disk->currentCylinder = cylinder;
}

void *BlockEnd(struct Disk* disk, int block)
{
    return (void*) ((char*) disk + block * disk->blockSize);
}

void Read(struct Disk* disk, int block, void* buf)
{
    double end = WaitTill(disk, block);
    Sleep(end - Now() * 1e9);
    UpdateDiskCylinder(disk, block);
    memcpy(buf, BlockEnd(disk, block), disk->blockSize);
}

void Write(struct Disk* disk, int block, void *buf)
{
    double end = WaitTill(disk, block);
    Sleep(end - Now() * 1e9);
    UpdateDiskCylinder(disk, block);
    memcpy(BlockEnd(disk, block), buf, disk->blockSize);
}