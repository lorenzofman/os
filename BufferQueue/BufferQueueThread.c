#include "BufferQueueThread.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <stdbool.h>
#include <stdarg.h>

#define DEBUG
#define WAIT_TIME 1

/* Tickets */
int bufferOrderTicket = 0;
int writeOrderTicket = 0;
int readOrderTicket = 0;

/* Global Tickets */
int bufferOrderGlobalTicket = 0;
int writeOrderGlobalTicket = 0;
int readOrderGlobalTicket = 0;

/* Ticket Mutexes */
pthread_mutex_t bufferOrderTicketMutex;
pthread_mutex_t writeOrderTicketMutex;
pthread_mutex_t readOrderTicketMutex;

/* Global Tickets Mutexes */
pthread_mutex_t bufferOrderGlobalTicketMutex;
pthread_mutex_t writeOrderGlobalTicketMutex;
pthread_mutex_t readOrderGlobalTicketMutex;


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
	pthread_mutex_init(&bufferQueue->dequeueLock, NULL);
	pthread_mutex_init(&bufferQueue->enqueueLock, NULL);
	pthread_mutex_init(&bufferQueue->usedBytesLock, NULL);
	pthread_mutex_init(&bufferQueue->readBytesLock, NULL);
	pthread_mutex_init(&bufferQueue->writtenBytesLock, NULL);
	pthread_mutex_init(&bufferOrderTicketMutex, NULL);
	pthread_mutex_init(&bufferOrderGlobalTicketMutex, NULL);
	pthread_mutex_init(&writeOrderGlobalTicketMutex, NULL);
	pthread_mutex_init(&readOrderGlobalTicketMutex, NULL);
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


void debug_pthread_mutex_lock(pthread_mutex_t* locker)
{
	if(pthread_mutex_lock(locker) != 0)
	{
		SyncPrintf("Fatal");
	}

}

void debug_pthread_mutex_unlock(pthread_mutex_t* locker)
{
	if(pthread_mutex_unlock(locker) != 0)
	{
		SyncPrintf("Fatal");
	}
}


int GetMyTicket(int *ticket, pthread_mutex_t *ticketMutex, void (*whenAcquireBeforeReleaseCallback)(void*), void * param)
{
	debug_pthread_mutex_lock(ticketMutex);
	int myTicket = *ticket;
	(*ticket)++;
	if(whenAcquireBeforeReleaseCallback != NULL)
	{
		whenAcquireBeforeReleaseCallback(param);
	}
	debug_pthread_mutex_unlock(ticketMutex);
	return myTicket;
}

void WaitTicketTurn(int bufferOrderTicket, int* globalTicket, pthread_mutex_t* lock, int waitTime)
{
	while (true)
	{
		if(bufferOrderTicket == *globalTicket)
		{
			debug_pthread_mutex_lock(lock);
			*globalTicket = bufferOrderTicket + 1;
			debug_pthread_mutex_unlock(lock);
			return;
		}
		else
		{
			struct timespec ts = {0, waitTime};
			nanosleep(&ts, NULL);
		}
	}
}


bool FullBuffer(struct BufferQueue* bufferQueue, int amount, int idx)
{
	debug_pthread_mutex_lock(&bufferQueue->usedBytesLock);
	int freeBufferSpace = bufferQueue->capacity - bufferQueue->usedBytes;
	int headerSize = sizeof(int);
	int totalRequiredSize = amount;
	if (totalRequiredSize > freeBufferSpace || bufferQueue->readBytes < totalRequiredSize)
	{
        debug_pthread_mutex_unlock(&bufferQueue->usedBytesLock);
		return false;
	}
	bufferQueue->usedBytes += amount;
	SyncPrintf("W%i) Used bytes: %i\n", idx, bufferQueue->usedBytes);
	debug_pthread_mutex_unlock(&bufferQueue->usedBytesLock);
	return true;
}

void FetchWriteOrder(void *param)
{
	*(int*)param = GetMyTicket(&writeOrderTicket, &writeOrderTicketMutex, NULL, NULL);
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
	int myWriteOrderTicket;
	int myBufferOrderTicket = GetMyTicket(&bufferOrderTicket, &bufferOrderTicketMutex, FetchWriteOrder, &myWriteOrderTicket);
	SyncPrintf("W%i) Fetching ticket: %i\n", idx, myBufferOrderTicket);
	WaitTicketTurn(myBufferOrderTicket, &bufferOrderGlobalTicket, &bufferOrderGlobalTicketMutex, 1);
	SyncPrintf("W%i) My turn: %i\n", idx, myBufferOrderTicket);


	int headerSize = sizeof(int);
	int totalSize = dataLength + headerSize;
	SyncPrintf("W%i) Inserting %i bytes + %i header bytes\n", idx, dataLength, headerSize);	
	while(FullBuffer(bufferQueue, totalSize, idx) == false)
	{
		SyncPrintf("W%i) Full buffer, waiting %i ns\n", idx, WAIT_TIME);
		struct timespec ts = {0, WAIT_TIME};
		nanosleep(&ts, NULL);
	}
	SyncPrintf("W%i) Buffer has capacity\n", idx, dataLength, headerSize);
	debug_pthread_mutex_lock(&bufferQueue->enqueueLock);
	byte* bufferEnqueue = bufferQueue->enqueue;
	IncrementEnqueue(bufferQueue, totalSize);
	SyncPrintf("W%i) Incrementing enqueue position for next writers\n", idx);
	debug_pthread_mutex_unlock(&bufferQueue->enqueueLock);

	WriteDataAsync(bufferQueue, bufferEnqueue, (byte*) &dataLength, headerSize);
	SyncPrintf("W%i) Writing header async\n", idx);
	bufferEnqueue = IncrementedPointer(bufferQueue, bufferEnqueue, headerSize);
	WriteDataAsync(bufferQueue, bufferEnqueue, data, dataLength);
	SyncPrintf("W%i) Writing data async\n", idx);


	SyncPrintf("W%i) Waiting to update write in order\n", idx);
	SyncPrintf("W%i) My index: %i, global index: %i\n", idx, myBufferOrderTicket, writeOrderGlobalTicket);
	WaitTicketTurn(myWriteOrderTicket, &writeOrderGlobalTicket, &writeOrderGlobalTicketMutex, 1);
	SyncPrintf("W%i) Turn is mine\n", idx);	
	debug_pthread_mutex_lock(&bufferQueue->writtenBytesLock);
	debug_pthread_mutex_lock(&bufferQueue->readBytesLock);
	SyncPrintf("W%i) Updating values\n", idx);		
	bufferQueue->writtenBytes += totalSize;
	bufferQueue->readBytes -= totalSize;
	debug_pthread_mutex_unlock(&bufferQueue->readBytesLock);
	debug_pthread_mutex_unlock(&bufferQueue->writtenBytesLock);
	SyncPrintf("W%i) Insertion terminated succesfully\n", idx);
	return 1;
}

