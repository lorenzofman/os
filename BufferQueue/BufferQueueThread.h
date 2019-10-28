#include "BufferQueue.h"
void WriteDataThread(struct BufferQueue* bufferQueue, byte* data, int length);

int EnqueueThread(struct BufferQueue* buffer, byte* data, int dataLength);

void ReadDataThread(struct BufferQueue* bufferQueue, byte* buffer, int length);

int DequeueThread(struct BufferQueue* bufferQueue, void* buffer, int bufferSize);