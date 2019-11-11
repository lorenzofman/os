#include <time.h>
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include "BufferQueueThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#define READERS 1
#define WRITERS 1
#define BUFFERSIZE 1024 * 1024 * 4
#define BLOCKSIZE (1024 - 4)
#define WAITIME 1
static pthread_mutex_t printf_mutex;

int SyncPrintf(const char *format, ... )
{
	int result = 0;
    va_list args;
    va_start(args, format);

    pthread_mutex_lock(&printf_mutex);
	result = vprintf(format, args);
    pthread_mutex_unlock(&printf_mutex);

    va_end(args);
	return result;
}

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

int ThreadBenchmark()
{
	struct BufferQueue* bufferQueue = CreateBuffer(BUFFERSIZE);
	pthread_mutex_init(&printf_mutex, NULL);
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
