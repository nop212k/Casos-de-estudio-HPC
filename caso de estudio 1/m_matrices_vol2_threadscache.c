#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

// ============================================================
//  CONFIGURACIÓN
// ============================================================
#define NUM_INTENTOS    10
#define NUM_DIMS        5
#define BLOCK_SIZE      64    // Tamaño del bloque para cache blocking (bytes cache line = 64)
#define CACHE_LINE_SIZE 64    // Bytes por cache line

static const int DIMS[NUM_DIMS] = {500, 1000, 1500, 2000, 3000};

// ============================================================
//  ESTRUCTURAS
// ============================================================

// Pad para evitar false sharing: cada resultado de hilo
// ocupa su propia cache line (64 bytes)
typedef struct {
    double tiempo;
    char   _pad[CACHE_LINE_SIZE - sizeof(double)];
} ResultadoHilo __attribute__((aligned(CACHE_LINE_SIZE)));

typedef struct {
    int**  A;
    int**  B_T;   // Transpuesta de B (cache-friendly)
    int**  C;
    int    n;
    int    id_hilo;
    int    num_hilos;
} HiloArg;

// ============================================================
//  GESTIÓN DE MEMORIA
// ============================================================

int** crearMatriz(int n) {
    // Bloque contiguo de memoria para mejor localidad de caché
    int** matriz = (int**)malloc(n * sizeof(int*));
    int*  bloque = (int*)malloc(n * n * sizeof(int));
    for (int i = 0; i < n; i++)
        matriz[i] = bloque + i * n;
    return matriz;
}

void liberarMatriz(int** matriz) {
    free(matriz[0]);  // libera el bloque contiguo
    free(matriz);
}

void llenarMatrizAleatoria(int** matriz, int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            matriz[i][j] = rand() % 100;
}

