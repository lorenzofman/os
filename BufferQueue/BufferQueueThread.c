#include "BufferQueueThread.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <stdbool.h>
#include <stdarg.h>

#define DEBUG
#define S_WAIT_TIME 0
#define WAIT_TIME 1

static struct timespec ts = {S_WAIT_TIME, WAIT_TIME};

int headerSize = sizeof(int);

/* Tickets */
int writeTicket = 0;
int readTicket = 0;

/* Ticket Mutexes */
pthread_mutex_t readTicketMutex;
pthread_mutex_t writeTicketMutex;

/* Global Tickets */
int readBeginGlobalTicket = 0;
int writeBeginGlobalTicket = 0;
int readFinishGlobalTicket = 0;
int writeFinishGlobalTicket = 0;

/*Ticket Global Mutexes */
pthread_mutex_t readBeginGlobalTicketMutex;
pthread_mutex_t writeBeginGlobalTicketMutex;
pthread_mutex_t readFinishGlobalTicketMutex;
pthread_mutex_t writeFinishGlobalTicketMutex;


pthread_mutex_t printf_mutex;

struct BufferQueue* CreateBufferThreaded(int size)
{
	pthread_mutex_init(&printf_mutex, NULL);
	struct BufferQueue* bufferQueue = (struct BufferQueue*)malloc(sizeof(struct BufferQueue));
	if (bufferQueue == NULL)
	{
		return NULL;
	}
	bufferQueue->buffer = bufferQueue->enqueue = bufferQueue->dequeue = (byte*)malloc(size * sizeof(byte));
	if (bufferQueue->enqueue == NULL)
	{
		free(bufferQueue);
		return NULL;
	}
	memset(bufferQueue->dequeue, 0, size);
	bufferQueue->capacity = bufferQueue->readBytes = size;
	bufferQueue->usedBytes = bufferQueue->writtenBytes = 0;

	pthread_mutex_init(&printf_mutex, NULL);
	pthread_mutex_init(&bufferQueue->usedBytesLock, NULL);
	pthread_mutex_init(&bufferQueue->readBytesLock, NULL);
	pthread_mutex_init(&bufferQueue->writtenBytesLock, NULL);

	pthread_mutex_init(&readTicketMutex, NULL);
	pthread_mutex_init(&writeTicketMutex, NULL);
	pthread_mutex_init(&readBeginGlobalTicketMutex, NULL);
	pthread_mutex_init(&writeBeginGlobalTicketMutex, NULL);
	pthread_mutex_init(&readFinishGlobalTicketMutex, NULL);
	pthread_mutex_init(&writeFinishGlobalTicketMutex, NULL);
	
	return bufferQueue;
}


int SyncPrintf(const char *format, ... )
{
	#ifdef DEBUG
	int result = 0;
    va_list args;
    va_start(args, format);

    pthread_mutex_lock(&printf_mutex);
	result = vprintf(format, args);
    pthread_mutex_unlock(&printf_mutex);

    va_end(args);
	return result;
	#endif
	return 0;
}

void IncrementTicket(int *ticket,  pthread_mutex_t* ticketLock)
{
	pthread_mutex_lock(ticketLock);
	(*ticket)++;
	pthread_mutex_unlock(ticketLock);
}

int GetMyTicket(int *ticket, pthread_mutex_t *ticketMutex)
{
	pthread_mutex_lock(ticketMutex);
	int myTicket = *ticket;
	(*ticket)++;
	pthread_mutex_unlock(ticketMutex);
	return myTicket;
}

void WaitTicketTurn(int ticket, int* globalTicket)
{
	while (true)
	{
		if(ticket == *globalTicket)
		{
			return;
		}
		else
		{
			nanosleep(&ts, NULL);
		}
	}
}

int AcquireTicket(int *ticket, pthread_mutex_t * ticketMutex, int * globalTicket)
{
	int myTicket = GetMyTicket(ticket, ticketMutex);
	WaitTicketTurn(myTicket, globalTicket);
	return myTicket;
}

void UpdateUsedSize(struct BufferQueue* bufferQueue, int amount)
{
	pthread_mutex_lock(&bufferQueue->usedBytesLock);
	bufferQueue->usedBytes += amount;
	pthread_mutex_unlock(&bufferQueue->usedBytesLock);
}

void UpdateReadAndWrittenBytesValues(struct BufferQueue* bufferQueue, int totalSize)
{
	pthread_mutex_lock(&bufferQueue->writtenBytesLock);
	pthread_mutex_lock(&bufferQueue->readBytesLock);
	
	bufferQueue->readBytes -= totalSize;
	bufferQueue->writtenBytes += totalSize;
	
	pthread_mutex_unlock(&bufferQueue->readBytesLock);	
	pthread_mutex_unlock(&bufferQueue->writtenBytesLock);
}

bool FullBuffer(struct BufferQueue* bufferQueue, int amount)
{
	int freeBufferSpace = bufferQueue->capacity - bufferQueue->usedBytes;
	int totalRequiredSize = amount + headerSize;
	if (totalRequiredSize > freeBufferSpace || bufferQueue->readBytes < totalRequiredSize)
	{
        return false;
	}
	UpdateUsedSize(bufferQueue, totalRequiredSize);
	return true;
}
	
void WaitsBufferSpace(struct BufferQueue* bufferQueue, int totalSize)
{
	while(FullBuffer(bufferQueue, totalSize) == false)
	{
		nanosleep(&ts, NULL);
	}
}


