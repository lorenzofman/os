ARGS=-std=c11 -pthread -lm
OARGS=-t
OUTPUT=Program.out
ITERATIONS=42
all: main run
main: BufferQueue.o BufferQueueThread.o Disk.o DiskScheduler.o DiskTest.c Sleep.o Constants.h
	gcc BufferQueue.o BufferQueueThread.o Disk.o DiskScheduler.o Sleep.o Constants.h DiskTest.c -o $(OUTPUT) $(ARGS)

DiskScheduler.o: Disk.o BufferQueue.o BufferQueueThread.o DiskScheduler.c Sleep.o
	gcc -c Disk.o Sleep.o BufferQueue.o BufferQueueThread.o DiskScheduler.c $(ARGS) -o DiskScheduler.o

Disk.o: Sleep.o Disk.c
	gcc -c Sleep.o Disk.c $(ARGS) -o Disk.o

BufferQueueThread.o: BufferQueueThread.c BufferQueue.o Sleep.o
	gcc -c Sleep.o BufferQueue.o BufferQueueThread.c $(ARGS) -o BufferQueueThread.o

BufferQueue.o: BufferQueue.c
	gcc -c BufferQueue.c $(ARGS) -o BufferQueue.o

Sleep.o: Sleep.c
	gcc -c Sleep.c $(ARGS) -o Sleep.o

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