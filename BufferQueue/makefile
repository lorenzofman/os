COMPILER=gcc
ARGS=-std=c11 -pthread -lm -Wall -O3
OUT=t1.out

all: Disk BufferQueueBenchmark

main: Benchmark.o BufferQueue.o BufferQueueThread.o Client.o Disk.o DiskScheduler.o Elevator.o Message.o Sleep.o Utils.o main.c
	$(COMPILER) Benchmark.o BufferQueue.o BufferQueueThread.o Client.o Disk.o DiskScheduler.o Elevator.o Message.o Sleep.o Utils.o main.c -o $(OUT) $(ARGS)

Benchmark.o: Benchmark.c
	$(COMPILER) -c Benchmark.c $(ARGS)

BufferQueue.o: BufferQueue.c
	$(COMPILER) -c BufferQueue.c $(ARGS)

BufferQueueThread.o: BufferQueueThread.c
	$(COMPILER) -c BufferQueueThread.c $(ARGS)

Client.o: Client.c
	$(COMPILER) -c Client.c $(ARGS)

Disk.o: Disk.c
	$(COMPILER) -c Disk.c $(ARGS)

DiskScheduler.o: DiskScheduler.c
	$(COMPILER) -c DiskScheduler.c $(ARGS)

Elevator.o: Elevator.c
	$(COMPILER) -c Elevator.c $(ARGS)

Message.o: Message.c
	$(COMPILER) -c Message.c $(ARGS)

Sleep.o: Sleep.c
	$(COMPILER) -c Sleep.c $(ARGS)

Utils.o: Utils.c
	$(COMPILER) -c Utils.c $(ARGS)
clean: 
	rm *.o