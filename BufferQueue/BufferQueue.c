#include <pthread.h>
#include "BufferQueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>

byte* IncrementedPointer(struct BufferQueue* queue, byte* pointer, int amount)
{
	pointer += amount;
	if (pointer > queue->buffer + queue->capacity)
	{
		pointer -= queue->capacity;
	}
	return pointer;
}

void IncrementDequeue(struct BufferQueue* queue, int amount)
{
	queue->dequeue = IncrementedPointer(queue, queue->dequeue, amount);
}


void IncrementEnqueue(struct BufferQueue* queue, int amount)
{
	queue->enqueue = IncrementedPointer(queue, queue->enqueue, amount);
}

/*
	Creates a Queue that is stored in a fixed length memory block
	Size is given in bytes
*/
struct BufferQueue* CreateBuffer(int size)
{
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
	bufferQueue->capacity = size;
	bufferQueue->usedBytes  = 0;
	
	return bufferQueue;
}

void DestroyBuffer(struct BufferQueue* queue)
{
	free(queue->buffer);
	free(queue);
}

/* Writes any data to buffer, using data total length in bytes */
void WriteData(struct BufferQueue* bufferQueue, byte* data, int length)
{
	byte* resultingEnd = bufferQueue->enqueue + length;
	byte* bufferCapacity = bufferQueue->buffer + bufferQueue->capacity;
	int remainingBytes = resultingEnd - bufferCapacity;
	if (remainingBytes > 0)
	{
		memcpy(bufferQueue->enqueue, data, length - remainingBytes);
		
		int dataStart = length - remainingBytes;
		memcpy(bufferQueue->buffer, data + dataStart, remainingBytes);
	}
	else
	{
		memcpy(bufferQueue->enqueue, data, length);
	}
	IncrementEnqueue(bufferQueue, length);
}

/*
	Given the buffer to insert, a pointer to data and the data length (in bytes) this method inserts it into the buffer queue.
	An integer is added as a header to every buffer indicating their length
	The returning integer is whether the operation worked or the buffer has not enough capacity for the new data
*/
int Enqueue(struct BufferQueue* buffer, byte* data, int dataLength)
{
	int bytesCount = sizeof(int);
	int totalRequiredSize = dataLength + bytesCount;
	int freeBufferSpace = buffer->capacity - buffer->usedBytes;
	if (totalRequiredSize > freeBufferSpace)
	{
		return 0;
	}
	WriteData(buffer, (byte*) &dataLength, sizeof(int));
	WriteData(buffer, data, dataLength);
	buffer->usedBytes += totalRequiredSize;
	return 1;
}

void ReadData(struct BufferQueue* bufferQueue, byte* buffer, int length)
{
	byte* resultingStart = bufferQueue->dequeue + length;
	byte* bufferCapacity = bufferQueue->buffer + bufferQueue->capacity;
	int remainingBytes = resultingStart - bufferCapacity;
	if (remainingBytes > 0)
	{
		memcpy(buffer, bufferQueue->dequeue, length - remainingBytes);

		int dataStart = length - remainingBytes;
		memcpy(buffer + dataStart, bufferQueue->buffer, remainingBytes);
	}
	else
	{
		memcpy(buffer, bufferQueue->dequeue, length);
	}
	IncrementDequeue(bufferQueue, length);
}

/*
	Read data from buffer queue into buffer, bufferSize must be passed to program don't write into protected memory
	The actual number of read bytes is returned by the function
*/
int Dequeue(struct BufferQueue* bufferQueue, void* buffer, int bufferSize)
{
	int bytesCount;
	ReadData(bufferQueue, (byte*) &bytesCount, sizeof(int));
	if (bytesCount > bufferSize)
	{
		return 0;
	}
	ReadData(bufferQueue, (byte*)buffer, bytesCount);
	bufferQueue->usedBytes -= bytesCount + sizeof(int);
	return bytesCount;
}


