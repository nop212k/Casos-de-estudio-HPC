/*
 * traffic_omp.c — Autómata celular de tráfico con OpenMP
 *
 * Compilar:
 * gcc -O2 -fopenmp traffic_omp.c -o traffic_omp
 *
 * Ejecutar:
 * ./traffic_omp ROAD_N N_WARMUP N_MEASURE N_THREADS
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

#define NDENSITIES 20

static const int RULE184[8] = {0, 0, 0, 1, 1, 1, 0, 1};

int init_road(int *road, int sz, double density, unsigned int *seed)
{
    int cars = 0;

    for (int i = 0; i < sz; i++)
    {
        double r = (double)rand_r(seed) / ((double)RAND_MAX + 1.0);

        road[i] = (r < density) ? 1 : 0;

        cars += road[i];
    }

    return cars;
}

long step_parallel(const int *old, int *new_road, int sz, int n_threads)
{
    long total_moved = 0;

#pragma omp parallel for num_threads(n_threads) reduction(+ : total_moved)
    for (int i = 0; i < sz; i++)
    {
        int L = old[(i - 1 + sz) % sz];
        int C = old[i];
        int R = old[(i + 1) % sz];

        int idx = (L << 2) | (C << 1) | R;

        new_road[i] = RULE184[idx];

        if (C == 1 && new_road[i] == 0)
            total_moved++;
    }

    return total_moved;
}

double simulate(
    int road_n,
    int n_warmup,
    int n_measure,
    double density,
    double *real_density,
    unsigned int seed,
    int n_threads)
{
    int *old = (int *)malloc(road_n * sizeof(int));
    int *new_road = (int *)malloc(road_n * sizeof(int));

    if (!old || !new_road)
    {
        fprintf(stderr, "Error: sin memoria suficiente\n");
        exit(EXIT_FAILURE);
    }

    int total_cars = init_road(old, road_n, density, &seed);

    *real_density = (double)total_cars / road_n;

    // Warmup
    for (int t = 0; t < n_warmup; t++)
    {
        step_parallel(old, new_road, road_n, n_threads);

        memcpy(old, new_road, road_n * sizeof(int));
    }

    // Medición
    long total_moved = 0;

    for (int t = 0; t < n_measure; t++)
    {
        total_moved += step_parallel(
            old,
            new_road,
            road_n,
            n_threads);

        memcpy(old, new_road, road_n * sizeof(int));
    }

    free(old);
    free(new_road);

    if (total_cars == 0)
        return 0.0;

    return (double)total_moved /
           ((double)total_cars * n_measure);
}

int main(int argc, char *argv[])
{
    int road_n = 5000;
    int n_warmup = 500;
    int n_measure = 1000;
    int n_threads = 4;

    if (argc >= 5)
    {
        road_n = atoi(argv[1]);
        n_warmup = atoi(argv[2]);
        n_measure = atoi(argv[3]);
        n_threads = atoi(argv[4]);
    }

    if (n_threads <= 0)
    {
        fprintf(stderr, "Error: cantidad de hilos inválida\n");
        return EXIT_FAILURE;
    }

    srand((unsigned int)time(NULL));

    printf("# ROAD_N=%d  N_WARMUP=%d  N_MEASURE=%d  THREADS=%d\n",
           road_n,
           n_warmup,
           n_measure,
           n_threads);

    printf("# densidad_objetivo  cantidad_autos  velocidad_asint tiempo\n");

    for (int d = 0; d <= NDENSITIES; d++)
    {
        double density_target = (double)d / NDENSITIES;

        double density_real;

        unsigned int seed = (unsigned int)rand();

        double start = omp_get_wtime();

        double vel = simulate(
            road_n,
            n_warmup,
            n_measure,
            density_target,
            &density_real,
            seed,
            n_threads);

        double end = omp_get_wtime();

        int cantidad_exacta = (int)(density_real * road_n);

        printf("%.4f  %d  %.6f %.6f\n",
               density_target,
               cantidad_exacta,
               vel, end - start);

        fflush(stdout);
    }

    return 0;
}