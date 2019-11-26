#ifndef MESSAGE
#define MESSAGE
#include "Types.h"

enum MessageType
{
    ReadMessageType,
    WriteMessageType
};

struct Message
{
    int safe;
    enum MessageType messageType;
    /* 
        Depends on MessageType:
        Read Messages  - buffer is the memory location where thread should copy block read from disk
        Write Messages - buffer is the memory location where thread should get data from to write in disk
    */
    byte * buffer;
    int diskBlock;
    int id;
};

#endif