COMPILER=gcc
ARGS=-std=c11 -pthread -lm -Wall -O3
BUFFERQUEUE=BufferQueue.out
DISK=Disk.out
all: Disk BufferQueueBenchmark
Disk: BufferQueue.o BufferQueueThread.o Disk.o DiskScheduler.o DiskTest.c Sleep.o Message.o Client.o Elevator.o Constants.h
	$(COMPILER) BufferQueue.o BufferQueueThread.o Elevator.o Disk.o DiskScheduler.o Sleep.o Message.o Client.o Constants.h DiskTest.c -o $(DISK) $(ARGS)

BufferQueueBenchmark: BufferQueueThread.o BufferQueue.o Sleep.o Benchmark.c 
	$(COMPILER) BufferQueueThread.o BufferQueue.o Sleep.o Benchmark.c -o $(BUFFERQUEUE) $(ARGS)

DiskScheduler.o: DiskScheduler.c
	$(COMPILER) -c DiskScheduler.c $(ARGS)

Elevator.o: Elevator.c
	$(COMPILER) -c Elevator.c $(ARGS)

Disk.o: Disk.c
	$(COMPILER) -c Disk.c $(ARGS)

BufferQueueThread.o: BufferQueueThread.c
	$(COMPILER) -c BufferQueueThread.c $(ARGS)

BufferQueue.o: BufferQueue.c
	$(COMPILER) -c BufferQueue.c $(ARGS)

Client.o: Client.c
	$(COMPILER) -c Client.c $(ARGS)
	
Message.o: Message.c
	$(COMPILER) -c Message.c $(ARGS)

Sleep.o: Sleep.c
	$(COMPILER) -c Sleep.c $(ARGS)

runBuffer: 
	./$(BUFFERQUEUE)
runBufferThreads: all
	./$(BUFFERQUEUE) -t

runDisk:
	./$(DISK)
clean: 
	rm *.o