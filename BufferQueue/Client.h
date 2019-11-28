#ifndef CLIENT
#define CLIENT
#include <stdlib.h>
#include <stdbool.h>
#include "BufferQueue.h"

struct Client
{
    struct BufferQueue * buffer;
};
struct Client* CreateClient(struct BufferQueue* bufferQueue);
void CopyFileToDisk(struct Client *client, struct DiskScheduler *scheduler, const char* filename, bool sequential);
#endif