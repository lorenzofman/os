#include "BufferQueueThread.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <stdbool.h>
#include <stdarg.h>

#define S_WAIT_TIME 0
#define WAIT_TIME 1

static struct timespec ts = {S_WAIT_TIME, WAIT_TIME};

int headerSize = sizeof(int);

struct BufferQueue* CreateBufferThreaded(int size)
{
	struct BufferQueue* bufferQueue = (struct BufferQueue*)malloc(sizeof(struct BufferQueue));
	if (bufferQueue == NULL)
	{
		return NULL;
	}
	bufferQueue->buffer = (byte*)malloc(size * sizeof(byte));
	if (bufferQueue->buffer == NULL)
	{
		free(bufferQueue);
		return NULL;
	}
	bufferQueue->enqueue = bufferQueue->dequeue = bufferQueue->buffer;
	bufferQueue->usedBytes = 0;
	bufferQueue->capacity = size;
	memset(bufferQueue->buffer, 0, bufferQueue->capacity);
	
	/* Tickets */
	bufferQueue->ticket = bufferQueue->globalTicket = 0;
	pthread_mutex_init(&bufferQueue->ticketLock, NULL);
	pthread_mutex_init(&bufferQueue->globalTicketLock, NULL);
	
	return bufferQueue;
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
	bufferQueue->usedBytes += amount;
}

int EnqueueThread(struct BufferQueue* bufferQueue, byte* data, int dataLength)
{
	/* Acquire ticket and wait for it's turn */
	int myTicket = AcquireTicket(&bufferQueue->ticket, &bufferQueue->ticketLock, &bufferQueue->globalTicket);
	/* Calculate totalSize of the buffer that will be used to store header + data */
	int totalSize = dataLength + headerSize;

	/* If buffer is full, return error code (0) */
	if(bufferQueue->capacity - bufferQueue->usedBytes < totalSize)
	{
		/* Releases current ticket allowing subsequent writers/readers to start working */
		IncrementTicket(&bufferQueue->globalTicket, &bufferQueue->globalTicketLock);
		return 0;
	}

	/* Writes header */
	WriteData(bufferQueue, (byte*) &dataLength, headerSize);
	
	/* Writes data */
	WriteData(bufferQueue, data, dataLength);

	/* Update buffer used size*/
	UpdateUsedSize(bufferQueue, totalSize);

	/* Releases current ticket allowing subsequent writers/readers to start working */
	IncrementTicket(&bufferQueue->globalTicket, &bufferQueue->globalTicketLock);
	
	return 1;
}

int DequeueThread(struct BufferQueue* bufferQueue, void* buffer, int bufferSize)
{
	/* Acquire ticket and wait for it's turn */
	int myTicket = AcquireTicket(&bufferQueue->ticket, &bufferQueue->ticketLock, &bufferQueue->globalTicket);
	if (bufferQueue->usedBytes == 0)
	{
		/* Increment ticket for next readers/writers be able to start */
		IncrementTicket(&bufferQueue->globalTicket, &bufferQueue->globalTicketLock);
		return 0;
	}

	/* Reads the header */
	int bytesCount;

	/* Read header */
	ReadData(bufferQueue, (byte*) &bytesCount, headerSize);	
	
	/* If given buffer doesn't contain enough space */
	if (bytesCount > bufferSize)
	{
		/* Increment ticket for next readers/writers be able to start */
		IncrementTicket(&bufferQueue->globalTicket, &bufferQueue->globalTicketLock);
		return 0;
	}

	/* Calculate total size */
	int totalSize = bytesCount + headerSize;

	/* Update usedBytes value */
	UpdateUsedSize(bufferQueue, -totalSize);

	/* Read data from bufferQueue into buffer */
	ReadData(bufferQueue, buffer, bytesCount);
	
	/* Increment ticket for next readers/writers be able to start */
	IncrementTicket(&bufferQueue->globalTicket, &bufferQueue->globalTicketLock);

    return bytesCount;
}
