#include <time.h>
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include "BufferQueueThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define READERS 4
#define WRITERS 4
#define BUFFERSIZE 1024 * 1024 * 32
#define BLOCKSIZE (1024 - 4)

struct QueueParameter
{
	struct BufferQueue* bufferQueue;
	int idx;
	byte * data;
};

char* SizeString(long long i)
{
	char* buffer = (char*)malloc(sizeof(char) * 5);
	if (buffer == NULL)
	{
		return "ERROR";
	}
	if (i < 1024)
	{
		sprintf(buffer, "%lldB", i);
	}
	else if (i < 1024 * 1024)
	{
		sprintf(buffer, "%lldKB", i / 1024);
	}
	else if (i < 1024 * 1024 * 1024)
	{
		sprintf(buffer, "%lldMB", i / (1024 * 1024));
	}
	else
	{
		sprintf(buffer, "%lfGB", (double) i / ((double)1024 * 1024 * 1024));
	}
	return buffer;
}


void *EnqueueData(void* varg)
{
	struct QueueParameter* queueParameter = (struct QueueParameter*) varg;
	int blocks = BUFFERSIZE / (BLOCKSIZE + sizeof(int));
	int blocksPerWriter = blocks/WRITERS;
	for(int j = 0; j < blocksPerWriter; j++)
	{
		EnqueueThread(queueParameter->bufferQueue, queueParameter->data, BLOCKSIZE);
	}

}
void *DequeueData(void* varg)
{
	struct QueueParameter* queueParameter = (struct QueueParameter*) varg;
	int blocks = BUFFERSIZE / (BLOCKSIZE + sizeof(int));
	int blocksPerReader = blocks/READERS;
	for(int j = 0; j < blocksPerReader; j++)
	{
		DequeueThread(queueParameter->bufferQueue, queueParameter->data, BLOCKSIZE);
	}
}

int ThreadBenchmark()
{
	struct BufferQueue* bufferQueue = CreateBufferThreaded(BUFFERSIZE);
	pthread_t* readers = (pthread_t*)malloc(sizeof(pthread_t) * READERS);
	pthread_t* writers = (pthread_t*)malloc(sizeof(pthread_t) * WRITERS);
	
	struct QueueParameter** queueParameters = (struct QueueParameter**)malloc(sizeof(struct QueueParameter) * (READERS + WRITERS));
	for(int i = 0; i < READERS; i++)
	{
		queueParameters[i] = (struct QueueParameter*) malloc(sizeof(struct QueueParameter));
		queueParameters[i]->data = (byte*)malloc(BLOCKSIZE);
		queueParameters[i]->bufferQueue = bufferQueue;
		queueParameters[i]->idx = i;
	}

	for(int i = 0; i < WRITERS; i++)
	{
		queueParameters[READERS + i] = (struct QueueParameter*) malloc(sizeof(struct QueueParameter));
		
		queueParameters[READERS + i]->data = (byte*)malloc(BLOCKSIZE);
		for(int j = 0; j < BLOCKSIZE; j++)
		{
			queueParameters[READERS + i]->data[j] = 'x';
		}
		queueParameters[READERS + i]->bufferQueue = bufferQueue;
		queueParameters[READERS + i]->idx = i;
	}

	clock_t start = clock();
	
	for(int i = 0; i < READERS; i++)
	{
		pthread_create(&readers[i], NULL, DequeueData, (void*)queueParameters[i]);
	}

	for(int i = 0; i < WRITERS; i++)
	{
		pthread_create(&writers[i], NULL, EnqueueData, (void*)queueParameters[READERS + i]);
	}

	for(int i = 0; i < READERS; i++)
	{
		pthread_join(readers[i], NULL);
	}

	for(int i = 0; i < WRITERS; i++)
	{
		pthread_join(writers[i], NULL);
	}


	clock_t end = clock();
	double elapsedTime = (double)(end - start) / CLOCKS_PER_SEC;
	double result = (double)BUFFERSIZE / elapsedTime;
	long long throughput = (long long) (result);
	printf("Time: %lf ms\n", elapsedTime * 1000);
	printf("Payload: %s\n", SizeString(BUFFERSIZE));
	printf("Velocity = %s/s\n", SizeString(throughput));
	return 0;
}

int Benchmark()
{
	struct BufferQueue* queue = CreateBuffer(BUFFERSIZE);
	int blocks = BUFFERSIZE / BLOCKSIZE;
	byte* data = (byte*)malloc(BLOCKSIZE * sizeof(byte));
	if (data == NULL)
	{
		return 1;
	}


	for (int i = 0; i < blocks; i++)
	{
		Enqueue(queue, data, BLOCKSIZE);
	}
	clock_t start = clock();

	for (int i = 0; i < blocks; i++)
	{
		Dequeue(queue, data, BLOCKSIZE);
	}

	clock_t end = clock();

	double elapsedTime = (double)(end - start) / CLOCKS_PER_SEC;
	double result = (double)BUFFERSIZE / elapsedTime;
	long long throughput = (long long) (result);
	printf("Time: %lf ms\n", elapsedTime * 1000);
	printf("Payload: %s\n", SizeString(BUFFERSIZE));
	printf("Velocity: %s/s\n", SizeString(throughput));
	DestroyBuffer(queue);
	free(data);
	return 0;
}

int main(int argc, char *argv[])
{
	if(argc > 0)
	{
		for(int i = 0; i < argc; i++)
		{
			if(strcmp(argv[i], "-t") == 0)
			{
				printf("Threaded Benchmark\n");
				return ThreadBenchmark();
			}
		}
	}
	printf("Non-threaded Benchmark\n");
	return Benchmark();
}
