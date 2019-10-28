ARGS= -O2 -std=c11 -pthread
OUTPUT=Program
ITERATIONS=42
all: main run
main: BufferQueue.o BufferQueueThread.o
	gcc BufferQueue.o BufferQueueThread.o TrivialBenchmark.c -o $(OUTPUT) -pthread
BufferQueueThread.o: BufferQueueThread.c BufferQueue.o
	gcc -c BufferQueue.o BufferQueueThread.c $(ARGS)
BufferQueue.o: BufferQueue.c
	gcc -c BufferQueue.c $(ARGS)
clean: rm *.o
run: 
	./$(OUTPUT)
benchmark: all
	bash -c "for i in {1..${ITERATIONS}}; do ./$(OUTPUT); done"