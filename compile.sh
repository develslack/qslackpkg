#!/bin/bash
# ==============================================================================
# Script de compilación y ejecución para QSlackpkg en Slackware Linux
# ==============================================================================

set -e

PROJECT_FILE="qslackpkg.pro"
BINARY_NAME="qslackpkg"
BUILD_ONLY=false
CLEAN_BUILD=false

# Colores para salida en terminal
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

show_help() {
    echo "Uso: $0 [OPCIONES]"
    echo ""
    echo "Opciones:"
    echo "  -b, --build-only    Compila el proyecto sin ejecutar el binario al finalizar."
    echo "  -c, --clean         Limpia compilaciones previas (make distclean) antes de compilar."
    echo "  -h, --help          Muestra esta ayuda."
    echo ""
    echo "Ejemplo:"
    echo "  $0                  # Compila y lanza ./qslackpkg"
    echo "  $0 --clean          # Recompila desde cero y lanza ./qslackpkg"
}

# Parseo de argumentos
while [[ $# -gt 0 ]]; do
    case "$1" in
        -b|--build-only)
            BUILD_ONLY=true
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}Error: Opción desconocida '$1'${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Verificar que estamos en el directorio del proyecto
if [[ ! -f "$PROJECT_FILE" ]]; then
    echo -e "${RED}Error: No se encontró '$PROJECT_FILE' en el directorio actual.${NC}"
    echo "Ejecuta este script dentro de la carpeta raíz del proyecto."
    exit 1
fi

# Detectar qmake (Slackware usa qmake-qt5 por defecto)
QMAKE_BIN=""
if command -v qmake-qt5 >/dev/null 2>&1; then
    QMAKE_BIN="qmake-qt5"
elif command -v qmake >/dev/null 2>&1; then
    QMAKE_BIN="qmake"
else
    echo -e "${RED}Error: No se encontró 'qmake-qt5' ni 'qmake' en el sistema.${NC}"
    echo "Asegúrate de tener instalado el paquete qt5 de Slackware."
    exit 1
fi

# Limpieza opcional
if [[ "$CLEAN_BUILD" == true ]]; then
    echo -e "${YELLOW}>>> Limpiando compilación previa...${NC}"
    if [[ -f "Makefile" ]]; then
        make distclean >/dev/null 2>&1 || true
    fi
    rm -f "$BINARY_NAME" moc_* *.o
fi

# Generar Makefile
echo -e "${GREEN}>>> Generando Makefile con $QMAKE_BIN...${NC}"
"$QMAKE_BIN" "$PROJECT_FILE"

# Compilar usando todos los núcleos disponibles
JOBS=$(nproc 2>/dev/null || echo 2)
echo -e "${GREEN}>>> Compilando $BINARY_NAME usando $JOBS hilos...${NC}"
make -j"$JOBS"

echo -e "${GREEN}>>> Compilación exitosa: ./$BINARY_NAME${NC}"

# Ejecutar si no se indicó --build-only
if [[ "$BUILD_ONLY" == false ]]; then
    echo -e "${GREEN}>>> Iniciando $BINARY_NAME...${NC}"
    ./"$BINARY_NAME"
fi