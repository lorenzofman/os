#include "Sleep.h"
#include <time.h>

void Sleep(int nanoseconds)
{
	struct timespec ts = { nanoseconds / (1000 * 1000 * 1000), nanoseconds % (1000 * 1000 * 1000)};
	nanosleep(&ts, NULL);
}