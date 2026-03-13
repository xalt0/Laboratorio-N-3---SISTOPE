# José Antonio Muñoz Álvarez - 21.154.079-6

# Simulador Concurrente de Memoria Virtual

Simulador de segmentación y paginación con múltiples threads POSIX (pthreads).
Implementa traducción de direcciones virtuales a físicas, TLB con política FIFO,
page faults con eviction FIFO, y modos SAFE/UNSAFE para observar concurrencia.

Laboratorio 3 - Sistemas Operativos 2/2025.

## Requisitos

- Compilador C con soporte C11 (`gcc`)
- Biblioteca `pthreads` (disponible por defecto en Linux)

## Compilación

```bash
make
```

## Ejecución

```bash
make run
```

Ejecuta un ejemplo por defecto en modo paginación con 2 threads.

## Reproducción de experimentos

```bash
make reproduce
```

Ejecuta los 3 experimentos y guarda los resultados en `out/`.

## Ejemplos de comandos

```bash
# Segmentación: 4 threads, workload uniform, mostrar stats
./simulator --mode seg --threads 4 --workload uniform --ops-per-thread 5000 --stats

# Paginación: 8 threads, TLB FIFO, eviction FIFO, modo UNSAFE
./simulator --mode page --threads 8 --frames 16 --tlb-size 32 \
            --tlb-policy fifo --evict-policy fifo --unsafe --stats

# Paginación: experimento de thrashing
./simulator --mode page --threads 8 --frames 4 --workload uniform \
            --ops-per-thread 10000 --seed 123 --stats

# Segmentación con límites personalizados
./simulator --mode seg --threads 1 --segments 4 \
            --seg-limits 1024,2048,4096,8192 --seed 100 --stats
```
