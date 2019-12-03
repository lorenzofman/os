#include <time.h>
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include "BufferQueueThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct QueueParameter
{
	struct BufferQueue* bufferQueue;
	int idx;
	byte * data;
	int bufferSize;
	int blockSize;
	int readers;
	int writers;
};


void *EnqueueData(void* varg)
{
	struct QueueParameter* queueParameter = (struct QueueParameter*) varg;
	int blocks = queueParameter->bufferSize / (queueParameter->blockSize + sizeof(int));
	int blocksPerWriter = blocks/queueParameter->writers;
	for(int j = 0; j < blocksPerWriter; j++)
	{
		EnqueueThread_B(queueParameter->bufferQueue, queueParameter->data, queueParameter->blockSize);
	}
	return NULL;
}

void *DequeueData(void* varg)
{
	struct QueueParameter* queueParameter = (struct QueueParameter*) varg;
	int blocks = queueParameter->bufferSize / (queueParameter->blockSize + sizeof(int));
	int blocksPerReader = blocks/queueParameter->readers;
	for(int j = 0; j < blocksPerReader; j++)
	{
		DequeueThread_B(queueParameter->bufferQueue, queueParameter->data, queueParameter->blockSize);
	}
	return NULL;
}

long long ThreadBenchmark(int bufferSize, int blockSize, int readers, int writers)
{
	int blocks = bufferSize / (blockSize + sizeof(int));
	float blocksPerReader = (float) blocks/readers;
	float blocksPerWriter = (float) blocks/writers;
	if (blocksPerReader - (int) blocksPerReader > 0)
	{
		printf("Cannot split blocks across readers\n");
		printf("Blocks: %i\n", blocks);
		printf("Blocks per reader: %f\n", blocksPerReader);
		return 0;
	}
	if (blocksPerWriter - (int) blocksPerWriter > 0)
	{
		printf("Cannot split blocks across writers\n");
		return 0;
	}
	/* Create buffer queue */
	struct BufferQueue* bufferQueue = CreateBufferThreaded(bufferSize, "Test");
	if(bufferQueue == NULL)
	{
		return 0;
	}
	/* Create threads buffer */
	pthread_t* readerThreads = (pthread_t*)malloc(sizeof(pthread_t) * readers);
	if (readerThreads == NULL)
	{
		DestroyBuffer(bufferQueue);
		printf("Error\n");
		return 0;
	}
	pthread_t* writerThreads = (pthread_t*)malloc(sizeof(pthread_t) * writers);
	
	if (writerThreads == NULL)
	{
		DestroyBuffer(bufferQueue);
		free(readerThreads);
		printf("Error\n");
		return 0;
	}

	/* Create parameters for threads */

	struct QueueParameter** queueParameters = (struct QueueParameter**)malloc(sizeof(struct QueueParameter) * (readers + writers));
	if (queueParameters == NULL) 
	{
		DestroyBuffer(bufferQueue);
		free(readerThreads);
		free(writerThreads);
		printf("Error\n");
		return 0;
	}
	for(int i = 0; i < readers + writers; i++)
	{
		queueParameters[i] = (struct QueueParameter*) malloc(sizeof(struct QueueParameter));
		if(queueParameters != NULL)
		{
			queueParameters[i]->data = (byte*)malloc(blockSize);
		}
		if (queueParameters[i] == NULL || queueParameters[i]->data == NULL)
		{
			DestroyBuffer(bufferQueue);
			free(readerThreads);
			free(writerThreads);
			for(int j = 0; j < i - 1; j++)
			{
				free(queueParameters[i]);
			}
			free(queueParameters);
			printf("Error\n");
			return 0;
		}
	}
	for(int i = 0; i < readers; i++)
	{
		queueParameters[i]->bufferQueue = bufferQueue;
		queueParameters[i]->idx = i;
		queueParameters[i]->bufferSize = bufferSize;
		queueParameters[i]->blockSize = blockSize;
		queueParameters[i]->readers = readers;
		queueParameters[i]->writers = writers;

	}

	for(int i = 0; i < writers; i++)
	{
		queueParameters[readers + i]->bufferQueue = bufferQueue;
		queueParameters[readers + i]->idx = i;
		queueParameters[readers + i]->bufferSize = bufferSize;
		queueParameters[readers + i]->blockSize = blockSize;
		queueParameters[readers + i]->readers = readers;
		queueParameters[readers + i]->writers = writers;
	}

	/* Start enqueuing/dequeueing */
    clock_t start = clock();
	
	/* Craete reader threads */
	for(int i = 0; i < readers; i++)
	{
		pthread_create(&readerThreads[i], NULL, DequeueData, (void*)queueParameters[i]);
	}
	/* Create writer threads */
	for(int i = 0; i < writers; i++)
	{
		pthread_create(&writerThreads[i], NULL, EnqueueData, (void*)queueParameters[readers + i]);
	}

	/* Wait readers */
	for(int i = 0; i < readers; i++)
	{
		pthread_join(readerThreads[i], NULL);
	}

	/* Wait writers */
	for(int i = 0; i < writers; i++)
	{
		pthread_join(writerThreads[i], NULL);
	}

	/* Stop clock */
	clock_t end = clock();
	
	/* Calculate throughput */; 
	double elapsedTime = (double) (end - start)/ CLOCKS_PER_SEC;
	double result = (double)bufferSize * 2 / elapsedTime; /* Multiplication by two because data is copied to buffer and then copied from buffer*/
	long long throughput = (long long) (result);


	/* Remove all memory allocations */
	DestroyBuffer(bufferQueue);
	for(int i = 0; i < readers + writers; i++)
	{
		free(queueParameters[i]->data);
		free(queueParameters[i]);
	}
	free(readerThreads);
	free(writerThreads);
	free(queueParameters);

	return throughput;
}

long long Benchmark(int bufferSize, int blocksize)
{
	struct BufferQueue* queue = CreateBuffer(bufferSize);
	if(queue == NULL)
	{
		return 0;
	}
	int blocks = bufferSize / blocksize;
	byte* data = (byte*)malloc(blocksize * sizeof(byte));
	if (data == NULL)
	{
		DestroyBuffer(queue);
		return 0;
	}

	clock_t start = clock();

	for (int i = 0; i < blocks; i++)
	{
		Enqueue(queue, data, blocksize);
	}

	for (int i = 0; i < blocks; i++)
	{
		Dequeue(queue, data, blocksize);
	}

	clock_t end = clock();

	double elapsedTime = (double)(end - start) / CLOCKS_PER_SEC;
	char* buffer = malloc(10 * sizeof(char));

	double result = (double) 2 * bufferSize / elapsedTime;
	long long throughput = (long long) (result);
	DestroyBuffer(queue);
	free(data);
	free(buffer);
	return throughput;
}