#ifndef BUFFER_QUEUE_THREAD
#define BUFFER_QUEUE_THREAD

#include "BufferQueue.h"
#include <stdbool.h>

struct BufferQueue* CreateBufferThreaded(int size);

void DestroyBufferQueueThreaded(struct BufferQueue* bufferQueue);

int EnqueueThread_NB(struct BufferQueue* buffer, byte* data, int dataLength);

int DequeueThread_NB(struct BufferQueue* bufferQueue, void* buffer, int bufferSize);

int EnqueueThread_B(struct BufferQueue* buffer, byte* data, int dataLength);

int DequeueThread_B(struct BufferQueue* bufferQueue, void* buffer, int bufferSize);

bool Empty(struct BufferQueue* bufferQueue);

bool Fits(struct BufferQueue* bufferQueue, int size);
#endif