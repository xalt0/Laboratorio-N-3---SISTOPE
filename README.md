# José Antonio Muñoz Álvarez - 21.154.079-6

# Simulador Concurrente de Memoria Virtual

Simulador de segmentación y paginación con pthreads. Laboratorio 3 - Sistemas Operativos 2/2025.

## Requisitos

- `gcc` con soporte C11
- `pthreads` (disponible en Linux)

## Compilación

```bash
make
```

## Ejecución

```bash
make run
```

O manualmente:

```bash
./simulator --mode <seg|page> [opciones] --stats
```

## Flags disponibles

| Flag | Default | Descripción |
|------|---------|-------------|
| `--mode seg\|page` | — | Modo (obligatorio) |
| `--threads INT` | 1 | Número de threads |
| `--ops-per-thread INT` | 1000 | Operaciones por thread |
| `--workload uniform\|80-20` | uniform | Patrón de acceso |
| `--seed INT` | 42 | Semilla aleatoria |
| `--unsafe` | — | Sin locks |
| `--stats` | — | Mostrar reporte |
| `--segments INT` | 4 | Segmentos (modo seg) |
| `--seg-limits CSV` | 4096,… | Límites por segmento |
| `--pages INT` | 64 | Páginas virtuales (modo page) |
| `--frames INT` | 32 | Frames físicos globales |
| `--page-size INT` | 4096 | Tamaño de página en bytes |
| `--tlb-size INT` | 16 | Entradas TLB (0=desactivada) |
| `--tlb-policy fifo` | fifo | Política TLB |
| `--evict-policy fifo` | fifo | Política eviction |

## Reproducir experimentos

```bash
make reproduce
```

Genera los JSONs en `out/exp1.json`, `out/exp2a.json`, `out/exp2b.json`, `out/exp3a.json`, `out/exp3b.json`.

## Tests

```bash
make tests
```
