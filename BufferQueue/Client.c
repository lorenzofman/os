#include <stdio.h>
#include "DiskScheduler.h"
#include "Disk.h"
#include "Message.h"
#include "Client.h"
#include "BufferQueueThread.h"
#include <math.h>
#include <time.h>

struct Client* CreateClient(struct BufferQueue* bufferQueue)
{
    struct Client* client = (struct Client*)malloc(sizeof(struct Client));
    client->buffer = bufferQueue;
    return client;
}
void Swap(int * array, int i, int j)
{
    int tmp = array[i];
    array[i] = array[j];
    array[j] = tmp;
}

void DestroyClient(struct Client* client)
{
    DestroyBufferQueueThreaded(client->buffer);
    free(client);
}

/* Generates a sequence of sequential indexes without exceeding the number of disk blocks */
/* Also avoids writing in the first block */
int* SequentialBlocks(struct Disk* disk, int n)
{
    int availableBlocks = disk->blocks - 1;
    if (n > availableBlocks)
    {
        return NULL;
    }
    
    int * blocks = (int*) malloc(n * sizeof(int));
    
    if(blocks == NULL)
    {
        return NULL;
    }

    for(int i = 0; i < n; i++)
    {
        blocks[i] = i + 1;
    }

    return blocks;
}

int* RandomBlocks(struct Disk* disk, int n)
{
    int availableBlocks = disk->blocks - 1;
    if (n > availableBlocks)
    {
        return NULL;
    }

    int * blocks = (int*) malloc(disk->blocks * sizeof(int));
    
    if(blocks == NULL)
    {
        return NULL;
    }
    for(int i = 0; i < disk->blocks - 1; i++)
    {
        blocks[i] = i + 1;
    }
    for(int i = 0; i < n; i++)
    {
        int rndIdx = rand() % (disk->blocks - 1);
        Swap(blocks, i, rndIdx);
    }

    return blocks;
    
}

void CopyFileToDisk(struct Client* client, struct DiskScheduler * scheduler, const char* filename, bool sequential)
{
    FILE * file = fopen(filename, "r");

    if(file == NULL)
    {
        printf("File doesn't exist\n");
        exit(EXIT_FAILURE);
    }
    while (!feof(file) && !ferror(file)) 
    {
        fgetc(file);
    }
    int size = ftell(file);

    rewind(file);

    int blockSize = scheduler->disk->blockSize;
    int blocks = (int) ceil ((double) size / blockSize);
    if(blocks > scheduler->disk->blocks - 1)
    {
        printf("File is too big for the disk\n");
        exit(EXIT_FAILURE);
    }
    int * blocksIdxs = sequential ? SequentialBlocks(scheduler->disk, blocks) : RandomBlocks(scheduler->disk, blocks);
    struct timespec start, finish;
    clock_gettime(CLOCK_MONOTONIC, &start);
    byte** blocksArr = malloc(sizeof(byte*) * blocks);

    for(int i = 0; i < blocks; i++)
    {
        byte* buf = blocksArr[i] = (byte*) malloc(blockSize);
        int res = fread(blocksArr[i], blockSize, 1, file);
        if(res == 0)
        {
            printf("Read error\n");
            return;
        }
        struct Message *message = CreateMessage(WriteMessageType, client->buffer, blocksIdxs[i], i, buf);

        EnqueueThread_B(scheduler->receiver, (byte*) message, sizeof(struct Message));

        free(message);
    }
    fclose(file);
    free(blocksIdxs);

    /* Wait for message confirmations */
    for(int i = 0; i < blocks; i++)
    {
        struct Message message;
        DequeueThread_B(client->buffer, &message, sizeof(struct Message));
    };
    clock_gettime(CLOCK_MONOTONIC, &finish);
    double elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Copied file in %lf ms\n", elapsed * 1e3);
    for (int i = 0; i < blocks; i++)
    {
        free(blocksArr[i]);
    }
    free(blocksArr);
    

}