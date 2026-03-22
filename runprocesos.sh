#!/bin/bash

# ============================================================
#  Script: run_procesos.sh
#  Descripción: Ejecuta multiplicación de matrices con
#               procesos para distintas dimensiones y número
#               de procesos, repitiendo cada combinación N veces.
#  Uso: bash run_procesos.sh
# ============================================================

# --- Configuración ---
BINARY="./mul_procesos"              # Binario compilado
OUTPUT="tiempos_procesos.txt"        # Archivo de salida con resultados
REPETICIONES=10                      # Número de repeticiones por combinación

# Dimensiones a probar
DIMENSIONES=(500 1000 1500 2000 3000)

# Número de procesos a probar
PROCESOS=(2 4 8)

# --- Limpiar archivo de salida anterior ---
echo "============================================================"  > "$OUTPUT"
echo "  Resultados: Multiplicación de Matrices con Procesos"        >> "$OUTPUT"
echo "  Fecha: $(date)"                                             >> "$OUTPUT"
echo "  Repeticiones por combinación: $REPETICIONES"               >> "$OUTPUT"
echo "============================================================" >> "$OUTPUT"
echo "" >> "$OUTPUT"

# --- Ejecución ---
for num_procs in "${PROCESOS[@]}"
do
    echo "============================================================" >> "$OUTPUT"
    echo "  PROCESOS: $num_procs"                                       >> "$OUTPUT"
    echo "============================================================" >> "$OUTPUT"
    echo "" >> "$OUTPUT"

    for n in "${DIMENSIONES[@]}"
    do
        echo "------------------------------------------------------------" >> "$OUTPUT"
        echo "  n=$n  procesos=$num_procs" >> "$OUTPUT"
        echo "------------------------------------------------------------" >> "$OUTPUT"

        echo "Ejecutando n=$n con $num_procs procesos ($REPETICIONES repeticiones)..."

        for (( rep=1; rep<=REPETICIONES; rep++ ))
        do
            echo -n "  Rep $rep/$REPETICIONES ... "
            resultado=$("$BINARY" "$n" "$num_procs" 2>&1)
            echo "$resultado" >> "$OUTPUT"
            echo "listo."
        done

        echo "" >> "$OUTPUT"
    done
done

echo "============================================================" >> "$OUTPUT"
echo "  Fin de la ejecución."                                       >> "$OUTPUT"
echo "============================================================" >> "$OUTPUT"

echo ""
echo "Todas las ejecuciones completadas."
echo "Resultados guardados en: $OUTPUT"