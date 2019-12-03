#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "CSVGen.h"
#include "Benchmark.h"

/* Convert string to integer (using base 10)*/
int ExtractInt(char* str)
{
    return(strtol(str, NULL, 10));
}

char* ExecuteBenchmark(int argc, char *argv[])
{
    if(argc != 3)
    {
        return "Invalid arguments to benchmark. Please Use -t (threaded) or -nt (non-threaded) and inform BufferSize, BlockSize";
    }
    
    char* firstArg = argv[0];

    int bufferSize = ExtractInt(argv[1]);
    int blockSize = ExtractInt(argv[2]);

    if(strcmp(firstArg, "-t") == 0)
    {
        ThreadBenchmark(bufferSize, blockSize);
        return "Threaded Benchmark succesfull\n";
    }

    if(strcmp(firstArg, "-nt") == 0)
    {
        Benchmark(bufferSize, blockSize);
        return "Benchmark succesfull\n";
    }
    
    return "Invalid parameter benchmark. Rerun with -t for threaded and -nt for non-threaded \n";
   
}

char* GenerateCSV(int argc, char *argv[])
{
    if(argc != 4)
    {
        return "Invalid arguments to generate CSV. Please Use -t or -nt and inform MinBufferSize, MaxBufferSize and MinBlockSize\n";
    }
    int minBufSize = ExtractInt(argv[1]);
    int maxBufSize = ExtractInt(argv[2]);
    int minBlockSize = ExtractInt(argv[3]);
    char* firstArg = argv[0];

    if(strcmp(firstArg, "-t") == 0)
    {
        CSVThreaded(minBufSize, maxBufSize, minBlockSize);
        return "Threaded CSV generated with success\n";
    }

    if(strcmp(firstArg, "-nt") == 0)
    {
        CSVNonThreaded(minBufSize, maxBufSize, minBlockSize);
        return "CSV generated with success\n";
    }
    
    return "Invalid parameter for CSV. Rerun with -t for threaded and -nt for non-threaded \n";
}

char* Execute(int argc, char *argv[])
{
    if(argc == 0)
    {
        return "No arguments passed to program\n";
    }

    char* firstArg = argv[0];

    if(strcmp(firstArg, "-csv") == 0)
    {
        return GenerateCSV(argc - 1, argv + 1);
    }

    if(strcmp(firstArg, "-benchmark") == 0)
    {
        return ExecuteBenchmark(argc - 1, argv + 1);
    }
}

int main(int argc, char *argv[])
{
    char* returnedMessage = Execute(argc, argv);
    printf(returnedMessage);
}