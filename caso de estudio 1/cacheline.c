#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int** crearMatriz(int n) {
    int** matriz = (int**)malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++) {
        matriz[i] = (int*)malloc(n * sizeof(int));
    }
    return matriz;
}

void liberarMatriz(int** matriz, int n) {
    for (int i = 0; i < n; i++) {
        free(matriz[i]);
    }
    free(matriz);
}

void llenarMatrizAleatoria(int** matriz, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matriz[i][j] = rand() % 100;
        }
    }
}

// AT ya viene traspuesta desde afuera
// C[i][j] = sum_k AT[k][i] * B[k][j]
int** multiplicarMatrices(int** AT, int** B, int n) {
    int** C = crearMatriz(n);

    // Inicializar C en cero
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            C[i][j] = 0;

    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            int at_ki = AT[k][i];  // escalar, evita re-acceder a AT[k][i] en el loop j
            for (int j = 0; j < n; j++) {
                C[i][j] += at_ki * B[k][j];  // B[k][j]: acceso secuencial por fila
            }
        }
    }

    return C;
}

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

    srand((unsigned int)time(NULL));

    // AT representa A ya traspuesta (viene dada así)
    int** AT = crearMatriz(n);
    int** B  = crearMatriz(n);

    llenarMatrizAleatoria(AT, n);
    llenarMatrizAleatoria(B, n);

    if (n <= 10) {
        printf("Matriz AT (A traspuesta):\n");
        imprimirMatriz(AT, n);
        printf("\nMatriz B:\n");
        imprimirMatriz(B, n);
    }

    struct timespec inicio, fin;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    int** C = multiplicarMatrices(AT, B, n);

    clock_gettime(CLOCK_MONOTONIC, &fin);

    double wall_clock = (fin.tv_sec - inicio.tv_sec) +
                        (fin.tv_nsec - inicio.tv_nsec) / 1e9;

    if (n <= 10) {
        printf("\nMatriz C = A x B:\n");
        imprimirMatriz(C, n);
    }

    printf("n=%d  wall_clock=%.6f s\n", n, wall_clock);

    liberarMatriz(AT, n);
    liberarMatriz(B, n);
    liberarMatriz(C, n);

    return 0;
}