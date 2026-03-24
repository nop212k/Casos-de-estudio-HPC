#!/bin/bash

NS=(100 500 1000 2000 5000 10000)
INTENTOS=10
DIR="$(cd "$(dirname "$0")" && pwd)"
RESDIR="$DIR/resultados"
mkdir -p "$RESDIR"

# ---- archivos de salida ----------------------------------
FILES=(
"serial_O0.txt"
"serial_O1.txt"
"serial_O2.txt"
"serial_O3.txt"
"serial_Ofast.txt"
"cacheline.txt"
"hilos_2.txt"
"hilos_4.txt"
"hilos_8.txt"
"procs_2.txt"
"procs_4.txt"
"procs_8.txt"
)

# limpiar archivos
for f in "${FILES[@]}"; do
    > "$RESDIR/$f"
done

# ---- compilación ----------------------------------------
echo "COMPILANDO..."

gcc -O0 -o "$DIR/jac_O0" jacobis.c -lm
gcc -O1 -o "$DIR/jac_O1" jacobis.c -lm
gcc -O2 -o "$DIR/jac_O2" jacobis.c -lm
gcc -O3 -o "$DIR/jac_O3" jacobis.c -lm
gcc -Ofast -march=native -ffast-math -o "$DIR/jac_Ofast" jacobis.c -lm
gcc -O0 -o "$DIR/jac_cache" jacobic.c -lm
gcc -O2 -o "$DIR/jac_hilos" jacobih.c -lm -lpthread
gcc -O2 -o "$DIR/jac_procs" jacobip.c -lm

# ---- función de ejecución --------------------------------
run_and_save() {
    local file=$1
    local bin=$2
    local args=$3

    for i in $(seq 1 $INTENTOS); do
        r=$("$DIR/$bin" $args 2>/dev/null)
        echo "$r" >> "$RESDIR/$file"
        echo "$r"
    done
}

# ---- ejecución ------------------------------------------
for N in "${NS[@]}"; do
    echo "==== N=$N ===="

    # SERIAL
    run_and_save "serial_O0.txt"    jac_O0    "$N"
    run_and_save "serial_O1.txt"    jac_O1    "$N"
    run_and_save "serial_O2.txt"    jac_O2    "$N"
    run_and_save "serial_O3.txt"    jac_O3    "$N"
    run_and_save "serial_Ofast.txt" jac_Ofast "$N"

    # CACHELINE
    run_and_save "cacheline.txt" jac_cache "$N"

    # HILOS
    for T in 2 4 8; do
        run_and_save "hilos_${T}.txt" jac_hilos "$N $T"
    done

    # PROCESOS
    for P in 2 4 8; do
        run_and_save "procs_${P}.txt" jac_procs "$N $P"
    done

done

echo "==== COMPLETADO ===="
echo "Resultados en: $RESDIR"