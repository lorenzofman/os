#include "Message.h"
#include <stdlib.h>
struct Message* CreateMessage(enum MessageType messageType, struct BufferQueue * clientBuffer, int diskBlock, int id, byte* buf)
{
    struct Message* message = (struct Message*) malloc(sizeof(struct Message));
    message->messageType = messageType;
    message->clientBuffer = clientBuffer;
    message->diskBlock = diskBlock;
    message->id = id;
    message->buf = buf;
    return message;
}