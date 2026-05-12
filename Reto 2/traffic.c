/*
 * traffic.c — Autómata celular de tráfico (versión serial)
 *             Implementa Rule 184 con condiciones de frontera periódicas.
 *
 * Compilar:  gcc -O2 -o traffic traffic.c
 * Ejecutar:  ./traffic
 *
 * Salida en stdout:
 *   densidad_objetivo  densidad_real  velocidad_asintótica
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ——— Parámetros de simulación ——— */
#define ROAD_N      5000   /* tamaño de la carretera (número de celdas)    */
#define N_WARMUP     500   /* pasos de calentamiento (estado transitorio)  */
#define N_MEASURE   1000   /* pasos de medición para promediar velocidad   */
#define NDENSITIES    20   /* número de densidades que se van a explorar   */

/*
 * Tabla de la Rule 184 (autómata elemental número 184).
 * El índice es el patrón de 3 bits: (izquierda<<2)|(centro<<1)|derecha
 *
 *  idx  L C R   nuevo valor
 *   7   1 1 1 → 1   (auto bloqueado, permanece)
 *   6   1 1 0 → 0   (auto avanza, celda queda libre)
 *   5   1 0 1 → 1   (celda vacía entre dos autos: el de la izquierda está bloqueado)
 *   4   1 0 0 → 1   (celda vacía recibe auto desde la izquierda)
 *   3   0 1 1 → 1   (auto bloqueado, permanece)
 *   2   0 1 0 → 0   (auto avanza, celda queda libre)
 *   1   0 0 1 → 0   (celda vacía, no hay auto a la izquierda)
 *   0   0 0 0 → 0   (celda vacía, nadie llega)
 */
static const int RULE184[8] = {0, 0, 0, 1, 1, 1, 0, 1};

/* ——— Inicialización de la carretera ——— */
/*
 * Asigna aleatoriamente 1s y 0s con la probabilidad `density`.
 * Devuelve el número real de autos colocados.
 */
int init_road(int *road, int sz, double density, unsigned int *seed)
{
    int cars = 0;
    for (int i = 0; i < sz; i++) {
        /* rand_r es re-entrante (útil más adelante en la versión paralela) */
        double r = (double)rand_r(seed) / ((double)RAND_MAX + 1.0);
        road[i]  = (r < density) ? 1 : 0;
        cars    += road[i];
    }
    return cars;
}

/* ——— Un paso de tiempo ——— */
/*
 * Aplica la Rule 184 sobre old[0..sz-1] (condiciones periódicas)
 * y guarda el resultado en new_road[0..sz-1].
 * Devuelve el número de autos que avanzaron en este paso.
 */
int step(const int *old, int *new_road, int sz)
{
    int moved = 0;
    for (int i = 0; i < sz; i++) {
        int L   = old[(i - 1 + sz) % sz];
        int C   = old[i];
        int R   = old[(i + 1)      % sz];
        int idx = (L << 2) | (C << 1) | R;

        new_road[i] = RULE184[idx];

        /* Un auto se "mueve" cuando su celda pasa de 1 a 0 */
        if (C == 1 && new_road[i] == 0)
            moved++;
    }
    return moved;
}

/* ——— Simulación completa para una densidad ——— */
/*
 * Corre N_WARMUP pasos de calentamiento (descartados) y luego
 * N_MEASURE pasos de medición.
 * Devuelve la velocidad promedio asintótica: fracción de autos
 * que se mueven por paso, valor en [0, 1].
 */
double simulate(double density, double *real_density, unsigned int seed)
{
    int *old      = (int *)malloc(ROAD_N * sizeof(int));
    int *new_road = (int *)malloc(ROAD_N * sizeof(int));
    if (!old || !new_road) {
        fprintf(stderr, "Error: sin memoria suficiente\n");
        exit(EXIT_FAILURE);
    }

    int total_cars = init_road(old, ROAD_N, density, &seed);
    *real_density  = (double)total_cars / ROAD_N;

    /* Fase de calentamiento — la distribución aleatoria inicial tarda
       varios centenares de pasos en alcanzar el régimen estacionario */
    for (int t = 0; t < N_WARMUP; t++) {
        step(old, new_road, ROAD_N);
        memcpy(old, new_road, ROAD_N * sizeof(int));
    }

    /* Fase de medición */
    long total_moved = 0;
    for (int t = 0; t < N_MEASURE; t++) {
        total_moved += step(old, new_road, ROAD_N);
        memcpy(old, new_road, ROAD_N * sizeof(int));
    }

    free(old);
    free(new_road);

    if (total_cars == 0) return 0.0;
    return (double)total_moved / ((double)total_cars * N_MEASURE);
}

/* ——— Verificaciones básicas ——— */
/*
 * Casos de prueba simples donde el resultado esperado es exacto.
 * Útil para validar que la implementación es correcta antes de
 * correr el experimento completo.
 */
void run_checks(void)
{
    printf("# ——— Verificaciones ———\n");

    /* 1. Carretera llena (densidad 1): ningún auto puede avanzar → vel = 0 */
    {
        int old[10]  = {1,1,1,1,1,1,1,1,1,1};
        int nw[10];
        int moved = step(old, nw, 10);
        int after = 0; for (int i = 0; i < 10; i++) after += nw[i];
        printf("# Carretera llena  → moved=%d (esperado 0), autos=%d (esperado 10)  %s\n",
               moved, after, (moved == 0 && after == 10) ? "OK" : "FALLO");
    }

    /* 2. Auto solitario: siempre avanza → vel = 1 */
    {
        int old[10]  = {0,0,0,0,1,0,0,0,0,0};
        int nw[10];
        int moved = step(old, nw, 10);
        int after = 0; for (int i = 0; i < 10; i++) after += nw[i];
        printf("# Auto solitario   → moved=%d (esperado 1), autos=%d (esperado 1)   %s\n",
               moved, after, (moved == 1 && after == 1) ? "OK" : "FALLO");
    }

    /* 3. Alternado 1,0,1,0,...: cada auto avanza, todos se conservan */
    {
        int old[8] = {1,0,1,0,1,0,1,0};
        int nw[8];
        int before = 4;
        step(old, nw, 8);
        int after = 0; for (int i = 0; i < 8; i++) after += nw[i];
        printf("# Alternado 1,0    → autos antes=%d, después=%d  %s\n",
               before, after, before == after ? "OK" : "FALLO");
    }

    /* 4. Conservación general con densidad 0.5 */
    {
        unsigned int seed = 99999;
        int old[500], nw[500];
        int before = init_road(old, 500, 0.5, &seed);
        step(old, nw, 500);
        int after = 0; for (int i = 0; i < 500; i++) after += nw[i];
        printf("# Conservación N=500 → antes=%d, después=%d  %s\n\n",
               before, after, before == after ? "OK" : "FALLO");
    }
}

/* ——— Programa principal ——— */
int main(void)
{
    srand((unsigned int)time(NULL));

    run_checks();

    printf("# ——— Experimento: velocidad vs densidad ———\n");
    printf("# ROAD_N=%d  N_WARMUP=%d  N_MEASURE=%d\n", ROAD_N, N_WARMUP, N_MEASURE);
    printf("# densidad_objetivo  densidad_real  velocidad_asint\n\n");

    for (int d = 0; d <= NDENSITIES; d++) {
        double density_target = (double)d / NDENSITIES;
        double density_real;
        unsigned int seed = (unsigned int)rand();

        double vel = simulate(density_target, &density_real, seed);
        printf("%.4f  %.4f  %.6f\n", density_target, density_real, vel);
        fflush(stdout);
    }

    return 0;
}
