#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <ctype.h>
#include <stdbool.h>

#define BUFFERSIZE 16
#define MAXPACKETS 1024
#define DEBUG
/* 
	[30][30 bytes][1][1 byte][4][4 bytes][10][10 bytes]
*/
typedef unsigned char byte;
struct BufferQueue
{
	byte* buffer; /* Buffer start position (static)*/
	byte* start; /* Dequeue start position */
	byte* end; /* Enqueue start position */
	int usedSize;
	int totalSize;
};

void ClampBufferQueue(struct BufferQueue* queue)
{
	queue->start = (queue->start - queue->buffer) % queue->totalSize + queue->buffer;
	queue->end = (queue->end - queue->buffer) % queue->totalSize + queue->buffer;
}

void IncrementStart(struct BufferQueue* queue)
{
	queue->start++;
	ClampBufferQueue(queue);
}

void IncrementEnd(struct BufferQueue* queue)
{
	queue->end++;
	ClampBufferQueue(queue);
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
	for (int i = 0; i < length; i++)
	{
		*bufferQueue->end = *(data + i);
		IncrementEnd(bufferQueue);
	}
}

/*
	Given the buffer to insert, a pointer to data and the data length (in bytes) this method inserts it into the buffer queue.
	An integer is added as a header to every buffer indicating their length
	The returning integer is whether the operation worked or the buffer has not enough capacity for the new data
*/
int Enqueue(struct BufferQueue* buffer, void* data, int dataLength)
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
	for (int i = 0; i < length; i++)
	{
		*(buffer + i) = *bufferQueue->start;
		IncrementStart(bufferQueue);
	}
}

/* 
	Read data from buffer queue into buffer, bufferSize must be passed to program don't write into protected memory
	The actual number of read bytes is returned by the function
*/
int Dequeue(struct BufferQueue* bufferQueue, void * buffer, int bufferSize)
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
/* Prints the buffer array with colors */
void PrintBufferRawBytes(struct BufferQueue* queue)
{
	bool freeArea = true;
	bool started = false;
	bool ended = false;
	int rawBytesToRead = 0;
	for (int i = 0; i < queue->totalSize; i++)
	{
		if ((queue->buffer + i) == queue->start && started == false)
		{
			freeArea = false;
			started = true;
			printf("<");
		}
		if ((queue->buffer + i) == queue->end && ended == false)
		{
			freeArea = true;
			ended = true;
			printf(">");
		}
		
		if (freeArea)
		{
			printf("#");
		}
		else 
		{
			if (rawBytesToRead == 0)
			{
				byte* pos = queue->buffer + i;
				rawBytesToRead = *(int*) pos;
				printf("|%i|", rawBytesToRead);
				i += sizeof(int) - 1;
			}
			else 
			{
				printf("%c", *(queue->buffer + i));
				rawBytesToRead--;
			}
		}
	}
	printf("\n");
}

void PrintBufferQueue(struct BufferQueue* bufferQueue)
{
	int offset = 0;
	int bufferSize = bufferQueue->totalSize;
	if (bufferSize < 1024)
	{
		printf("Buffer size = %iB\n", bufferSize);
	}
	else if (bufferSize < 1024 * 1024)
	{
		printf("Buffer size = %iKB\n", bufferSize/1024);
	}
	else if (bufferSize < 1024 * 1024 * 1024)
	{
		printf("Buffer size = %iMB\n", bufferSize / (1024 * 1024));
	}
	else 
	{
		printf("Buffer size = %iGB\n", bufferSize / (1024 * 1024 * 1024));
	}

	printf("\nBuffer usage = %.2f%%\n\n",100 * (float) bufferQueue->usedSize / bufferSize);
}

int main()
{
	char aBuf[3];
	char bBuf[4];
	char cBuf[3];
	struct BufferQueue* queue = CreateBuffer(BUFFERSIZE);
	Enqueue(queue, "aaa", 3);
	PrintBufferRawBytes(queue);

	Enqueue(queue, "bbbb", 4);
	PrintBufferRawBytes(queue);

	Dequeue(queue, aBuf, 3);
	PrintBufferRawBytes(queue);

	Enqueue(queue, "ccc", 3);
	PrintBufferRawBytes(queue);

	Dequeue(queue, bBuf, 4);
	PrintBufferRawBytes(queue);

	Dequeue(queue, cBuf, 3);
	PrintBufferRawBytes(queue);

}