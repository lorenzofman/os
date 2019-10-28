#include <time.h>
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include "BufferQueueThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define READERS 1
#define WRITERS 1
#define BUFFERSIZE 1024 * 1024 * 256
#define BLOCKSIZE (1024 * 1024 * 256 - 4) // 1KB - Header
#define USETHREADS
#define WAITIME 1000000000000L

struct QueueParameter
{
	struct BufferQueue* bufferQueue;
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
/*
void FileBenchmark()
{
	FILE* input = fopen("TrivialBenchmark.c", "r");
	FILE* output = fopen("Output.c", "wb");
	struct BufferQueue* queue = CreateBuffer(BUFFERSIZE);
	char buffer[1024];
	int lines = 0;
	while (fgets(buffer, 1024, input) != NULL) 
	{
		Enqueue(queue, (byte*)buffer, strlen(buffer) + 1);
		lines++;
		Dequeue(queue, buffer, 1024);
		fputs(buffer, output);
	}
}
*/
#ifdef USETHREADS

void *EnqueueData(void* varg)
{
	struct QueueParameter* queueParameter = (struct QueueParameter*) varg;
	int blocks = BUFFERSIZE / BLOCKSIZE;
	for (int i = 0; i < blocks; i++)
	{
		while(1)
		{
			int result = EnqueueThread(queueParameter->bufferQueue, queueParameter->data, BLOCKSIZE);
			if(result == 0)
			{
				struct timespec ts = {0, WAITIME };
				nanosleep(&ts, NULL);
			}
			else
			{
				break;
			}
		}
	}

}
void *DequeueData(void* varg)
{
	struct QueueParameter* queueParameter = (struct QueueParameter*) varg;
	int blocks = BUFFERSIZE / BLOCKSIZE;
	for (int i = 0; i < blocks; i++)
	{
		while(1)
		{
			int result = DequeueThread(queueParameter->bufferQueue, queueParameter->data, BLOCKSIZE);
			if(result == 0)
			{
				struct timespec ts = {0, WAITIME };
				nanosleep(&ts, NULL);
			}
			else
			{
				break;
			}
		}
	}
}

int main()
{
	struct BufferQueue* bufferQueue = CreateBuffer(BUFFERSIZE);

	pthread_t* readers = (pthread_t*)malloc(sizeof(pthread_t) * READERS);
	pthread_t* writers = (pthread_t*)malloc(sizeof(pthread_t) * WRITERS);
	
	struct QueueParameter* queueParameter = (struct QueueParameter*) malloc(sizeof(struct QueueParameter));
	queueParameter->data = (byte*)malloc(BLOCKSIZE * sizeof(byte));
	queueParameter->bufferQueue = bufferQueue;

	clock_t start = clock();
	for(int i = 0; i < WRITERS; i++)
	{
		pthread_create(&writers[i], NULL, EnqueueData, (void*)queueParameter);
	}
	for(int i = 0; i < READERS; i++)
	{
		pthread_create(&readers[i], NULL, DequeueData, (void*)queueParameter);
	}

	for(int i = 0; i < WRITERS; i++)
	{
		pthread_join(writers[i], NULL);
	}
	for(int i = 0; i < READERS; i++)
	{
		pthread_join(readers[i], NULL);
	}
	clock_t end = clock();
	double elapsedTime = (double)(end - start) / CLOCKS_PER_SEC;
	double result = (double)BUFFERSIZE / elapsedTime;
	long long throughput = (long long) (result);
	printf("Velocity = %s/s\n", SizeString(throughput));
}

#else

int main()
{
	struct BufferQueue* queue = CreateBuffer(BUFFERSIZE);
	int blocks = BUFFERSIZE / BLOCKSIZE;
	byte* data = (byte*)malloc(BLOCKSIZE * sizeof(byte));
	if (data == NULL)
	{
		return 0;
	}
	clock_t start = clock();
	for (int i = 0; i < blocks; i++)
	{
		EnqueueThread(queue, data, BLOCKSIZE);
	}

	for (int i = 0; i < blocks; i++)
	{
		DequeueThread(queue, data, BLOCKSIZE);
	}

	clock_t end = clock();
	double elapsedTime = (double)(end - start) / CLOCKS_PER_SEC;
	double result = (double)BUFFERSIZE / elapsedTime;
	long long throughput = (long long) (result);
	printf("Velocity = %s/s\n", SizeString(throughput));
}

#endif