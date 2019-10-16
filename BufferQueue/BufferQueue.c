#include "BufferQueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>

void IncrementStart(struct BufferQueue* queue, int amount)
{
	queue->start += amount;
	queue->start = (queue->start - queue->buffer) % queue->totalSize + queue->buffer;
}


void IncrementEnd(struct BufferQueue* queue, int amount)
{
	queue->end += amount;
	queue->end = (queue->end - queue->buffer) % queue->totalSize + queue->buffer;
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
	bufferQueue->buffer = bufferQueue->end = bufferQueue->start = (byte*)malloc(size * sizeof(byte));
	if (bufferQueue->end == NULL)
	{
		free(bufferQueue);
		return NULL;
	}
	memset(bufferQueue->start, 0, size);
	bufferQueue->totalSize = size;
	bufferQueue->usedSize = 0;
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
	byte* resultingEnd = bufferQueue->end + length;
	byte* realEnd = bufferQueue->buffer + bufferQueue->totalSize;
	byte* remainingBytes = resultingEnd - bufferQueue->end;
	if (resultingEnd > realEnd)
	{
		memcpy(bufferQueue->buffer, data + (int)remainingBytes, realEnd - resultingEnd);
	}
	memcpy(bufferQueue->end, data, min(remainingBytes, length));

	IncrementEnd(bufferQueue, length);
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
	int freeBufferSpace = buffer->totalSize - buffer->usedSize;
	if (totalRequiredSize > freeBufferSpace)
	{
		return 0;
	}
	WriteData(buffer, &dataLength, sizeof(int));
	WriteData(buffer, data, dataLength);
	buffer->usedSize += totalRequiredSize;
	return 1;
}

void ReadData(struct BufferQueue* bufferQueue, byte* buffer, int length)
{
	byte* resultingStart = bufferQueue->start + length;
	byte* realEnd = bufferQueue->buffer + bufferQueue->totalSize;
	byte* remainingBytes = resultingStart - bufferQueue->end;
	if (resultingStart > realEnd)
	{
		memcpy(buffer + (int)remainingBytes, bufferQueue->buffer, realEnd - resultingStart);
	}
	memcpy(buffer, bufferQueue->start, min(remainingBytes, length));

	IncrementStart(bufferQueue, length);
}

/*
	Read data from buffer queue into buffer, bufferSize must be passed to program don't write into protected memory
	The actual number of read bytes is returned by the function
*/
int Dequeue(struct BufferQueue* bufferQueue, void* buffer, int bufferSize)
{
	int bytesCount;
	ReadData(bufferQueue, &bytesCount, sizeof(int));
	if (bytesCount > bufferSize)
	{
		return 0;
	}
	ReadData(bufferQueue, buffer, bytesCount);
	bufferQueue->usedSize -= bytesCount + sizeof(int);
	return bytesCount;
}