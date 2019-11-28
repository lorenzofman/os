COMPILER=gcc
ARGS=-std=c11 -pthread -lm -g
OARGS=-t
OUTPUT=Program.out
ITERATIONS=42
all: main run
main: BufferQueue.o BufferQueueThread.o Disk.o DiskScheduler.o DiskTest.c Sleep.o Message.o Client.o Constants.h
	$(COMPILER) BufferQueue.o BufferQueueThread.o Disk.o DiskScheduler.o Sleep.o Message.o Client.o Constants.h DiskTest.c -o $(OUTPUT) $(ARGS)

DiskScheduler.o: Disk.o BufferQueue.o BufferQueueThread.o DiskScheduler.c
	$(COMPILER) -c Disk.o BufferQueue.o BufferQueueThread.o DiskScheduler.c $(ARGS)

Disk.o: Disk.c
	$(COMPILER) -c Disk.c $(ARGS)

BufferQueueThread.o: BufferQueueThread.c BufferQueue.o Sleep.o
	$(COMPILER) -c Sleep.o BufferQueue.o BufferQueueThread.c $(ARGS)

BufferQueue.o: BufferQueue.c
	$(COMPILER) -c BufferQueue.c $(ARGS)

Client.o: Client.c
	$(COMPILER) -c Client.c $(ARGS)
Message.o: Message.c
	$(COMPILER) -c Message.c $(ARGS)

Sleep.o: Sleep.c
	$(COMPILER) -c Sleep.c $(ARGS)

clean: 
	rm *.o
run: 
	./$(OUTPUT)
threads: main
	./$(OUTPUT) ${OARGS}
benchmark: main 
	bash -c "for i in {1..${ITERATIONS}}; do ./$(OUTPUT); done"
benchmarkThreads: main 
	bash -c "for i in {1..${ITERATIONS}}; do ./$(OUTPUT) ${OARGS}; done"