void FetchReadOrder(void *param)
{
	*(int*)param = GetMyTicket(&readOrderTicket, &readOrderGlobalTicketMutex, NULL, NULL);
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


int DequeueThread(struct BufferQueue* bufferQueue, void* buffer, int bufferSize, int idx)
{
	int myReadOrderTicket;
	int myTicket = GetMyTicket(&bufferOrderTicket, &bufferOrderTicketMutex, FetchReadOrder, &myReadOrderTicket);
	SyncPrintf("R%i) Fetching ticket: %i\n", idx, myTicket);
	WaitTicketTurn(myTicket, &bufferOrderGlobalTicket, &bufferOrderGlobalTicketMutex, 1);
	SyncPrintf("R%i) My turn: %i\n", idx, myTicket);

	while(true)
	{
		debug_pthread_mutex_lock(&bufferQueue->usedBytesLock);
		if(bufferQueue->usedBytes > 0)
		{
			SyncPrintf("R%i) Buffer isn't empty, continuing\n", idx);
			debug_pthread_mutex_unlock(&bufferQueue->usedBytesLock);
			break;
		}
		SyncPrintf("R%i) Empty buffer, waiting %i ns\n", idx, WAIT_TIME);
		debug_pthread_mutex_unlock(&bufferQueue->usedBytesLock);
		struct timespec ts = {0, WAIT_TIME};
		nanosleep(&ts, NULL);
	}

	debug_pthread_mutex_lock(&bufferQueue->dequeueLock);
	SyncPrintf("R%i) Locking dequeue\n", idx);
	byte* dequeuePosition = bufferQueue->dequeue;
	int bytesCount;
	SyncPrintf("R%i) Reading length of block\n", idx);
	ReadDataAsync(bufferQueue, dequeuePosition, (byte*) &bytesCount, sizeof(int));

	while(true)
	{
		debug_pthread_mutex_lock(&bufferQueue->writtenBytesLock);
		if(bytesCount < bufferQueue->writtenBytes)
		{
			SyncPrintf("R%i) Data is written", idx);
			debug_pthread_mutex_unlock(&bufferQueue->writtenBytesLock);
			break;
		}
		SyncPrintf("R%i) Data not written yet, waiting %i ns\n", idx, WAIT_TIME);
		SyncPrintf("R%i) WrittenBytes: %i, BytesCount: %i, UsedBytes: %i\n", idx, bufferQueue->writtenBytes, bytesCount, bufferQueue->usedBytes);
		debug_pthread_mutex_unlock(&bufferQueue->writtenBytesLock);
		struct timespec ts = {0, WAIT_TIME};
		nanosleep(&ts, NULL);
	}
	
	SyncPrintf("R%i) Increment Dequeue\n", idx);
	IncrementDequeue(bufferQueue, bytesCount);
	SyncPrintf("R%i) Update used bytes\n", idx);	
	bufferQueue->usedBytes -= bytesCount + sizeof(int);
	if (bytesCount > bufferSize)
	{
		SyncPrintf("R%i) Exception: Invalid buffer\n", idx);
		return 0;
	}
	SyncPrintf("R%i) Unlocing dequeue and usedBytes locks\n", idx);
	debug_pthread_mutex_unlock(&bufferQueue->dequeueLock);
	debug_pthread_mutex_unlock(&bufferQueue->usedBytesLock);

	SyncPrintf("R%i) Reading data async\n", idx);
	byte* readPos = IncrementedPointer(bufferQueue, dequeuePosition, sizeof(int));
	ReadDataAsync(bufferQueue, readPos, (byte*) &bytesCount, sizeof(int));

	SyncPrintf("R%i) Waiting ticket to update read/written bytes\n", idx);
	WaitTicketTurn(myReadOrderTicket, &readOrderGlobalTicket, &readOrderGlobalTicketMutex, 1);
	debug_pthread_mutex_lock(&bufferQueue->writtenBytesLock);
	debug_pthread_mutex_lock(&bufferQueue->readBytesLock);
	SyncPrintf("R%i) Updating read/written bytes\n", idx);
	bufferQueue->writtenBytes -= bytesCount + sizeof(int);
	bufferQueue->readBytes += bytesCount + sizeof(int);
	debug_pthread_mutex_unlock(&bufferQueue->readBytesLock);
	debug_pthread_mutex_unlock(&bufferQueue->writtenBytesLock);

    return bytesCount;
}