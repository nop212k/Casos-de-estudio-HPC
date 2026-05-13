#!/bin/bash

# ============================================
# analisis_hilos.sh
# Benchmark tráfico celular:
# - Pthreads
# - OpenMP
#
# Guarda resultados para análisis posterior
# ============================================

# Compilación
gcc -O2 -pthread traffich.c -o traffich
gcc -O2 -fopenmp traffichomp.c -o traffichomp

# Parámetros base
ROAD_SIZES=(1000000 5000000 10000000)
THREADS=(1 2 4 8)

N_WARMUP=500
N_MEASURE=1000

# Archivos salida
PTHREAD_OUT="results_pthreadsreto2.csv"
OMP_OUT="results_openmpreto2.csv"

# Limpiar archivos viejos
rm -f $PTHREAD_OUT
rm -f $OMP_OUT

# Headers CSV
echo "implementation,road_n,threads,density,cars,velocity,time" >> $PTHREAD_OUT
echo "implementation,road_n,threads,density,cars,velocity,time" >> $OMP_OUT

# ============================================
# PTHREADS
# ============================================

echo "====================================="
echo "BENCHMARK PTHREADS"
echo "====================================="

for SIZE in "${ROAD_SIZES[@]}"
do
    for T in "${THREADS[@]}"
    do
        echo "Running Pthreads -> SIZE=$SIZE THREADS=$T"

        ./traffich $SIZE $N_WARMUP $N_MEASURE $T | while read line
        do
            # Ignorar comentarios
            if [[ $line == \#* ]]; then
                continue
            fi

            density=$(echo $line | awk '{print $1}')
            cars=$(echo $line | awk '{print $2}')
            velocity=$(echo $line | awk '{print $3}')
            time=$(echo $line | awk '{print $4}')

            echo "pthreads,$SIZE,$T,$density,$cars,$velocity,$time" >> $PTHREAD_OUT
        done
    done
done

# ============================================
# OPENMP
# ============================================

echo "====================================="
echo "BENCHMARK OPENMP"
echo "====================================="

for SIZE in "${ROAD_SIZES[@]}"
do
    for T in "${THREADS[@]}"
    do
        echo "Running OpenMP -> SIZE=$SIZE THREADS=$T"

        ./traffichomp $SIZE $N_WARMUP $N_MEASURE $T | while read line
        do
            # Ignorar comentarios
            if [[ $line == \#* ]]; then
                continue
            fi

            density=$(echo $line | awk '{print $1}')
            cars=$(echo $line | awk '{print $2}')
            velocity=$(echo $line | awk '{print $3}')
            time=$(echo $line | awk '{print $4}')

            echo "openmp,$SIZE,$T,$density,$cars,$velocity,$time" >> $OMP_OUT
        done
    done
done

echo ""
echo "====================================="
echo "Benchmark terminado"
echo "====================================="
echo "Resultados:"
echo " - $PTHREAD_OUT"
echo " - $OMP_OUT"