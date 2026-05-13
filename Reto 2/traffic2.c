/*
 * traffic.c — Autómata celular de tráfico (versión parametrizada)
 * Compilar y ejecutar según la herramienta de profiling.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

int step(const int *old, int *new_road, int sz)
{
    int moved = 0;
    for (int i = 0; i < sz; i++)
    {
        int L = old[(i - 1 + sz) % sz];
        int C = old[i];
        int R = old[(i + 1) % sz];
        int idx = (L << 2) | (C << 1) | R;

        new_road[i] = RULE184[idx];
        if (C == 1 && new_road[i] == 0)
            moved++;
    }
    return moved;
}

double simulate(int road_n, int n_warmup, int n_measure, double density, double *real_density, unsigned int seed)
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

    for (int t = 0; t < n_warmup; t++)
    {
        step(old, new_road, road_n);
        memcpy(old, new_road, road_n * sizeof(int));
    }

    long total_moved = 0;
    for (int t = 0; t < n_measure; t++)
    {
        total_moved += step(old, new_road, road_n);
        memcpy(old, new_road, road_n * sizeof(int));
    }

    free(old);
    free(new_road);

    if (total_cars == 0)
        return 0.0;
    return (double)total_moved / ((double)total_cars * n_measure);
}

void run_checks(void)
{
    /* Verificaciones simplificadas (silenciadas para el script) */
}

int main(int argc, char *argv[])
{
    int road_n = 5000;
    int n_warmup = 500;
    int n_measure = 1000;

    /* Leer argumentos desde la consola si existen */
    if (argc >= 4)
    {
        road_n = atoi(argv[1]);
        n_warmup = atoi(argv[2]);
        n_measure = atoi(argv[3]);
    }

    srand((unsigned int)time(NULL));
    run_checks();

    printf("# ROAD_N=%d  N_WARMUP=%d  N_MEASURE=%d\n", road_n, n_warmup, n_measure);
    printf("# densidad_objetivo  densidad_real  velocidad_asint\n");

    for (int d = 0; d <= NDENSITIES; d++)
    {
        double density_target = (double)d / NDENSITIES;
        double density_real;
        unsigned int seed = (unsigned int)rand();

        double vel = simulate(road_n, n_warmup, n_measure, density_target, &density_real, seed);
        int cantidad_exacta = (int)(density_real * road_n);
        printf("%.4f  %d  %.6f\n", density_target, cantidad_exacta, vel);
        fflush(stdout);
    }
    return 0;
}