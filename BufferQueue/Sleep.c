#include "Sleep.h"

#include <time.h>
void Sleep(int nanoseconds)
{
	struct timespec ts = { 0, nanoseconds };
	nanosleep(&ts, NULL);
}