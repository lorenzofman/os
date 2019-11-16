#ifndef BUFFERQUEUE
#define BUFFERQUEUE
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
typedef unsigned char byte;
struct BufferQueue
{
	byte* buffer; /* Buffer start position (static)*/
	byte* dequeue; /* Dequeue start position */
	byte* enqueue; /* Enqueue start position */
	int usedBytes;
	int capacity;
	int ticket;
	int globalTicket;
	pthread_mutex_t ticketLock;
	pthread_mutex_t globalTicketLock;
};

byte* IncrementedPointer(struct BufferQueue* queue, byte* pointer, int amount);

void IncrementDequeue(struct BufferQueue* queue, int amount);

void IncrementEnqueue(struct BufferQueue* queue, int amount);

struct BufferQueue* CreateBuffer(int size);

void DestroyBuffer(struct BufferQueue* queue);

void WriteData(struct BufferQueue* bufferQueue, byte* data, int length);

int Enqueue(struct BufferQueue* buffer, byte* data, int dataLength);

void ReadData(struct BufferQueue* bufferQueue, byte* buffer, int length);

int Dequeue(struct BufferQueue* bufferQueue, void* buffer, int bufferSize);


#endif // ! BUFFERQUEUE