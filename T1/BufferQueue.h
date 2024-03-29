#ifndef BUFFERQUEUE
#define BUFFERQUEUE
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include "Types.h"
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

	int readTicket;
	int globalReadTicket;
	pthread_mutex_t readTicketLock;

	int writeTicket;
	int globalWriteTicket;
	pthread_mutex_t writeTicketLock;

	int pendingReads;
	int pendingWrites;

	pthread_cond_t ticketUpdate;
	pthread_cond_t writeTicketUpdate;
	pthread_cond_t readTicketUpdate;

	pthread_mutex_t ticketCondMutex;
	pthread_mutex_t writeTicketCondMutex;
	pthread_mutex_t readTicketCondMutex;

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