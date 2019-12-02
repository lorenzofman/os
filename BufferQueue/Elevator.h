#ifndef ELEVATOR
#define ELEVATOR
#include <stdbool.h>
#include "Message.h"
#include "DiskScheduler.h"
struct Elevator
{
    int previousBlock;
    int pendingMessages;
    bool lifting;
};

struct Elevator* CreateElevator();

struct Message Escalonate(struct DiskScheduler* scheduler, struct Elevator* elevator);

#endif