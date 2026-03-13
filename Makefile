CC     = gcc
CFLAGS = -std=c11 -pthread -Wall -Wextra -Iinclude -D_POSIX_C_SOURCE=200809L
LDFLAGS= -pthread

SRCS = src/simulator.c src/segmentacion.c src/paginacion.c \
       src/tlb.c src/frame_allocator.c src/workloads.c
OBJS = $(SRCS:.c=.o)

LIB_OBJS = src/segmentacion.o src/paginacion.o src/tlb.o \
           src/frame_allocator.o src/workloads.o

.PHONY: all run reproduce tests clean

all: simulator

simulator: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: simulator
	@mkdir -p out
	./simulator --mode page --threads 2 --ops-per-thread 500 \
	            --pages 32 --frames 16 --tlb-size 16 --seed 42 --stats

reproduce: simulator
	@mkdir -p out
	@echo "=== Experimento 1: Segmentación con Segfaults ==="
	./simulator --mode seg --threads 1 --workload uniform \
	            --ops-per-thread 10000 --segments 4 \
	            --seg-limits 1024,2048,4096,8192 --seed 100 --stats
	@cp out/summary.json out/exp1.json

	@echo "=== Experimento 2a: Sin TLB ==="
	./simulator --mode page --threads 1 --workload 80-20 \
	            --ops-per-thread 50000 --pages 128 --frames 64 \
	            --page-size 4096 --tlb-size 0 --seed 200 --stats
	@cp out/summary.json out/exp2a.json

	@echo "=== Experimento 2b: Con TLB ==="
	./simulator --mode page --threads 1 --workload 80-20 \
	            --ops-per-thread 50000 --pages 128 --frames 64 \
	            --page-size 4096 --tlb-size 32 --seed 200 --stats
	@cp out/summary.json out/exp2b.json

	@echo "=== Experimento 3a: Thrashing 1 thread ==="
	./simulator --mode page --threads 1 --workload uniform \
	            --ops-per-thread 10000 --pages 64 --frames 8 \
	            --page-size 4096 --tlb-size 16 --seed 300 --stats
	@cp out/summary.json out/exp3a.json

	@echo "=== Experimento 3b: Thrashing 8 threads ==="
	./simulator --mode page --threads 8 --workload uniform \
	            --ops-per-thread 10000 --pages 64 --frames 8 \
	            --page-size 4096 --tlb-size 16 --seed 300 --stats
	@cp out/summary.json out/exp3b.json

tests: $(LIB_OBJS)
	$(CC) $(CFLAGS) -o tests/test_segmentacion  tests/test_segmentacion.c  $(LIB_OBJS) $(LDFLAGS)
	$(CC) $(CFLAGS) -o tests/test_paginacion    tests/test_paginacion.c    $(LIB_OBJS) $(LDFLAGS)
	$(CC) $(CFLAGS) -o tests/test_concurrencia  tests/test_concurrencia.c  $(LIB_OBJS) $(LDFLAGS)
	./tests/test_segmentacion
	./tests/test_paginacion
	./tests/test_concurrencia

clean:
	rm -f $(OBJS) simulator
	rm -f tests/test_segmentacion tests/test_paginacion tests/test_concurrencia
	rm -f out/*.json
