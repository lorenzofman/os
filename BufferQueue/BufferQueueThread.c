#include "BufferQueueThread.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <stdbool.h>
#include <stdarg.h>
#include "Sleep.h"
//#define DEBUG

#define WAIT_TIME 1

int headerSize = sizeof(int);

struct BufferQueue* CreateBufferThreaded(int size, char* name)
{
	struct BufferQueue* bufferQueue = (struct BufferQueue*)malloc(sizeof(struct BufferQueue));
	if (bufferQueue == NULL)
	{
		return NULL;
	}
	memcpy(bufferQueue->name, name, strlen(name));
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
	pthread_mutex_init(&bufferQueue->globalReadTicketLock, NULL);

	pthread_mutex_init(&bufferQueue->writeTicketLock, NULL);
	pthread_mutex_init(&bufferQueue->globalWriteTicketLock, NULL);
	
	return bufferQueue;
}


/* Printf++ */
int printfpp(char* format, int idx, struct BufferQueue* queue, bool enqueue , ... )
{
	#ifdef DEBUG
	int result = 0;
    va_list args;
    va_start(args, format);
    char buffer[1024];
    if (enqueue)
    {
        sprintf(buffer, "%s (%i) \033[0;32m(Enqueue)\033[0m %s", queue->name, idx, format);
    }
    else
    {
        sprintf(buffer, "%s (%i) \033[0;31m(Dequeue)\033[0m %s", queue->name, idx, format);
    }
	result = vprintf(buffer, args);

    va_end(args);
	return result;
	#else
	return 0;
	#endif

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
		IncrementTicket(&bufferQueue->globalWriteTicket, &bufferQueue->globalWriteTicketLock);
		return true;
	}
}

bool PendingReads(struct BufferQueue* bufferQueue)
{
	if(bufferQueue->pendingReads > 0)
	{
		IncrementTicket(&bufferQueue->globalReadTicket, &bufferQueue->globalReadTicketLock);
		return true;
	}
}

int EnqueueThread_B(struct BufferQueue* bufferQueue, byte* data, int dataLength)
{
	printfpp("Acquiring ticket; Global: %i\n", bufferQueue->ticket, bufferQueue, true, bufferQueue->globalTicket);
	/* Acquire ticket and wait for it's turn */
	int myTicket = AcquireTicket(&bufferQueue->ticket, &bufferQueue->ticketLock, &bufferQueue->globalTicket);
	printfpp("Inserting %i bytes\n", myTicket, bufferQueue, true, dataLength);
	/* Calculate totalSize of the buffer that will be used to store header + data */
	int totalSize = dataLength + headerSize;

	bool pendingWrite = false;

	/* If buffer is full, return error code (0) */
	if(bufferQueue->capacity - bufferQueue->usedBytes < totalSize || bufferQueue->pendingWrites > 0)
	{
		int myWriteTicket = GetMyTicket(&bufferQueue->writeTicket, &bufferQueue->writeTicketLock);
		printfpp("Buffer is full, pending write. My write ticket: %i, global write ticket: %i\n", myTicket, bufferQueue, true, myWriteTicket, bufferQueue->globalWriteTicket);
		bufferQueue->pendingWrites++;
		pendingWrite = true;
		/* Releases current ticket allowing subsequent writers/readers to start working */
		IncrementTicket(&bufferQueue->globalTicket, &bufferQueue->globalTicketLock);
		
		WaitTicketTurn(myWriteTicket, &bufferQueue->globalWriteTicket);

	}

	/* Writes header */
	WriteData(bufferQueue, (byte*) &dataLength, headerSize);
	
	/* Writes data */
	WriteData(bufferQueue, data, dataLength);

	/* Update buffer used size*/
	UpdateUsedSize(bufferQueue, totalSize);


	if(pendingWrite)
	{
		printfpp("Pending write released\n", myTicket, bufferQueue, true);
		bufferQueue->pendingWrites--;
	}

	printfpp("Writing completed\n", myTicket, bufferQueue, true);
	printfpp("%i Pending reads; %i Pending writes\n", myTicket, bufferQueue, true,  bufferQueue->pendingReads, bufferQueue->pendingWrites);

	if(PendingReads(bufferQueue))
	{
		printfpp("Trigger pending read, read global ticket is now: %i\n", myTicket, bufferQueue, true,  bufferQueue->globalReadTicket);
	}
	else if(PendingWrites(bufferQueue))
	{
		printfpp("Trigger pending write, write global ticket is now: %i\n", myTicket, bufferQueue, true,  bufferQueue->globalWriteTicket);
	}
	else
	{
		printfpp("Incrementing global ticket\n", myTicket, bufferQueue, true);
		IncrementTicket(&bufferQueue->globalTicket, &bufferQueue->globalTicketLock);
	}

	printfpp("Success\n", myTicket, bufferQueue, true);
	
	return 1;
}

int DequeueThread_B(struct BufferQueue* bufferQueue, void* buffer, int bufferSize)
{
	/* Acquire ticket and wait for it's turn */
	int myTicket = AcquireTicket(&bufferQueue->ticket, &bufferQueue->ticketLock, &bufferQueue->globalTicket);
	printfpp("Acquiring ticket; Global: %i\n", myTicket, bufferQueue, false, bufferQueue->globalTicket);
	bool pendingRead = false;
	if (bufferQueue->usedBytes == 0)
	{
		printfpp("Buffer is empty, pendind read\n", myTicket, bufferQueue, false);
		int myReadTicket = GetMyTicket(&bufferQueue->readTicket, &bufferQueue->readTicketLock);
		printfpp("My read ticket is %i, global: %i\n", myTicket, bufferQueue, false, myReadTicket, bufferQueue->globalReadTicket);
		bufferQueue->pendingReads++;
		pendingRead = true;
		/* Releases current ticket allowing subsequent writers/readers to start working */
		IncrementTicket(&bufferQueue->globalTicket, &bufferQueue->globalTicketLock);
		WaitTicketTurn(myReadTicket, &bufferQueue->globalReadTicket);
		printfpp("Some data has been added to buffer\n", myTicket, bufferQueue, false);
	}

	/* Reads the header */
	int bytesCount;

	/* Read header */
	ReadData(bufferQueue, (byte*) &bytesCount, headerSize);	
	
	/* If given buffer doesn't contain enough space */
	if (bytesCount > bufferSize)
	{
		printfpp("Fatal\n", myTicket, bufferQueue, false);
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
	printfpp("%i Reading completed\n", myTicket, bufferQueue, true);
	printfpp("%i Pending reads; %i Pending writes\n", myTicket, bufferQueue, false, bufferQueue->pendingReads, bufferQueue->pendingWrites);
	if (pendingRead)
	{
		printfpp("Pending read released\n", myTicket, bufferQueue, false);
		bufferQueue->pendingReads--;
	}

	if (PendingWrites(bufferQueue))
	{
		printfpp("Trigger pending write, write global ticket is now: %i\n", myTicket, bufferQueue, false,  bufferQueue->globalWriteTicket);
	}
	else if (PendingReads(bufferQueue))
	{
		printfpp("Trigger pending read, read global ticket is now: %i\n", myTicket, bufferQueue, false,  bufferQueue->globalReadTicket);
	}
	else
	{
		printfpp("Incrementing global ticket\n", myTicket, bufferQueue, false);
		IncrementTicket(&bufferQueue->globalTicket, &bufferQueue->globalTicketLock);
	}
	
	
	printfpp("Success\n", myTicket, bufferQueue, false);
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
