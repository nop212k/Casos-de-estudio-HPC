#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Función para asignar memoria para una matriz cuadrada
int** crearMatriz(int n) {
    int** matriz = (int**)malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++) {
        matriz[i] = (int*)malloc(n * sizeof(int));
    }
    return matriz;
}

// Función para liberar memoria de una matriz
void liberarMatriz(int** matriz, int n) {
    for (int i = 0; i < n; i++) {
        free(matriz[i]);
    }
    free(matriz);
}

// Función para llenar una matriz con valores aleatorios
void llenarMatrizAleatoria(int** matriz, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matriz[i][j] = rand() % 100; // Valores aleatorios entre 0 y 99
        }
    }
}

// Función para multiplicar dos matrices
int** multiplicarMatrices(int** A, int** B, int n) {
    int** C = crearMatriz(n);
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = 0;
            for (int k = 0; k < n; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    
    return C;
}

// Función para imprimir una matriz
void imprimirMatriz(int** matriz, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%6d ", matriz[i][j]);
        }
        printf("\n");
    }
}

int main(int argc, char* argv[]) {
    int n;

    // Leer dimensión desde argumento de línea de comandos
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <dimension>\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s 500\n", argv[0]);
        return 1;
    }

    n = atoi(argv[1]);

    if (n <= 0) {
        fprintf(stderr, "El tamaño debe ser un número positivo.\n");
        return 1;
    }

    // Inicializar semilla para números aleatorios
    srand((unsigned int)time(NULL));

    // Crear las matrices
    int** A = crearMatriz(n);
    int** B = crearMatriz(n);

    // Llenar las matrices con valores aleatorios
    llenarMatrizAleatoria(A, n);
    llenarMatrizAleatoria(B, n);

    // Mostrar las matrices (solo si n <= 10 para no saturar la pantalla)
    if (n <= 10) {
        printf("Matriz A:\n");
        imprimirMatriz(A, n);
        printf("\nMatriz B:\n");
        imprimirMatriz(B, n);
    }

    // Medir wall clock SOLO de la multiplicación
    struct timespec inicio, fin;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    int** C = multiplicarMatrices(A, B, n);

    clock_gettime(CLOCK_MONOTONIC, &fin);

    double wall_clock = (fin.tv_sec - inicio.tv_sec) +
                        (fin.tv_nsec - inicio.tv_nsec) / 1e9;

    // Mostrar resultado
    if (n <= 10) {
        printf("\nMatriz C = A x B:\n");
        imprimirMatriz(C, n);
    }

    // Imprimir solo el wall clock time
    printf("n=%d  wall_clock=%.6f s\n", n, wall_clock);

    // Liberar memoria
    liberarMatriz(A, n);
    liberarMatriz(B, n);
    liberarMatriz(C, n);

    return 0;
}
