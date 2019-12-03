#include "Utils.h"
#include <stdlib.h>
#include <stdio.h>
char* FormattedBytes(long long i, char* buffer)
{
	if (buffer == NULL)
	{
		return "ERROR";
	}
	if (i < 1024)
	{
		sprintf(buffer, "%lldB", i);
	}
	else if (i < 1024 * 1024)
	{
		sprintf(buffer, "%lldKB", i / 1024);
	}
	else if (i < 1024 * 1024 * 1024)
	{
		sprintf(buffer, "%lldMB", i / (1024 * 1024));
	}
	else
	{
		sprintf(buffer, "%.5lfGB", (double) i / ((double)1024 * 1024 * 1024));
	}
	return buffer;
}