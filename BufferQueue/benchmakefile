ARGS=-std=c11 -pthread
OARGS=-t
OUTPUT=Program.out
ITERATIONS=42
all: main run
main: BufferQueue.o BufferQueueThread.o
	gcc BufferQueue.o BufferQueueThread.o Benchmark.c -o $(OUTPUT) -pthread
BufferQueueThread.o: BufferQueueThread.c BufferQueue.o
	gcc -c BufferQueue.o BufferQueueThread.c $(ARGS)
BufferQueue.o: BufferQueue.c
	gcc -c BufferQueue.c $(ARGS)
clean: rm *.o
run: 
	./$(OUTPUT)
threads: main
	./$(OUTPUT) ${OARGS}
benchmark: main 
	bash -c "for i in {1..${ITERATIONS}}; do ./$(OUTPUT); done"
benchmarkThreads: main 
	bash -c "for i in {1..${ITERATIONS}}; do ./$(OUTPUT) ${OARGS}; done"