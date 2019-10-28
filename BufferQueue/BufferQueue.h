#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#ifndef BUFFERQUEUE
#define BUFFERQUEUE

typedef unsigned char byte;
struct BufferQueue
{
	byte* buffer; /* Buffer start position (static)*/
	byte* start; /* Dequeue start position */
	byte* end; /* Enqueue start position */
	int usedSize;
	int capacity;
};

void IncrementStart(struct BufferQueue* queue, int amount);

void IncrementEnd(struct BufferQueue* queue, int amount);

struct BufferQueue* CreateBuffer(int size);

void DestroyBuffer(struct BufferQueue* queue);

void WriteData(struct BufferQueue* bufferQueue, byte* data, int length);

int Enqueue(struct BufferQueue* buffer, byte* data, int dataLength);

void ReadData(struct BufferQueue* bufferQueue, byte* buffer, int length);

int Dequeue(struct BufferQueue* bufferQueue, void* buffer, int bufferSize);


void WriteData(struct BufferQueue* bufferQueue, byte* data, int length);

int Enqueue(struct BufferQueue* buffer, byte* data, int dataLength);

void ReadData(struct BufferQueue* bufferQueue, byte* buffer, int length);

int Dequeue(struct BufferQueue* bufferQueue, void* buffer, int bufferSize);

#endif // ! BUFFERQUEUE