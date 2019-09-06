#include <stdio.h>
#include <malloc.h>
#include <memory.h>
/* 
	[30][30 bytes][1][1 byte][4][4 bytes][-100][100 free bytes logically deleted][10][10 bytes]
*/
typedef unsigned char byte;
struct BufferQueue
{
	byte *start;
	byte *end;
	int usedSize;
	int totalSize;
};

/* 
	Creates a Queue that is stored in a fixed length memory block 
	Size is given in bytes
*/
struct BufferQueue* CreateBuffer(int size)
{
	BufferQueue* bufferQueue = (BufferQueue*)malloc(sizeof(BufferQueue));
	if (bufferQueue == NULL)
	{
		return NULL;
	}
	bufferQueue->end = bufferQueue->start = (byte*)malloc(size * sizeof(byte));
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

/* Writes any data to buffer, just need their total length in bytes */
void WriteData(struct BufferQueue* bufferQueue, void* data, int length)
{
	for (int i = 0; i < length; i++)
	{
		*(bufferQueue->end++) = *((byte*)data + i);
	}
}

/*
	Given the buffer to insert, a pointer to data and the data length (in bytes) this method inserts it into the buffer queue.
	An integer is added as a header to every buffer indicating their length
	The resulting integer is whether the operation worked or the buffer has not enough capacity for the new data
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
	WriteData(buffer, &totalRequiredSize, sizeof(int));
	WriteData(buffer, data, dataLength);
	buffer->usedSize += bytesCount;
	return 1;
}

void ReadData(struct BufferQueue* bufferQueue, void* buffer, int length)
{
	for (int i = 0; i < length; i++)
	{
		*((byte*)buffer + i) = *(bufferQueue->start++);
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
	bufferQueue->usedSize -= bytesCount;
	return bytesCount;
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

	printf("\nBuffer usage = %.2f%%\n\n", (float) bufferQueue->usedSize / bufferSize);

	for (void* it = bufferQueue->start; it < bufferQueue->end;)
	{
		int bytesCount = *(int*)it;
		printf("Block allocated with %i bytes\n", bytesCount);
		printf("Data (raw bytes): ");
		for (int i = 0; i < bytesCount; i++)
		{
			putc(*((byte*)it + i), stdout);
		}
		printf("\n");
		it = (byte*)it + bytesCount;
	}
}

int main()
{
	BufferQueue* queue = CreateBuffer(1024);
	const char* str = "My name is Lorenzo";
	Enqueue(queue, (void*) str, 19 * sizeof(byte));
	int data[] = { 0,1,2,3,4,5,6,7,8,9,10 };
	Enqueue(queue, data, 11 * sizeof(int));
	PrintBufferQueue(queue);
}