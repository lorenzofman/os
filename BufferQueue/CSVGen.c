#include "CSVGen.h"
#include "Benchmark.h"
#include <stdio.h>
void CSVThreaded(int minBufferSize, int maxBufferSize, int minBlockSize, int readers, int writers)
{
	printf("%s", "BufferSize; BlockSize; Velocity\n");
    for(long long i = minBufferSize; i <= maxBufferSize; i*=2)
    {
        for(long long j = minBlockSize; j <= i; j *= 2)
		{
			if(i/j >= readers && i/j >= writers)
			{
				long long velocity = ThreadBenchmark(i, j - 4, readers, writers);
				printf("%lli; %lli; %lli\n", i, j, velocity);
			}
		}
	}
}

void CSVNonThreaded(int minBufferSize, int maxBufferSize, int minBlockSize)
{
	printf("%s", "BufferSize; BlockSize; Velocity\n");
    for(long long i = minBufferSize; i <= maxBufferSize; i*=2)
    {
        for(long long j = minBlockSize; j <= i; j *= 2)
		{
			long long velocity = Benchmark(i, j - 4);
			printf("%lli; %lli; %lli\n", i, j, velocity);
		}
	}
}