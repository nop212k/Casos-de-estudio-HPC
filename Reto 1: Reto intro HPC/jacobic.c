#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define CACHELINE 64
#define DOUBLES_PER_LINE (CACHELINE / sizeof(double))   /* 8 */

double f(double x) { return 1.0; }

int main(int argc, char *argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 500;
    if (N <= 0) { fprintf(stderr, "N debe ser > 0\n"); return 1; }

    double TOL     = 1e-6;
    int    MAXITER = 100000;
    double h  = 1.0 / (N + 1);
    double h2 = h * h;

    /* alinear arreglos a 64 bytes para que cada bloque empiece en una
       línea de caché limpia y evitar false sharing en accesos paralelos */
    size_t sz = (N + 2) * sizeof(double);
    double *u_old = aligned_alloc(CACHELINE, sz);
    double *u_new = aligned_alloc(CACHELINE, sz);
    double *rhs   = aligned_alloc(CACHELINE, sz);
    if (!u_old || !u_new || !rhs) { fprintf(stderr, "Error: memoria\n"); return 1; }

    for (int i = 0; i < N + 2; i++) { u_old[i] = 0.0; u_new[i] = 0.0; }
    for (int i = 1; i <= N; i++) rhs[i] = h2 * f(i * h);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    int    iter  = 0;
    double error = TOL + 1.0;

    while (error > TOL && iter < MAXITER) {

        /* procesar en bloques de DOUBLES_PER_LINE (8) para aprovechar
           líneas de caché completas y facilitar vectorización automática */
        int block = DOUBLES_PER_LINE;
        for (int i = 1; i <= N; i += block) {
            int end = i + block - 1;
            if (end > N) end = N;

            /* prefetch del siguiente bloque mientras procesamos el actual */
            __builtin_prefetch(&u_old[end + 1], 0, 1);
            __builtin_prefetch(&u_new[end + 1], 1, 1);

            for (int j = i; j <= end; j++)
                u_new[j] = (u_old[j-1] + u_old[j+1] + rhs[j]) * 0.5;
        }

        error = 0.0;
        for (int i = 1; i <= N; i++) {
            double d = fabs(u_new[i] - u_old[i]);
            if (d > error) error = d;
        }

        double *tmp = u_old; u_old = u_new; u_new = tmp;
        iter++;
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    printf("cacheline,%d,1,%.6f,%d,%.2e\n", N, elapsed, iter, error);

    free(u_old); free(u_new); free(rhs);
    return 0;
}
