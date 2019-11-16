#include "BufferQueue.h"

struct BufferQueue* CreateBufferThreaded(int size);

int EnqueueThread(struct BufferQueue* buffer, byte* data, int dataLength);

int DequeueThread(struct BufferQueue* bufferQueue, void* buffer, int bufferSize);