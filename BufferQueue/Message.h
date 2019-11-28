#ifndef MESSAGE
#define MESSAGE
#include "Types.h"
#include "BufferQueue.h"

enum MessageType
{
    ReadMessageType,
    WriteMessageType
};

struct Message
{
    /* 
        Depends on MessageType:
        Read Messages  - buf is the memory location where thread should copy block read from disk
        Write Messages - buf is the memory location where thread should get data from to write in disk
    */
    enum MessageType messageType;
    struct BufferQueue * clientBuffer;
    int diskBlock;
    int id;
    byte* buf;
};

struct Message* CreateMessage(enum MessageType messageType, struct BufferQueue * buffer, int diskBlock, int id, byte* buf);
#endif