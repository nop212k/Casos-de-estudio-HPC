#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

#define CACHELINE 64

static int     N, NTHREADS, MAXITER;
static double  TOL;
static double *u_old, *u_new, *rhs;
static double  error_global;
static int     iter_global;
static double *local_errors;        /* un slot por hilo */
static pthread_barrier_t barrier;

double f_src(double x) { return 1.0; }

void *jacobi_worker(void *arg) {
    int tid = *(int *)arg;

    int chunk = (N + NTHREADS - 1) / NTHREADS;
    int start = tid * chunk + 1;
    int end   = start + chunk - 1;
    if (end > N) end = N;

    while (1) {
        /* --- verificar condición de parada (solo lectura) --- */
        if (error_global <= TOL || iter_global >= MAXITER) break;

        /* --- fase 1: cada hilo actualiza su bloque --- */
        double le = 0.0;
        for (int i = start; i <= end; i++) {
            u_new[i] = (u_old[i-1] + u_old[i+1] + rhs[i]) * 0.5;
            double d = fabs(u_new[i] - u_old[i]);
            if (d > le) le = d;
        }
        local_errors[tid] = le;

        /* --- barrera: todos terminaron u_new --- */
        pthread_barrier_wait(&barrier);

        /* --- hilo 0: reducción + intercambio + conteo --- */
        if (tid == 0) {
            double max_e = 0.0;
            for (int t = 0; t < NTHREADS; t++)
                if (local_errors[t] > max_e) max_e = local_errors[t];
            error_global = max_e;

            double *tmp = u_old; u_old = u_new; u_new = tmp;
            iter_global++;
        }

        /* --- barrera: esperar intercambio de punteros --- */
        pthread_barrier_wait(&barrier);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    N        = (argc > 1) ? atoi(argv[1]) : 500;
    NTHREADS = (argc > 2) ? atoi(argv[2]) : 4;
    TOL      = 1e-6;
    MAXITER  = 100000;

    if (N <= 0 || NTHREADS <= 0) {
        fprintf(stderr, "Uso: %s <N> <hilos>\n", argv[0]); return 1;
    }

    double h  = 1.0 / (N + 1);
    double h2 = h * h;
    size_t sz = (size_t)(N + 2) * sizeof(double);

    u_old = aligned_alloc(CACHELINE, sz);
    u_new = aligned_alloc(CACHELINE, sz);
    rhs   = aligned_alloc(CACHELINE, sz);
    local_errors = calloc(NTHREADS, sizeof(double));
    if (!u_old || !u_new || !rhs || !local_errors) {
        fprintf(stderr, "Error: memoria\n"); return 1;
    }

    for (int i = 0; i < N + 2; i++) { u_old[i] = 0.0; u_new[i] = 0.0; }
    for (int i = 1; i <= N; i++) rhs[i] = h2 * f_src(i * h);

    error_global = TOL + 1.0;
    iter_global  = 0;

    pthread_barrier_init(&barrier, NULL, NTHREADS);

    int *tids_id = malloc(NTHREADS * sizeof(int));
    pthread_t *tids = malloc(NTHREADS * sizeof(pthread_t));
    for (int t = 0; t < NTHREADS; t++) tids_id[t] = t;

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int t = 0; t < NTHREADS; t++)
        pthread_create(&tids[t], NULL, jacobi_worker, &tids_id[t]);
    for (int t = 0; t < NTHREADS; t++)
        pthread_join(tids[t], NULL);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    printf("hilos,%d,%d,%.6f,%d,%.2e\n", N, NTHREADS, elapsed, iter_global, error_global);

    pthread_barrier_destroy(&barrier);
    free(tids); free(tids_id); free(local_errors);
    free(u_old); free(u_new); free(rhs);
    return 0;
}
