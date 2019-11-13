#include "BufferQueueThread.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <stdbool.h>
#include <stdarg.h>

#define DEBUG
#define S_WAIT_TIME 1
#define WAIT_TIME 0

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
	pthread_mutex_init(&bufferQueue->dequeueLock, NULL);
	pthread_mutex_init(&bufferQueue->enqueueLock, NULL);
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
			struct timespec ts = {S_WAIT_TIME, WAIT_TIME};
			nanosleep(&ts, NULL);
		}
	}
}


bool FullBuffer(struct BufferQueue* bufferQueue, int amount, int idx)
{
	pthread_mutex_lock(&bufferQueue->usedBytesLock);
	int freeBufferSpace = bufferQueue->capacity - bufferQueue->usedBytes;
	int headerSize = sizeof(int);
	int totalRequiredSize = amount;
	if (totalRequiredSize > freeBufferSpace || bufferQueue->readBytes < totalRequiredSize)
	{
        pthread_mutex_unlock(&bufferQueue->usedBytesLock);
		return false;
	}
	bufferQueue->usedBytes += amount;
	SyncPrintf("W%i) Used bytes: %i\n", idx, bufferQueue->usedBytes);
	pthread_mutex_unlock(&bufferQueue->usedBytesLock);
	return true;
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
	int myWriteTicket = GetMyTicket(&writeTicket, &writeTicketMutex);
	SyncPrintf("W%i) Fetching ticket: %i\n", idx, myWriteTicket);
	WaitTicketTurn(myWriteTicket, &writeBeginGlobalTicket);
	SyncPrintf("W%i) My turn: %i\n", idx, myWriteTicket);

	int headerSize = sizeof(int);
	int totalSize = dataLength + headerSize;
	SyncPrintf("W%i) Inserting %i bytes + %i header bytes\n", idx, dataLength, headerSize);	
	while(FullBuffer(bufferQueue, totalSize, idx) == false)
	{
		SyncPrintf("W%i) Full buffer, waiting %i s and %i ns\n", idx, S_WAIT_TIME, WAIT_TIME);
		struct timespec ts = {S_WAIT_TIME, WAIT_TIME};
		nanosleep(&ts, NULL);
	}
	SyncPrintf("W%i) Buffer has capacity\n", idx, dataLength, headerSize);
	pthread_mutex_lock(&bufferQueue->enqueueLock);
	byte* bufferEnqueue = bufferQueue->enqueue;
	IncrementEnqueue(bufferQueue, totalSize);
	SyncPrintf("W%i) Incrementing enqueue position for next writers\n", idx);
	pthread_mutex_unlock(&bufferQueue->enqueueLock);

	SyncPrintf("W%i) Incrementing write begin ticket\n", idx);
	IncrementTicket(&writeBeginGlobalTicket, &writeBeginGlobalTicketMutex); // Allowing the next writer to enter

	WriteDataAsync(bufferQueue, bufferEnqueue, (byte*) &dataLength, headerSize);
	SyncPrintf("W%i) Writing header async\n", idx);
	bufferEnqueue = IncrementedPointer(bufferQueue, bufferEnqueue, headerSize);
	WriteDataAsync(bufferQueue, bufferEnqueue, data, dataLength);
	SyncPrintf("W%i) Writing data async\n", idx);


	SyncPrintf("W%i) Waiting to update write in order\n", idx);
	SyncPrintf("W%i) My index: %i, global index: %i\n", idx, myWriteTicket, writeBeginGlobalTicket);
	WaitTicketTurn(myWriteTicket, &writeFinishGlobalTicket);
	SyncPrintf("W%i) Turn is mine\n", idx);	
	pthread_mutex_lock(&bufferQueue->writtenBytesLock);
	pthread_mutex_lock(&bufferQueue->readBytesLock);
	SyncPrintf("W%i) Updating values\n", idx);	
	bufferQueue->writtenBytes += totalSize;
	bufferQueue->readBytes -= totalSize;
	SyncPrintf("W%i) Used bytes: %i, WrittenBytes: %i, ReadBytes: %i\n", idx, bufferQueue->usedBytes, bufferQueue->writtenBytes, bufferQueue->readBytes);
	SyncPrintf("W%i) Incrementing write finish ticket\n", idx);
	IncrementTicket(&writeFinishGlobalTicket, &writeFinishGlobalTicketMutex);
	pthread_mutex_unlock(&bufferQueue->readBytesLock);
	pthread_mutex_unlock(&bufferQueue->writtenBytesLock);
	SyncPrintf("W%i) Insertion terminated succesfully\n", idx);
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


int DequeueThread(struct BufferQueue* bufferQueue, void* buffer, int bufferSize, int idx)
{
	int myTicket = GetMyTicket(&readTicket, &readTicketMutex);
	SyncPrintf("R%i) Fetching ticket: %i Global ticket: %i\n", idx, myTicket, readBeginGlobalTicket);
	WaitTicketTurn(myTicket, &readBeginGlobalTicket);
	SyncPrintf("R%i) My turn: %i\n", idx, myTicket);

	while(true)
	{
		pthread_mutex_lock(&bufferQueue->usedBytesLock);
		if(bufferQueue->usedBytes > 0)
		{
			SyncPrintf("R%i) Buffer isn't empty, continuing\n", idx);
			pthread_mutex_unlock(&bufferQueue->usedBytesLock);
			break;
		}
		
		SyncPrintf("R%i) Empty buffer, waiting %i s and %i ns\n", idx, S_WAIT_TIME, WAIT_TIME);
		pthread_mutex_unlock(&bufferQueue->usedBytesLock);
		struct timespec ts = {S_WAIT_TIME, WAIT_TIME};
		nanosleep(&ts, NULL);
	}

	int bytesCount;
	byte* dequeuePosition;
	
	while(bufferQueue->writtenBytes < sizeof(int))
	{
		struct timespec ts = {S_WAIT_TIME, WAIT_TIME};
		nanosleep(&ts, NULL);
	}
	pthread_mutex_lock(&bufferQueue->dequeueLock);
	SyncPrintf("R%i) Locking dequeue\n", idx);
	dequeuePosition = bufferQueue->dequeue;
	SyncPrintf("R%i) Reading length of block\n", idx);
	ReadDataAsync(bufferQueue, dequeuePosition, (byte*) &bytesCount, sizeof(int));
	SyncPrintf("R%i) Block length: %i\n", idx, bytesCount);

	while(true)
	{
		if(bytesCount < bufferQueue->writtenBytes)
		{
			SyncPrintf("R%i) Data is written\n", idx);
			pthread_mutex_unlock(&bufferQueue->writtenBytesLock);
			break;
		}
		SyncPrintf("R%i) Data not written yet, waiting %i ns\n", idx, WAIT_TIME);
		SyncPrintf("R%i) WrittenBytes: %i, BytesCount: %i, UsedBytes: %i\n", idx, bufferQueue->writtenBytes, bytesCount, bufferQueue->usedBytes);
		pthread_mutex_unlock(&bufferQueue->writtenBytesLock);
		struct timespec ts = {S_WAIT_TIME, WAIT_TIME};
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
	SyncPrintf("R%i) Unlocking dequeue and usedBytes locks\n", idx);
	SyncPrintf("R%i) Incrementing read begin ticket, global ticket new value = %i\n", idx, readBeginGlobalTicket + 1);
	IncrementTicket(&readBeginGlobalTicket, &readBeginGlobalTicketMutex);
	pthread_mutex_unlock(&bufferQueue->dequeueLock);
	pthread_mutex_unlock(&bufferQueue->usedBytesLock);

	SyncPrintf("R%i) Reading data async\n", idx);
	byte* readPos = IncrementedPointer(bufferQueue, dequeuePosition, sizeof(int));
	ReadDataAsync(bufferQueue, readPos, (byte*) &bytesCount, sizeof(int));

	SyncPrintf("R%i) Waiting ticket to update read/written bytes\n", idx);
	WaitTicketTurn(myTicket, &readFinishGlobalTicket);
	pthread_mutex_lock(&bufferQueue->writtenBytesLock);
	pthread_mutex_lock(&bufferQueue->readBytesLock);
	SyncPrintf("R%i) Updating read/written bytes\n", idx);

	SyncPrintf("R%i) [BEFORE] Used bytes: %i, WrittenBytes: %i, ReadBytes: %i\n", idx, bufferQueue->usedBytes, bufferQueue->writtenBytes, bufferQueue->readBytes);			
	
	bufferQueue->writtenBytes -= bytesCount + sizeof(int);
	bufferQueue->readBytes += bytesCount + sizeof(int);
	SyncPrintf("R%i) BytesCount: %i\n", idx, bytesCount);			
	SyncPrintf("R%i) [AFTER] Used bytes: %i, WrittenBytes: %i, ReadBytes: %i\n", idx, bufferQueue->usedBytes, bufferQueue->writtenBytes, bufferQueue->readBytes);			
	
	IncrementTicket(&readFinishGlobalTicket, &readFinishGlobalTicketMutex);
	pthread_mutex_unlock(&bufferQueue->readBytesLock);
	pthread_mutex_unlock(&bufferQueue->writtenBytesLock);
	SyncPrintf("R%i) Read succesfull\n", idx);
    return bytesCount;
}