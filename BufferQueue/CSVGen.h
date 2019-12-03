#ifndef CSV
#define CSV

void CSVThreaded(int minBufferSize, int maxBufferSize, int minBlockSize, int readers, int writers);
void CSVNonThreaded(int minBufferSize, int maxBufferSize, int minBlockSize);

#endif