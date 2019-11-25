#ifndef Sleep
#define Sleep
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32

void _Sleep(int nanoseconds)
{
#ifdef _WIN32
	Sleep(nanoseconds/(1000 * 1000));
#else
	struct timespec ts = { 0, nanoseconds };
	nanosleep(&ts, NULL);
#endif
}

#endif