// OPTIMIZACIÓN 1: Transponer B para acceso row-wise
// En lugar de B[k][j] (columnas = cache miss),
// usamos B_T[j][k] (filas = cache friendly)
int** transponerMatriz(int** B, int n) {
    int** B_T = crearMatriz(n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            B_T[j][i] = B[i][j];
    return B_T;
}

// ============================================================
//  FUNCIÓN DEL HILO — con cache blocking + B transpuesta
// ============================================================
void* multiplicar_chunk(void* arg) {
    HiloArg* ha    = (HiloArg*)arg;
    int**    A     = ha->A;
    int**    B_T   = ha->B_T;
    int**    C     = ha->C;
    int      n     = ha->n;
    int      id    = ha->id_hilo;
    int      nthrd = ha->num_hilos;

    // Distribución de filas por hilo
    int chunk    = n / nthrd;
    int fila_ini = id * chunk;
    int fila_fin = (id == nthrd - 1) ? n : fila_ini + chunk;

    // OPTIMIZACIÓN 2: Cache blocking (tiling)
    // Procesa submatrices de BLOCK_SIZE x BLOCK_SIZE
    // para maximizar reutilización de datos en L1/L2 cache
    for (int i = fila_ini; i < fila_fin; i += BLOCK_SIZE) {
        for (int j = 0; j < n; j += BLOCK_SIZE) {
            for (int k = 0; k < n; k += BLOCK_SIZE) {

                // Límites del bloque actual
                int i_max = i + BLOCK_SIZE < fila_fin ? i + BLOCK_SIZE : fila_fin;
                int j_max = j + BLOCK_SIZE < n        ? j + BLOCK_SIZE : n;
                int k_max = k + BLOCK_SIZE < n        ? k + BLOCK_SIZE : n;

                // Multiplicación dentro del bloque
                // OPTIMIZACIÓN 3: B_T[j][k] en lugar de B[k][j]
                // Ambos accesos son ahora row-wise → cache friendly
                for (int ii = i; ii < i_max; ii++) {
                    for (int jj = j; jj < j_max; jj++) {
                        long long acc = C[ii][jj]; // preservar acumulado entre bloques k
                        for (int kk = k; kk < k_max; kk++) {
                            acc += (long long)A[ii][kk] * B_T[jj][kk];
                        }
                        C[ii][jj] = (int)acc;
                    }
                }
            }
        }
    }
    return NULL;
}

// ============================================================
//  MULTIPLICACIÓN PARALELA CON CACHE OPTIMIZATIONS
// ============================================================
double multiplicarParalelo(int** A, int** B_T, int** C, int n, int num_hilos) {
    pthread_t* hilos = (pthread_t*)malloc(num_hilos * sizeof(pthread_t));
    HiloArg*   args  = (HiloArg*)malloc(num_hilos * sizeof(HiloArg));

    // Limpiar C antes de cada multiplicación
    memset(C[0], 0, n * n * sizeof(int));

    struct timespec inicio, fin;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    for (int t = 0; t < num_hilos; t++) {
        args[t].A        = A;
        args[t].B_T      = B_T;
        args[t].C        = C;
        args[t].n        = n;
        args[t].id_hilo  = t;
        args[t].num_hilos = num_hilos;
        pthread_create(&hilos[t], NULL, multiplicar_chunk, &args[t]);
    }
    for (int t = 0; t < num_hilos; t++)
        pthread_join(hilos[t], NULL);

    clock_gettime(CLOCK_MONOTONIC, &fin);

    free(hilos);
    free(args);

    return (fin.tv_sec  - inicio.tv_sec) +
           (fin.tv_nsec - inicio.tv_nsec) / 1e9;
}

// ============================================================
//  MAIN
// ============================================================
int main(int argc, char* argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Uso:     %s <num_hilos>\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s 4\n", argv[0]);
        fprintf(stderr, "Hilos validos: 2, 4, 8\n");
        return 1;
    }

    int num_hilos = atoi(argv[1]);
    if (num_hilos != 2 && num_hilos != 4 && num_hilos != 8) {
        fprintf(stderr, "Error: num_hilos debe ser 2, 4 u 8.\n");
        return 1;
    }

    srand((unsigned int)time(NULL));

    printf("=======================================================\n");
    printf("  Multiplicacion de Matrices — Cache Optimizado\n");
    printf("  Hilos: %d  |  Block size: %d  |  Intentos: %d\n",
           num_hilos, BLOCK_SIZE, NUM_INTENTOS);
    printf("  Optimizaciones: transpuesta-B + cache-blocking\n");
    printf("=======================================================\n\n");

    printf("%-8s  %-7s  %-14s\n", "n", "intento", "wall_clock_s");
    printf("%-8s  %-7s  %-14s\n", "--------", "-------", "--------------");

    for (int d = 0; d < NUM_DIMS; d++) {
        int n = DIMS[d];

        double suma   = 0.0;
        double minimo = 1e18;
        double maximo = 0.0;

        int** A   = crearMatriz(n);
        int** B   = crearMatriz(n);
        int** B_T = crearMatriz(n);
        int** C   = crearMatriz(n);

        llenarMatrizAleatoria(A, n);
        llenarMatrizAleatoria(B, n);

        // Transponer B una sola vez por dimensión
        int** tmp_B_T = transponerMatriz(B, n);
        // Copiar al bloque contiguo
        for (int i = 0; i < n; i++)
            memcpy(B_T[i], tmp_B_T[i], n * sizeof(int));
        liberarMatriz(tmp_B_T);

        for (int intento = 1; intento <= NUM_INTENTOS; intento++) {
            double t = multiplicarParalelo(A, B_T, C, n, num_hilos);

            printf("n=%-6d  t=%-5d   %.6f\n", n, intento, t);
            fflush(stdout);

            suma += t;
            if (t < minimo) minimo = t;
            if (t > maximo) maximo = t;
        }

        printf("  >> n=%-6d  hilos=%-2d  min=%.6f  max=%.6f  prom=%.6f\n\n",
               n, num_hilos, minimo, maximo, suma / NUM_INTENTOS);

        liberarMatriz(A);
        liberarMatriz(B);
        liberarMatriz(B_T);
        liberarMatriz(C);
    }

    printf("=======================================================\n");
    printf("  Pruebas finalizadas.\n");
    printf("=======================================================\n");

    return 0;
}
