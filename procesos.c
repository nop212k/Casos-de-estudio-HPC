#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>

// ============================================================
//  GESTIÓN DE MEMORIA COMPARTIDA
// ============================================================
int* crearShm(int n, int* shmid) {
    *shmid = shmget(IPC_PRIVATE, n * n * sizeof(int), IPC_CREAT | 0666);
    if (*shmid < 0) { perror("shmget"); exit(1); }
    int* ptr = (int*)shmat(*shmid, NULL, 0);
    if (ptr == (int*)-1) { perror("shmat"); exit(1); }
    return ptr;
}

void liberarShm(int* ptr, int shmid) {
    shmdt(ptr);
    shmctl(shmid, IPC_RMID, NULL);
}

// ============================================================
//  MULTIPLICACIÓN (ejecutada por cada proceso hijo)
// ============================================================
void multiplicar_chunk(int* A, int* B_T, int* C,
                       int n, int id, int num_procs) {
    int chunk    = n / num_procs;
    int fila_ini = id * chunk;
    int fila_fin = (id == num_procs - 1) ? n : fila_ini + chunk;

    for (int i = fila_ini; i < fila_fin; i++) {
        for (int k = 0; k < n; k++) {
            long long a_ik = A[i * n + k];
            for (int j = 0; j < n; j++) {
                C[i * n + j] += a_ik * B_T[k * n + j];
            }
        }
    }
}

// ============================================================
//  LLENAR MATRIZ
// ============================================================
void llenarMatrizAleatoria(int* matriz, int n) {
    for (int i = 0; i < n * n; i++)
        matriz[i] = rand() % 100;
}

void transponerMatriz(int* B, int* B_T, int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            B_T[j * n + i] = B[i * n + j];
}

// ============================================================
//  MAIN
// ============================================================
int main(int argc, char* argv[]) {

    if (argc < 3) {
        fprintf(stderr, "Uso:     %s <dimension> <num_procesos>\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s 1000 4\n", argv[0]);
        return 1;
    }

    int n         = atoi(argv[1]);
    int num_procs = atoi(argv[2]);

    if (n <= 0 || num_procs <= 0) {
        fprintf(stderr, "Error: dimension y num_procesos deben ser positivos.\n");
        return 1;
    }

    srand((unsigned int)time(NULL));

    // Crear matrices en memoria compartida
    int shmid_A, shmid_B, shmid_BT, shmid_C;
    int* A   = crearShm(n, &shmid_A);
    int* B   = crearShm(n, &shmid_B);
    int* B_T = crearShm(n, &shmid_BT);
    int* C   = crearShm(n, &shmid_C);

    llenarMatrizAleatoria(A, n);
    llenarMatrizAleatoria(B, n);
    memset(C, 0, n * n * sizeof(int));
    transponerMatriz(B, B_T, n);

    // Medir wall clock SOLO de la multiplicación
    struct timespec inicio, fin;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    // Crear procesos hijo
    for (int id = 0; id < num_procs; id++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            // Proceso hijo
            multiplicar_chunk(A, B_T, C, n, id, num_procs);
            shmdt(A); shmdt(B); shmdt(B_T); shmdt(C);
            exit(0);
        }
    }

    // Padre espera a todos los hijos
    for (int i = 0; i < num_procs; i++)
        wait(NULL);

    clock_gettime(CLOCK_MONOTONIC, &fin);

    double wall_clock = (fin.tv_sec  - inicio.tv_sec) +
                        (fin.tv_nsec - inicio.tv_nsec) / 1e9;

    printf("n=%-6d  procesos=%-2d  wall_clock=%.6f s\n", n, num_procs, wall_clock);

    // Liberar memoria compartida
    liberarShm(A,   shmid_A);
    liberarShm(B,   shmid_B);
    liberarShm(B_T, shmid_BT);
    liberarShm(C,   shmid_C);

    return 0;
}