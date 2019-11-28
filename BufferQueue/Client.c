#include <stdio.h>
#include "DiskScheduler.h"
#include "Disk.h"
#include "Message.h"
#include "Client.h"
#include "BufferQueueThread.h"
#include <math.h>

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
        return;
    }
    while (!feof(file) && !ferror(file)) 
    {
        char c = fgetc(file);
    }
    int size = ftell(file);

    rewind(file);

    int blockSize = scheduler->disk->blockSize;
    int blocks = (int) ceil ((double) size / blockSize);
    int * blocksIdxs = sequential ? SequentialBlocks(scheduler->disk, blocks) : RandomBlocks(scheduler->disk, blocks);
    for(int i = 0; i< blocks; i++)
    {
        printf("%i\n", blocksIdxs[i]);
    }
    clock_t start = clock();
    for(int i = 0; i < blocks; i++)
    {
        byte* buf = (byte*) malloc(blockSize);
        fread(buf, blockSize, 1, file);
        struct Message *message = CreateMessage(WriteMessageType, client->buffer, blocksIdxs[i], i, buf);
        EnqueueThread_B(scheduler->receiver, (byte*) message, sizeof(struct Message));
    }
    /* Wait for message confirmations */
    for(int i = 0; i < blocks; i++)
    {
        struct Message message;
        DequeueThread_B(client->buffer, &message, sizeof(struct Message));
    }
    clock_t end = clock();
    printf("Copied file in %.3lf ms\n", (double)(end - start) / (CLOCKS_PER_SEC/1000));
}