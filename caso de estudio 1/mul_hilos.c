#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

// ============================================================
//  ESTRUCTURAS
// ============================================================
typedef struct {
    int** A;
    int** B_T;      // Transpuesta de B (cache-friendly)
    int** C;
    int   n;
    int   id_hilo;
    int   num_hilos;
} HiloArg;

// ============================================================
//  GESTIÓN DE MEMORIA
// ============================================================
int** crearMatriz(int n) {
    int** matriz = (int**)malloc(n * sizeof(int*));
    int*  bloque = (int*)malloc(n * n * sizeof(int));
    for (int i = 0; i < n; i++)
        matriz[i] = bloque + i * n;
    return matriz;
}

void liberarMatriz(int** matriz) {
    free(matriz[0]);
    free(matriz);
}

void llenarMatrizAleatoria(int** matriz, int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            matriz[i][j] = rand() % 100;
}

int** transponerMatriz(int** B, int n) {
    int** B_T = crearMatriz(n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            B_T[j][i] = B[i][j];
    return B_T;
}

// ============================================================
//  FUNCIÓN DEL HILO
// ============================================================
void* multiplicar_chunk(void* arg) {
    HiloArg* ha    = (HiloArg*)arg;
    int**    A     = ha->A;
    int**    B_T   = ha->B_T;
    int**    C     = ha->C;
    int      n     = ha->n;
    int      id    = ha->id_hilo;
    int      nthrd = ha->num_hilos;

    // Cada hilo procesa su rango de filas
    int chunk    = n / nthrd;
    int fila_ini = id * chunk;
    int fila_fin = (id == nthrd - 1) ? n : fila_ini + chunk;

    for (int i = fila_ini; i < fila_fin; i++) {
        for (int j = 0; j < n; j++) {
            long long acc = 0;
            for (int k = 0; k < n; k++) {
                acc += (long long)A[i][k] * B_T[j][k];
            }
            C[i][j] = (int)acc;
        }
    }
    return NULL;
}

// ============================================================
//  MAIN
// ============================================================
int main(int argc, char* argv[]) {

    if (argc < 3) {
        fprintf(stderr, "Uso:     %s <dimension> <num_hilos>\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s 1000 4\n", argv[0]);
        return 1;
    }

    int n         = atoi(argv[1]);
    int num_hilos = atoi(argv[2]);

    if (n <= 0 || num_hilos <= 0) {
        fprintf(stderr, "Error: dimension y num_hilos deben ser positivos.\n");
        return 1;
    }

    srand((unsigned int)time(NULL));

    // Crear y llenar matrices
    int** A   = crearMatriz(n);
    int** B   = crearMatriz(n);
    int** C   = crearMatriz(n);

    llenarMatrizAleatoria(A, n);
    llenarMatrizAleatoria(B, n);
    memset(C[0], 0, n * n * sizeof(int));

    // Transponer B una vez
    int** B_T = transponerMatriz(B, n);

    // Crear hilos
    pthread_t* hilos = (pthread_t*)malloc(num_hilos * sizeof(pthread_t));
    HiloArg*   args  = (HiloArg*)malloc(num_hilos * sizeof(HiloArg));

    // Medir wall clock SOLO de la multiplicación
    struct timespec inicio, fin;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    for (int t = 0; t < num_hilos; t++) {
        args[t].A         = A;
        args[t].B_T       = B_T;
        args[t].C         = C;
        args[t].n         = n;
        args[t].id_hilo   = t;
        args[t].num_hilos = num_hilos;
        pthread_create(&hilos[t], NULL, multiplicar_chunk, &args[t]);
    }
    for (int t = 0; t < num_hilos; t++)
        pthread_join(hilos[t], NULL);

    clock_gettime(CLOCK_MONOTONIC, &fin);

    double wall_clock = (fin.tv_sec  - inicio.tv_sec) +
                        (fin.tv_nsec - inicio.tv_nsec) / 1e9;

    printf("n=%-6d  hilos=%-2d  wall_clock=%.6f s\n", n, num_hilos, wall_clock);

    // Liberar memoria
    free(hilos);
    free(args);
    liberarMatriz(A);
    liberarMatriz(B);
    liberarMatriz(B_T);
    liberarMatriz(C);

    return 0;
}