/* Writes any data to buffer, using data total length in bytes */
void WriteDataAsync(struct BufferQueue* bufferQueue, byte* enqueuePosition, byte* data, int length)
{
	byte* resultingEnd = enqueuePosition + length;
	byte* bufferCapacity = bufferQueue->buffer + bufferQueue->capacity;
	int remainingBytes = resultingEnd - bufferCapacity;
	if (remainingBytes > 0)
	{
		memcpy(enqueuePosition, data, length - remainingBytes);
		int dataStart = length - remainingBytes;
		memcpy(bufferQueue->buffer, data + dataStart, remainingBytes);
	}
	else
	{
		memcpy(enqueuePosition, data, length);
	}
}


int EnqueueThread(struct BufferQueue* bufferQueue, byte* data, int dataLength, int idx)
{
	/* Acquire ticket and wait for it's turn */
	int myTicket = AcquireTicket(&writeTicket, &writeTicketMutex, &writeBeginGlobalTicket);

	/* Calculate totalSize of the buffer that will be used to store header + data */
	int totalSize = dataLength + headerSize;

	/* Waits until buffer has totalSize free space */
	WaitsBufferSpace(bufferQueue, totalSize);

	/* Store current enqueue position and update the next one before releasing ticket for next writers */
	byte* bufferEnqueue = bufferQueue->enqueue;
	IncrementEnqueue(bufferQueue, totalSize);

	/* Releases current ticket allowing subsequent writers to start working */
	IncrementTicket(&writeBeginGlobalTicket, &writeBeginGlobalTicketMutex);

	/* Writes header*/
	WriteDataAsync(bufferQueue, bufferEnqueue, (byte*) &dataLength, headerSize);
	
	/* Calculate data position */
	bufferEnqueue = IncrementedPointer(bufferQueue, bufferEnqueue, headerSize);

	/* Writes data*/
	WriteDataAsync(bufferQueue, bufferEnqueue, data, dataLength);

	/* 
		Wait to update writtenBytes in order. One writer starts, release ticket and loose CPU, 
		another writer is started and finish writing his data, if the second writer updates the 
		writtenBytes value, a reader might think the first writer has finished writing and their
		data is ready. Updating writtenBytes in order prevent that 
	*/ 
	WaitTicketTurn(myTicket, &writeFinishGlobalTicket);

	UpdateReadAndWrittenBytesValues(bufferQueue, totalSize);

	/* Release ticket, allowing other writer inform it finished writing as well */
	IncrementTicket(&writeFinishGlobalTicket, &writeFinishGlobalTicketMutex);
	return 1;
}

void ReadDataAsync(struct BufferQueue* bufferQueue, byte* dequeuePosition, byte* buffer, int length)
{
	byte* bufferCapacity = bufferQueue->buffer + bufferQueue->capacity;
	byte* resultingStart = dequeuePosition + length;
	int remainingBytes = resultingStart - bufferCapacity;
	if (remainingBytes > 0)
	{
		memcpy(buffer, dequeuePosition, length - remainingBytes);

		int dataStart = length - remainingBytes;
		memcpy(buffer + dataStart, bufferQueue->buffer, remainingBytes);
	}
	else
	{
		memcpy(buffer, dequeuePosition, length);
	}
}


void WaitBufferNotEmpty(struct BufferQueue* bufferQueue)
{
	while(true)
	{
		if(bufferQueue->usedBytes > 0)
		{
			return;
		}
		nanosleep(&ts, NULL);
	}
}

void WaitDataIsWritten(struct BufferQueue* bufferQueue)
{
	while(bufferQueue->writtenBytes == 0)
	{
		nanosleep(&ts, NULL);
	}
}

int DequeueThread(struct BufferQueue* bufferQueue, void* buffer, int bufferSize, int idx)
{
	/* Acquire ticket and wait for it's turn */
	int myTicket = AcquireTicket(&readTicket, &readTicketMutex, &readBeginGlobalTicket);

	/* Waits until buffer has at least one byte used */
	WaitBufferNotEmpty(bufferQueue);

	/* Waits, based on written bytes, before reading the data which might be corrupted */
	WaitDataIsWritten(bufferQueue);

	/* Reads the header */
	int bytesCount;
	byte* dequeuePosition = bufferQueue->dequeue;
	ReadDataAsync(bufferQueue, dequeuePosition, (byte*) &bytesCount, headerSize);	
	int totalSize = bytesCount + headerSize;
	if (bytesCount > bufferSize)
	{
		SyncPrintf("R%i) Exception: Invalid buffer\n", idx);
		IncrementTicket(&readBeginGlobalTicket, &readBeginGlobalTicketMutex);
		return 0;
	}

	/* Increment dequeue pointer for the next reader work with */
	IncrementDequeue(bufferQueue, totalSize);	
	
	/* Update usedBytes value */
	UpdateUsedSize(bufferQueue, -totalSize);
	
	/* Update ticket allowing next reader start working */
	IncrementTicket(&readBeginGlobalTicket, &readBeginGlobalTicketMutex);

	/* Calculates the data position */
	dequeuePosition = IncrementedPointer(bufferQueue, dequeuePosition, headerSize);
	
	/* Read data from bufferQueue into buffer */
	ReadDataAsync(bufferQueue, dequeuePosition, buffer, bytesCount);

	/* Wait read bytes update turn */
	WaitTicketTurn(myTicket, &readFinishGlobalTicket);

	UpdateReadAndWrittenBytesValues(bufferQueue, -totalSize);
	
	/* Increment ticket for next readers be able to also update read and written bytes values*/
	IncrementTicket(&readFinishGlobalTicket, &readFinishGlobalTicketMutex);
    return bytesCount;
}