#include "BufferQueue.h"

struct BufferQueue* CreateBufferThreaded(int size);

int EnqueueThread_NB(struct BufferQueue* buffer, byte* data, int dataLength);

int DequeueThread_NB(struct BufferQueue* bufferQueue, void* buffer, int bufferSize);

int EnqueueThread_B(struct BufferQueue* buffer, byte* data, int dataLength);

int DequeueThread_B(struct BufferQueue* bufferQueue, void* buffer, int bufferSize);