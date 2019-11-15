#include "BufferQueue.h"

struct BufferQueue* CreateBufferThreaded(int size);

void WriteDataAsync(struct BufferQueue* bufferQueue, byte * enqueuePosition, byte* data, int length);

int EnqueueThread(struct BufferQueue* buffer, byte* data, int dataLength);

void ReadDataAsync(struct BufferQueue* bufferQueue, byte* dequeuePosition, byte* buffer, int length);

int DequeueThread(struct BufferQueue* bufferQueue, void* buffer, int bufferSize);