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
    double start = Now(); // s
    double totalSearchTime = 0;
    int cylinder = block / (disk->surfaces * disk->sectorsPerTrack);
    if(cylinder != disk->currentCylinder)
    {
        uint seekTime = disk->searchOverheadTime; // μs 
        int target = cylinder;
        int current = disk->currentCylinder;
        int cylinderDif = target - current;
        int absCylinderDif = ABS(cylinderDif);
        seekTime += absCylinderDif * disk->cylinderTime; // μs  
        totalSearchTime = seekTime / 1e6; // s
    }


    double timeAfterSeek = start + totalSearchTime; // s
    double rps = disk->rpm / 60.0; // r/s
    double fullRotationTime = 1.0 / rps; // s/r
    long long completedRotations = timeAfterSeek / fullRotationTime;
    double rotationTime = timeAfterSeek - completedRotations * fullRotationTime;
    
    double oneSectorTime = fullRotationTime / disk->sectorsPerTrack; // seconds per sector rotation
    int sector = block % disk->sectorsPerTrack;
    double sectorTime = sector * oneSectorTime; // seconds
    double rotationalWait = sectorTime - rotationTime; // seconds

    if(rotationalWait < 0)
    {
        rotationalWait += rotationTime;
    }
    
    //printf("Seek time: %lf ms\n", (double) totalSearchTime * 1e3);
    //printf("Rotational wait time: %lf ms\n", rotationalWait * 1e3);

    //printf("Accumulated rot wait time: %lf ms\n", allRotationalWaits * 1e3);

    //printf("Transfer time: %lf ms\n\n", (double) disk->transferTime *1e-3);


    double afterWaitRot = timeAfterSeek + rotationalWait;
    return afterWaitRot + disk->transferTime/1e6; // in seconds
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
    //printf("Read operation took: %lf ms\n", time/1e6);
    Sleep(time);
    UpdateDiskCylinder(disk, block);
    memcpy(buf, BlockEnd(disk, block), disk->blockSize);
}

void Write(struct Disk* disk, int block, void *buf)
{
    double end = WaitTill(disk, block);
    //printf("Start: %lf; End: %lf\n", Now(), end);    
    double time = (end - Now()) * 1e9;
    //printf("Write operation took: %lf ms\n", time/1e6);
    Sleep(time);
    UpdateDiskCylinder(disk, block);
    memcpy(BlockEnd(disk, block), buf, disk->blockSize);
}