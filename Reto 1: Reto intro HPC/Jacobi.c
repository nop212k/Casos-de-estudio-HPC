#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define N       500
#define TOL     1e-6
#define MAXITER 100000

double f(double x) {
    return 1.0;
}

int main() {
    double h  = 1.0 / (N + 1);
    double h2 = h * h;

    double *u_old = calloc(N + 2, sizeof(double));
    double *u_new = calloc(N + 2, sizeof(double));
    double *rhs   = malloc((N + 2) * sizeof(double));

    if (!u_old || !u_new || !rhs) {
        fprintf(stderr, "Error: memoria insuficiente\n");
        return 1;
    }

    // precalcular terminos fuente
    for (int i = 1; i <= N; i++) {
        double x = i * h;
        rhs[i] = h2 * f(x);
    }

    // condiciones de frontera
    u_old[0]   = 0.0;  u_old[N+1] = 0.0;
    u_new[0]   = 0.0;  u_new[N+1] = 0.0;

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    int    iter  = 0;
    double error = TOL + 1.0;

    while (error > TOL && iter < MAXITER) {

        // actualizar todos los puntos interiores
        for (int i = 1; i <= N; i++) {
            u_new[i] = (u_old[i-1] + u_old[i+1] + rhs[i]) * 0.5;
        }

        // calcular error (norma infinito)
        error = 0.0;
        for (int i = 1; i <= N; i++) {
            double diff = fabs(u_new[i] - u_old[i]);
            if (diff > error) error = diff;
        }

        // intercambiar punteros
        double *tmp = u_old;
        u_old = u_new;
        u_new = tmp;

        iter++;
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed = (t_end.tv_sec - t_start.tv_sec)
                   + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    // resultados
    printf("=== Jacobi 1D - Ecuacion de Poisson ===\n");
    printf("Puntos interiores : %d\n", N);
    printf("Paso h            : %.6f\n", h);
    printf("Tolerancia        : %.0e\n", TOL);
    printf("Iteraciones       : %d\n", iter);
    printf("Error final       : %.2e\n", error);
    printf("Tiempo            : %.4f segundos\n", elapsed);

    // solucion analitica: u(x) = x(1-x)/2
    printf("\n--- Comparacion con solucion analitica u(x) = x(1-x)/2 ---\n");
    printf("  i     x        numerica     analitica    error\n");
    for (int i = 1; i <= N; i += N/10) {
        double x        = i * h;
        double analitica = x * (1.0 - x) / 2.0;
        printf("  %-5d %.4f   %.6f     %.6f     %.2e\n",
               i, x, u_old[i], analitica, fabs(u_old[i] - analitica));
    }

    free(u_old);
    free(u_new);
    free(rhs);
    return 0;
}