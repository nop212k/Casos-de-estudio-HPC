#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>

#define CACHELINE 64

double f_src(double x) { return 1.0; }

typedef struct {
    sem_t  sem_work;
    sem_t  sem_done;
    sem_t  sem_ready;
    double error_global;
    int    iter_global;
    int    converged;
    int    swap;        /* 0 = u0 es old, 1 = u1 es old */
} Sync;

int main(int argc, char *argv[]) {
    int N      = (argc > 1) ? atoi(argv[1]) : 500;
    int NPROCS = (argc > 2) ? atoi(argv[2]) : 4;
    double TOL     = 1e-6;
    int    MAXITER = 100000;

    if (N <= 0 || NPROCS <= 0) {
        fprintf(stderr, "Uso: %s <N> <procesos>\n", argv[0]); return 1;
    }

    double h  = 1.0 / (N + 1);
    double h2 = h * h;
    size_t sz = (size_t)(N + 2) * sizeof(double);

    /* dos arreglos fijos en shm: u0 y u1, se alternan como old/new */
    double *u0 = mmap(NULL, sz, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    double *u1 = mmap(NULL, sz, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    double *rhs = mmap(NULL, sz, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    double *local_errors = mmap(NULL, NPROCS*sizeof(double),
                                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    Sync *sync = mmap(NULL, sizeof(Sync), PROT_READ|PROT_WRITE,
                      MAP_SHARED|MAP_ANONYMOUS, -1, 0);

    if (!u0 || !u1 || !rhs || !local_errors || !sync) {
        fprintf(stderr, "Error: mmap\n"); return 1;
    }

    for (int i = 0; i < N+2; i++) { u0[i]=0.0; u1[i]=0.0; }
    for (int i = 1; i <= N; i++) rhs[i] = h2 * f_src(i*h);

    sem_init(&sync->sem_work,  1, 0);
    sem_init(&sync->sem_done,  1, 0);
    sem_init(&sync->sem_ready, 1, 0);
    sync->error_global = TOL + 1.0;
    sync->iter_global  = 0;
    sync->converged    = 0;
    sync->swap         = 0;

    int chunk = (N + NPROCS - 1) / NPROCS;

    for (int p = 0; p < NPROCS; p++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return 1; }
        if (pid == 0) {
            int start = p * chunk + 1;
            int end   = start + chunk - 1;
            if (end > N) end = N;

            while (1) {
                sem_wait(&sync->sem_work);
                if (sync->converged) break;

                /* determinar cuál arreglo es old y cuál es new */
                double *old = (sync->swap == 0) ? u0 : u1;
                double *newv = (sync->swap == 0) ? u1 : u0;

                double le = 0.0;
                for (int i = start; i <= end; i++) {
                    newv[i] = (old[i-1] + old[i+1] + rhs[i]) * 0.5;
                    double d = fabs(newv[i] - old[i]);
                    if (d > le) le = d;
                }
                local_errors[p] = le;

                sem_post(&sync->sem_done);
                sem_wait(&sync->sem_ready);
            }
            exit(0);
        }
    }

    /* padre */
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    while (sync->error_global > TOL && sync->iter_global < MAXITER) {
        for (int p = 0; p < NPROCS; p++) sem_post(&sync->sem_work);
        for (int p = 0; p < NPROCS; p++) sem_wait(&sync->sem_done);

        double max_e = 0.0;
        for (int p = 0; p < NPROCS; p++)
            if (local_errors[p] > max_e) max_e = local_errors[p];
        sync->error_global = max_e;

        /* alternar swap en vez de mover datos */
        sync->swap = 1 - sync->swap;
        sync->iter_global++;

        for (int p = 0; p < NPROCS; p++) sem_post(&sync->sem_ready);
    }

    sync->converged = 1;
    for (int p = 0; p < NPROCS; p++) sem_post(&sync->sem_work);
    for (int p = 0; p < NPROCS; p++) wait(NULL);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    printf("procesos,%d,%d,%.6f,%d,%.2e\n",
           N, NPROCS, elapsed, sync->iter_global, sync->error_global);

    sem_destroy(&sync->sem_work);
    sem_destroy(&sync->sem_done);
    sem_destroy(&sync->sem_ready);
    munmap(u0,sz); munmap(u1,sz); munmap(rhs,sz);
    munmap(local_errors, NPROCS*sizeof(double));
    munmap(sync, sizeof(Sync));
    return 0;
}
