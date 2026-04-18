#!/bin/bash

SIZES=(500 1000 1500 2000 3000)
REPS=3

mkdir -p resultados

for N in "${SIZES[@]}"
do
  echo "=============================="
  echo "Ejecutando N=$N"
  echo "=============================="

  DIR="resultados/N$N"
  mkdir -p $DIR

  # Compilar versión normal
  gcc -O2 -DN=$N m_matrices_vol1.c -o matmul

  # TIME (3 veces)
  for i in $(seq 1 $REPS)
  do
    /usr/bin/time -v ./matmul 2>> $DIR/time.txt
  done

  # PERF (3 veces)
  for i in $(seq 1 $REPS)
  do
    perf stat ./matmul 2>> $DIR/perf.txt
  done

  # CACHEGRIND (1 vez)
  valgrind --tool=cachegrind ./matmul 2> $DIR/cache_raw.txt
  CGFILE=$(ls cachegrind.out.* | tail -n 1)
  mv $CGFILE $DIR/
  cg_annotate $DIR/cachegrind.out.* > $DIR/cache_detalle.txt

  # GPROF (1 vez)
  gcc -pg -O2 -DN=$N m_matrices_vol1.c -o matmul_gprof
  ./matmul_gprof
  gprof matmul_gprof gmon.out > $DIR/gprof.txt
  mv gmon.out $DIR/gmon.out

done

echo "✅ Todo el profiling terminó"
