ARGS=-std=c11 -pthread -lm
OARGS=-t
OUTPUT=Program.out
ITERATIONS=42
all: main run
main: BufferQueue.o BufferQueueThread.o Disk.o DiskScheduler.o DiskTest.c
	gcc BufferQueue.o BufferQueueThread.o Disk.o DiskScheduler.o DiskTest.c -o $(OUTPUT) $(ARGS)

DiskScheduler.o: Disk.o BufferQueue.o BufferQueueThread.o DiskScheduler.c Sleep.o
	gcc -c Disk.o Sleep.o BufferQueue.o BufferQueueThread.o DiskScheduler.c $(ARGS)

Disk.o: Sleep.o Disk.c
	gcc -c Sleep.o Disk.c $(ARGS)

BufferQueueThread.o: BufferQueueThread.c BufferQueue.o 
	gcc -c BufferQueue.o BufferQueueThread.c $(ARGS)

BufferQueue.o: BufferQueue.c
	gcc -c BufferQueue.c $(ARGS)

Sleep.o: Sleep.c
	gcc -c Sleep.c $(ARGS)

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