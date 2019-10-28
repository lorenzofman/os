#include "BufferQueueThread.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
/* Writes any data to buffer, using data total length in bytes */
void WriteDataThread(struct BufferQueue* bufferQueue, byte* data, int length)
{
	byte* resultingEnd = bufferQueue->end + length;
	byte* bufferCapacity = bufferQueue->buffer + bufferQueue->capacity;
	int remainingBytes = resultingEnd - bufferCapacity;
	if (remainingBytes > 0)
	{
		memcpy(bufferQueue->end, data, length - remainingBytes);
		
		int dataStart = length - remainingBytes;
		memcpy(bufferQueue->buffer, data + dataStart, remainingBytes);
	}
	else
	{
		memcpy(bufferQueue->end, data, length);
	}
	IncrementEnd(bufferQueue, length);
}

/*
	Given the buffer to insert, a pointer to data and the data length (in bytes) this method inserts it into the buffer queue.
	An integer is added as a header to every buffer indicating their length
	The returning integer is whether the operation worked or the buffer has not enough capacity for the new data
*/
int EnqueueThread(struct BufferQueue* bufferQueue, byte* data, int dataLength)
{
    pthread_mutex_lock(&bufferQueue->enqueueLock);
	int bytesCount = sizeof(int);
	int totalRequiredSize = dataLength + bytesCount;
	int freeBufferSpace = bufferQueue->capacity - bufferQueue->usedSize;
	if (totalRequiredSize > freeBufferSpace)
	{
        pthread_mutex_unlock(&bufferQueue->enqueueLock);
		return 0;
	}
	WriteData(bufferQueue, (byte*) &dataLength, sizeof(int));
	WriteData(bufferQueue, data, dataLength);
	bufferQueue->usedSize += totalRequiredSize;
    pthread_mutex_unlock(&bufferQueue->enqueueLock);
	return 1;
}

void ReadDataThread(struct BufferQueue* bufferQueue, byte* buffer, int length)
{
	byte* resultingStart = bufferQueue->start + length;
	byte* bufferCapacity = bufferQueue->buffer + bufferQueue->capacity;
	int remainingBytes = resultingStart - bufferCapacity;
	if (remainingBytes > 0)
	{
		memcpy(buffer, bufferQueue->start, length - remainingBytes);

		int dataStart = length - remainingBytes;
		memcpy(buffer + dataStart, bufferQueue->buffer, remainingBytes);
	}
	else
	{
		memcpy(buffer, bufferQueue->start, length);
	}
	IncrementStart(bufferQueue, length);
}

/*
	Read data from buffer queue into buffer, bufferSize must be passed to program don't write into protected memory
	The actual number of read bytes is returned by the function
*/
int DequeueThread(struct BufferQueue* bufferQueue, void* buffer, int bufferSize)
{
    pthread_mutex_lock(&bufferQueue->dequeueLock);
	int bytesCount;
	ReadData(bufferQueue, (byte*) &bytesCount, sizeof(int));
	if (bytesCount > bufferSize)
	{
        pthread_mutex_unlock(&bufferQueue->dequeueLock);
		return 0;
	}
	ReadData(bufferQueue, (byte*)buffer, bytesCount);
	bufferQueue->usedSize -= bytesCount + sizeof(int);
	pthread_mutex_unlock(&bufferQueue->dequeueLock);
    return bytesCount;
}