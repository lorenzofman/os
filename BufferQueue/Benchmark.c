#include <time.h>
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include "BufferQueueThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define READERS 4
#define WRITERS 1

struct QueueParameter
{
	struct BufferQueue* bufferQueue;
	int idx;
	byte * data;
	int bufferSize;
	int blockSize;
};


void *EnqueueData(void* varg)
{
	struct QueueParameter* queueParameter = (struct QueueParameter*) varg;
	int blocks = queueParameter->bufferSize / (queueParameter->blockSize + sizeof(int));
	int blocksPerWriter = blocks/WRITERS;
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
	int blocksPerReader = blocks/READERS;
	for(int j = 0; j < blocksPerReader; j++)
	{
		DequeueThread_B(queueParameter->bufferQueue, queueParameter->data, queueParameter->blockSize);
	}
	return NULL;
}

long long ThreadBenchmark(int bufferSize, int blockSize)
{
	/* Create buffer queue */
	struct BufferQueue* bufferQueue = CreateBufferThreaded(bufferSize, "Test");
	if(bufferQueue == NULL)
	{
		return 0;
	}
	/* Create threads buffer */
	pthread_t* readers = (pthread_t*)malloc(sizeof(pthread_t) * READERS);
	if (readers == NULL)
	{
		DestroyBuffer(bufferQueue);
		printf("Error\n");
		return 0;
	}
	pthread_t* writers = (pthread_t*)malloc(sizeof(pthread_t) * WRITERS);
	
	if (writers == NULL)
	{
		DestroyBuffer(bufferQueue);
		free(readers);
		printf("Error\n");
		return 0;
	}

	/* Create parameters for threads */

	struct QueueParameter** queueParameters = (struct QueueParameter**)malloc(sizeof(struct QueueParameter) * (READERS + WRITERS));
	if (queueParameters == NULL) 
	{
		DestroyBuffer(bufferQueue);
		free(readers);
		free(writers);
		printf("Error\n");
		return 0;
	}
	for(int i = 0; i < READERS + WRITERS; i++)
	{
		queueParameters[i] = (struct QueueParameter*) malloc(sizeof(struct QueueParameter));
		if(queueParameters != NULL)
		{
			queueParameters[i]->data = (byte*)malloc(blockSize);
		}
		if (queueParameters[i] == NULL || queueParameters[i]->data == NULL)
		{
			DestroyBuffer(bufferQueue);
			free(readers);
			free(writers);
			for(int j = 0; j < i - 1; j++)
			{
				free(queueParameters[i]);
			}
			free(queueParameters);
			printf("Error\n");
			return 0;
		}
	}
	for(int i = 0; i < READERS; i++)
	{
		queueParameters[i]->bufferQueue = bufferQueue;
		queueParameters[i]->idx = i;
		queueParameters[i]->bufferSize = bufferSize;
		queueParameters[i]->blockSize = blockSize;
	}

	for(int i = 0; i < WRITERS; i++)
	{
		queueParameters[READERS + i]->bufferQueue = bufferQueue;
		queueParameters[READERS + i]->idx = i;
		queueParameters[READERS + i]->bufferSize = bufferSize;
		queueParameters[READERS + i]->blockSize = blockSize;
	}

	/* Start enqueuing/dequeueing */
    clock_t start = clock();
	
	/* Craete reader threads */
	for(int i = 0; i < READERS; i++)
	{
		pthread_create(&readers[i], NULL, DequeueData, (void*)queueParameters[i]);
	}
	/* Create writer threads */
	for(int i = 0; i < WRITERS; i++)
	{
		pthread_create(&writers[i], NULL, EnqueueData, (void*)queueParameters[READERS + i]);
	}

	/* Wait readers */
	for(int i = 0; i < READERS; i++)
	{
		pthread_join(readers[i], NULL);
	}

	/* Wait writers */
	for(int i = 0; i < WRITERS; i++)
	{
		pthread_join(writers[i], NULL);
	}

	/* Stop clock */
	clock_t end = clock();
	
	/* Calculate throughput */; 
	double elapsedTime = (double) (end - start)/ CLOCKS_PER_SEC;
	double result = (double)bufferSize * 2 / elapsedTime; /* Multiplication by two because data is copied to buffer and then copied from buffer*/
	long long throughput = (long long) (result);


	/* Remove all memory allocations */
	DestroyBuffer(bufferQueue);
	for(int i = 0; i < READERS + WRITERS; i++)
	{
		free(queueParameters[i]->data);
		free(queueParameters[i]);
	}
	free(readers);
	free(writers);
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