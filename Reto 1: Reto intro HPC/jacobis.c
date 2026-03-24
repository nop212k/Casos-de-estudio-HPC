#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

double f(double x) { return 1.0; }

int main(int argc, char *argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 500;
    if (N <= 0) { fprintf(stderr, "N debe ser > 0\n"); return 1; }

    double TOL    = 1e-6;
    int    MAXITER = 100000;
    double h  = 1.0 / (N + 1);
    double h2 = h * h;

    double *u_old = calloc(N + 2, sizeof(double));
    double *u_new = calloc(N + 2, sizeof(double));
    double *rhs   = malloc((N + 2) * sizeof(double));
    if (!u_old || !u_new || !rhs) { fprintf(stderr, "Error: memoria\n"); return 1; }

    for (int i = 1; i <= N; i++) rhs[i] = h2 * f(i * h);

    u_old[0] = 0.0; u_old[N+1] = 0.0;
    u_new[0] = 0.0; u_new[N+1] = 0.0;

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    int    iter  = 0;
    double error = TOL + 1.0;

    while (error > TOL && iter < MAXITER) {
        for (int i = 1; i <= N; i++)
            u_new[i] = (u_old[i-1] + u_old[i+1] + rhs[i]) * 0.5;

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

    printf("serial,%d,1,%.6f,%d,%.2e\n", N, elapsed, iter, error);

    free(u_old); free(u_new); free(rhs);
    return 0;
}
