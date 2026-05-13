#!/bin/bash

# Crear carpeta de salida
DIR="resultados_profiling"
mkdir -p $DIR

# Definir los escenarios: "ROAD_N N_WARMUP N_MEASURE"
# Original: ~150 millones de ciclos
# Medio:    ~1000 millones de ciclos
# Alto:     ~10,000 millones de ciclos (Alta precisión de profiling)
declare -a configs=(
    "5000 500 1000"
    "50000 1000 2000"
    "500000 2000 5000"
)
nombres=("bajo" "medio" "alto")

for i in "${!configs[@]}"; do
    config="${configs[$i]}"
    nombre="${nombres[$i]}"
    
    echo "================================================="
    echo "INICIANDO ESCENARIO: $nombre ($config)"
    echo "================================================="

    # 1. VALGRIND (Requiere -g, corre lento así que probamos memoria a fondo)
    echo "[1/3] Revisando memoria con Valgrind..."
    gcc -g -o traffic traffic.c
    valgrind --leak-check=full ./traffic $config > $DIR/simulacion_${nombre}.txt 2> $DIR/valgrind_${nombre}.txt

    # 2. GPROF (Requiere -O2 -pg)
    echo "[2/3] Generando perfil plano con Gprof..."
    gcc -O2 -pg -o traffic traffic.c
    ./traffic $config > /dev/null
    gprof traffic gmon.out > $DIR/gprof_${nombre}.txt

    # 3. PERF (Requiere -O2)
    echo "[3/3] Analizando ciclos de CPU y caché con Perf..."
    gcc -O2 -g -o traffic traffic.c
    
    # Perf Stat (Resumen de uso de hardware)
    perf stat -e cpu-cycles,instructions,cache-misses ./traffic $config > /dev/null 2> $DIR/perf_stat_${nombre}.txt
    
    # Perf Record y Report (Para ver dónde se concentra el tiempo)
    perf record -o perf.data ./traffic $config > /dev/null 2>&1
    perf report --stdio -i perf.data > $DIR/perf_report_${nombre}.txt

    echo "Escenario $nombre completado."
    echo ""
done

# Limpiar archivos temporales de ejecución
rm -f traffic gmon.out perf.data

echo "¡Análisis 100% completado!"
echo "Todos los reportes están en la carpeta '$DIR'"