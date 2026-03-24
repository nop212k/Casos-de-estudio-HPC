#!/bin/bash

# ============================================================
#  Script: run_hilos.sh
#  Descripción: Compila y ejecuta multiplicación de matrices
#               con hilos para distintas dimensiones y número
#               de hilos, repitiendo cada combinación N veces.
#  Uso: bash run_hilos.sh
# ============================================================

# --- Configuración ---
SOURCE="mul_hilos.c"               # Archivo fuente C
BINARY="./mul_hilos_bin"           # Binario compilado
OUTPUT="tiempos_hilos.txt"         # Archivo de salida con resultados
REPETICIONES=10                    # Número de repeticiones por dimensión

# Dimensiones a probar
DIMENSIONES=(500 1000 1500 2000 3000)

# Número de hilos a probar
HILOS=(2 4 8)


# --- Limpiar archivo de salida anterior ---
echo "============================================================"  > "$OUTPUT"
echo "  Resultados: Multiplicación de Matrices con Hilos"          >> "$OUTPUT"
echo "  Fecha: $(date)"                                             >> "$OUTPUT"
echo "  Repeticiones por combinación: $REPETICIONES"               >> "$OUTPUT"
echo "============================================================" >> "$OUTPUT"
echo "" >> "$OUTPUT"

# --- Ejecución ---
for num_hilos in "${HILOS[@]}"
do
    echo "============================================================" >> "$OUTPUT"
    echo "  HILOS: $num_hilos"                                          >> "$OUTPUT"
    echo "============================================================" >> "$OUTPUT"
    echo "" >> "$OUTPUT"

    for n in "${DIMENSIONES[@]}"
    do
        echo "------------------------------------------------------------" >> "$OUTPUT"
        echo "  n=$n  hilos=$num_hilos" >> "$OUTPUT"
        echo "------------------------------------------------------------" >> "$OUTPUT"

        echo "Ejecutando n=$n con $num_hilos hilos ($REPETICIONES repeticiones)..."

        for (( rep=1; rep<=REPETICIONES; rep++ ))
        do
            echo -n "  Rep $rep/$REPETICIONES ... "
            resultado=$("$BINARY" "$n" "$num_hilos" 2>&1)
            echo "$resultado" >> "$OUTPUT"
            echo "listo."
        done

        echo "" >> "$OUTPUT"
    done
done

echo "============================================================" >> "$OUTPUT"
echo "  Fin de la ejecución." >> "$OUTPUT"
echo "============================================================" >> "$OUTPUT"

echo ""
echo "Todas las ejecuciones completadas."
echo "Resultados guardados en: $OUTPUT"
