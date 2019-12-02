#include "Elevator.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "Message.h"
#include "DiskScheduler.h"
#include "BufferQueueThread.h"
#include "Disk.h"
#include "Utils.h"

struct Elevator* CreateElevator()
{
    struct Elevator* elevator = (struct Elevator*) malloc(sizeof(struct Elevator));
    elevator->previousBlock = 0;
    elevator->lifting = true;
    elevator->pendingMessages = 0;
}

int ElevatorDistance(struct DiskScheduler* scheduler, struct Message message, int lastBlock, bool lifting)
{
    int infinity = scheduler->disk->blocks;
    int block = message.diskBlock;
    if(lifting && block < lastBlock)
    {
        return infinity;
    }

    if(lifting == false && block > lastBlock)
    {
        return infinity;
    }
    int dif = lastBlock - block;
    return ABS(dif);
}

int BestMessage(struct DiskScheduler* scheduler, struct Elevator* elevator)
{
    int infinity = scheduler->disk->blocks;
    int closestBlockDif = infinity;
    int bestMessage = -1;
    for(int i = 0; i < elevator->pendingMessages; i++)
    {
        //printf("Pending message %i {block::%i}\n", i, scheduler->messageRequests[i].diskBlock);
        int dif = ElevatorDistance(scheduler, scheduler->messageRequests[i], elevator->previousBlock, elevator->lifting);
        if(dif < closestBlockDif)
        {
            closestBlockDif = dif;
            bestMessage = i;
        }
    }
    //printf("Best message is %i blocks away\n", closestBlockDif);
    return bestMessage;
}

struct Message Escalonate(struct DiskScheduler* scheduler, struct Elevator* elevator)
{
    uint messageSize = sizeof(struct Message);
    /* Fetch all messages first */
    while(Empty(scheduler->receiver) == false || elevator->pendingMessages == 0)
    {
        DequeueThread_B(scheduler->receiver, scheduler->messageRequests + elevator->pendingMessages * messageSize, messageSize);
        elevator->pendingMessages++;
    }
    int closestIdx = BestMessage(scheduler, elevator);
    if(closestIdx == -1)
    {
        elevator->lifting = !elevator->lifting; 
        closestIdx = BestMessage(scheduler, elevator);
    }
    struct Message closestRequest = scheduler->messageRequests[closestIdx];
    
    for(int i = closestIdx; i < elevator->pendingMessages; i++)
    {
        scheduler->messageRequests[i] = scheduler->messageRequests[i+1]; 
    }
    elevator->pendingMessages--;
    elevator->previousBlock = closestRequest.diskBlock;
    return closestRequest;
}