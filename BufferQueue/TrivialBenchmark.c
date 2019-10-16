#include "BufferQueue.h"
#include <stdio.h>
#include <Windows.h>
#define BUFFERSIZE 1024 * 1024
#define BLOCKSIZE (1024 - 4) // 1KB - Header

char* SizeString(long long i)
{
	char* buffer = malloc(sizeof(char) * 5);
	if (buffer == NULL)
	{
		return "ERROR";
	}
	if (i < 1024)
	{
		sprintf(buffer, "%lldB", i);
	}
	else if (i < 1024 * 1024)
	{
		sprintf(buffer, "%lldKB", i / 1024);
	}
	else if (i < 1024 * 1024 * 1024)
	{
		sprintf(buffer, "%lldMB", i / (1024 * 1024));
	}
	else
	{
		sprintf(buffer, "%lfGB", (double) i / (1024 * 1024 * 1024));
	}
	return buffer;
}

int main()
{
	struct BufferQueue* queue = CreateBuffer(BUFFERSIZE);
	int blocks = BUFFERSIZE / BLOCKSIZE;
	byte* data = malloc(BLOCKSIZE * sizeof(byte));

	LARGE_INTEGER frequency;
	LARGE_INTEGER t1, t2;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&t1);

	for (int i = 0; i < blocks; i++)
	{
		Enqueue(queue, data, BLOCKSIZE);
	}

	for (int i = 0; i < blocks; i++)
	{
		Dequeue(queue, data, BLOCKSIZE);
	}

	QueryPerformanceCounter(&t2);
	double elapsedTime = (double)(t2.QuadPart - t1.QuadPart) / frequency.QuadPart;
	double result = (double)BUFFERSIZE / elapsedTime;
	long long throughput = (long long) (result);
	printf("Velocity = %s/s\n", SizeString(throughput));
}