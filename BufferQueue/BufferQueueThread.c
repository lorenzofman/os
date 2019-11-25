#include "BufferQueueThread.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <stdbool.h>
#include <stdarg.h>
#include "Sleep.h"


#define WAIT_TIME 1

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
	bufferQueue->globalWriteTicket = bufferQueue->globalReadTicket = 0;
	bufferQueue->readTicket = bufferQueue->writeTicket = 1;

	pthread_mutex_init(&bufferQueue->ticketLock, NULL);
	pthread_mutex_init(&bufferQueue->globalTicketLock, NULL);

	pthread_mutex_init(&bufferQueue->readTicketLock, NULL);
	pthread_mutex_init(&bufferQueue->globalReadlTicketLock, NULL);

	pthread_mutex_init(&bufferQueue->writeTicketLock, NULL);
	pthread_mutex_init(&bufferQueue->globalWriteTicketLock, NULL);
	
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
			Sleep(WAIT_TIME);
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
/* If it's not possible to insert or remove data the function return an error code */
/* Problem needs to be solved by the caller */
#pragma region Non-Blocking Functions   

int EnqueueThread_NB(struct BufferQueue* bufferQueue, byte* data, int dataLength)
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

int DequeueThread_NB(struct BufferQueue* bufferQueue, void* buffer, int bufferSize)
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

#pragma endregion

/* Functions that certificate to finish correctly */
/* User must be careful though, calling a reading and never writing to buffer will wait indefinitely */
#pragma region Blocking Functions   

bool PendingWrites(struct BufferQueue* bufferQueue)
{
	if(bufferQueue->pendingWrites > 0)
	{
		bufferQueue->pendingWrites--;
		IncrementTicket(&bufferQueue->globalWriteTicket, &bufferQueue->globalWriteTicketLock);
		return true;
	}
}

bool PendingReads(struct BufferQueue* bufferQueue)
{
	if(bufferQueue->pendingReads > 0)
	{
		bufferQueue->pendingReads--;
		IncrementTicket(&bufferQueue->globalReadTicket, &bufferQueue->globalReadlTicketLock);
		return true;
	}
}

int EnqueueThread_B(struct BufferQueue* bufferQueue, byte* data, int dataLength)
{
	/* Acquire ticket and wait for it's turn */
	AcquireTicket(&bufferQueue->ticket, &bufferQueue->ticketLock, &bufferQueue->globalTicket);
	/* Calculate totalSize of the buffer that will be used to store header + data */
	int totalSize = dataLength + headerSize;

	bool pendingWrite = false;

	/* If buffer is full, return error code (0) */
	if(bufferQueue->capacity - bufferQueue->usedBytes < totalSize || bufferQueue->pendingWrites > 0)
	{
		int myTicket = GetMyTicket(&bufferQueue->writeTicket, &bufferQueue->writeTicketLock);
		bufferQueue->pendingWrites++;
		pendingWrite = true;
		/* Releases current ticket allowing subsequent writers/readers to start working */
		IncrementTicket(&bufferQueue->globalTicket, &bufferQueue->globalTicketLock);
		
		WaitTicketTurn(myTicket, &bufferQueue->globalWriteTicket);

	}

	/* Writes header */
	WriteData(bufferQueue, (byte*) &dataLength, headerSize);
	
	/* Writes data */
	WriteData(bufferQueue, data, dataLength);

	/* Update buffer used size*/
	UpdateUsedSize(bufferQueue, totalSize);

	/* A read might cause subsequent writings*/
	/* E.g a block of size 32KB is blocked because buffer is full */
	/* Another block of 16 KB too */
	/* 128 KB block is read */
	/* Read will call the first pending write */
	/* This pending write must see if there are no other pending writes */
	if(PendingReads(bufferQueue) == false)
	{
		PendingWrites(bufferQueue);
	}

	if(pendingWrite)
	{
		bufferQueue->pendingWrites--;
	}
	/* Releases current ticket allowing subsequent writers/readers to start working */
	IncrementTicket(&bufferQueue->globalTicket, &bufferQueue->globalTicketLock);
	
	
	return 1;
}

int DequeueThread_B(struct BufferQueue* bufferQueue, void* buffer, int bufferSize)
{
	/* Acquire ticket and wait for it's turn */
	int myTicket = AcquireTicket(&bufferQueue->ticket, &bufferQueue->ticketLock, &bufferQueue->globalTicket);
	bool pendingRead = false;
	if (bufferQueue->usedBytes == 0)
	{
		int myTicket = GetMyTicket(&bufferQueue->readTicket, &bufferQueue->readTicketLock);
		bufferQueue->pendingReads++;
		pendingRead = true;
		/* Releases current ticket allowing subsequent writers/readers to start working */
		IncrementTicket(&bufferQueue->globalTicket, &bufferQueue->globalTicketLock);

		WaitTicketTurn(myTicket, &bufferQueue->globalReadTicket);
	}

	/* Reads the header */
	int bytesCount;

	/* Read header */
	ReadData(bufferQueue, (byte*) &bytesCount, headerSize);	
	
	/* If given buffer doesn't contain enough space */
	if (bytesCount > bufferSize)
	{
		printf("Fatal\n");
		PendingWrites(bufferQueue);
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
	
	PendingWrites(bufferQueue);
	
	if (pendingRead)
	{
		bufferQueue->pendingReads--;
	}
	/* Increment ticket for next readers/writers be able to start */
	IncrementTicket(&bufferQueue->globalTicket, &bufferQueue->globalTicketLock);

    return bytesCount;
}


#pragma endregion

bool Empty(struct BufferQueue* bufferQueue)
{
    return bufferQueue->usedBytes == 0;
}

bool Full(struct BufferQueue* bufferQueue)
{
    return bufferQueue->usedBytes == bufferQueue->capacity;
}

bool Fits(struct BufferQueue* bufferQueue, int size)
{
    return bufferQueue->usedBytes + size <= bufferQueue->capacity;
}
