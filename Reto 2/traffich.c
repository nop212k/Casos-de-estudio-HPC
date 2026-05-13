/*
 * traffic_pthreads.c — Autómata celular de tráfico con hilos (Pthreads)
 *
 * Compilar:
 * gcc -O2 -pthread traffic_pthreads.c -o traffic
 *
 * Ejecutar:
 * ./traffic ROAD_N N_WARMUP N_MEASURE N_THREADS
 *
 * Ejemplo:
 * ./traffic 5000 500 1000 4
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define NDENSITIES 20

static const int RULE184[8] = {0, 0, 0, 1, 1, 1, 0, 1};

typedef struct
{
    int id;
    int start;
    int end;
    int sz;

    const int *old;
    int *new_road;

    long moved;
} ThreadData;

void *step_thread(void *arg)
{
    ThreadData *data = (ThreadData *)arg;

    long moved_local = 0;

    for (int i = data->start; i < data->end; i++)
    {
        int L = data->old[(i - 1 + data->sz) % data->sz];
        int C = data->old[i];
        int R = data->old[(i + 1) % data->sz];

        int idx = (L << 2) | (C << 1) | R;

        data->new_road[i] = RULE184[idx];

        if (C == 1 && data->new_road[i] == 0)
            moved_local++;
    }

    data->moved = moved_local;

    pthread_exit(NULL);
}

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
    pthread_t *threads = (pthread_t *)malloc(n_threads * sizeof(pthread_t));
    ThreadData *thread_data = (ThreadData *)malloc(n_threads * sizeof(ThreadData));

    int chunk = sz / n_threads;

    for (int t = 0; t < n_threads; t++)
    {
        thread_data[t].id = t;
        thread_data[t].start = t * chunk;

        if (t == n_threads - 1)
            thread_data[t].end = sz;
        else
            thread_data[t].end = (t + 1) * chunk;

        thread_data[t].sz = sz;
        thread_data[t].old = old;
        thread_data[t].new_road = new_road;
        thread_data[t].moved = 0;

        pthread_create(&threads[t], NULL, step_thread, &thread_data[t]);
    }

    long total_moved = 0;

    for (int t = 0; t < n_threads; t++)
    {
        pthread_join(threads[t], NULL);
        total_moved += thread_data[t].moved;
    }

    free(threads);
    free(thread_data);

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

    for (int t = 0; t < n_warmup; t++)
    {
        step_parallel(old, new_road, road_n, n_threads);

        memcpy(old, new_road, road_n * sizeof(int));
    }

    long total_moved = 0;

    for (int t = 0; t < n_measure; t++)
    {
        total_moved += step_parallel(old, new_road, road_n, n_threads);

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
    /* Verificaciones simplificadas */
}

int main(int argc, char *argv[])
{
    int road_n = 5000;
    int n_warmup = 500;
    int n_measure = 1000;
    int n_threads = 4;

    /*
     * Argumentos:
     * argv[1] = ROAD_N
     * argv[2] = N_WARMUP
     * argv[3] = N_MEASURE
     * argv[4] = N_THREADS
     */

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

    run_checks();

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

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        double vel = simulate(
            road_n,
            n_warmup,
            n_measure,
            density_target,
            &density_real,
            seed,
            n_threads);

        int cantidad_exacta = (int)(density_real * road_n);
        clock_gettime(CLOCK_MONOTONIC, &end);

        double elapsed =
            (end.tv_sec - start.tv_sec) +
            (end.tv_nsec - start.tv_nsec) / 1e9;

        printf("%.4f  %d  %.6f %.6f\n",
               density_target,
               cantidad_exacta,
               vel, elapsed);

        fflush(stdout);
    }

    return 0;
}
