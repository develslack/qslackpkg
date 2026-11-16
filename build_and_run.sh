#!/bin/bash
# ==============================================================================
# Script de compilación, ejecución, instalación y empaquetado para QSlackpkg
# Compatible con Slackware Linux 15.0 / -current
# ==============================================================================

set -e

PROJECT_FILE="qslackpkg.pro"
BINARY_NAME="qslackpkg"
APP_VERSION="1.0.0"
PKG_BUILD="1_qsl"

BUILD_ONLY=false
CLEAN_BUILD=false
INSTALL_SYSTEM=false
BUILD_SLACK_PKG=false
PKG_EXT="txz"
BUILD_RUN_INSTALLER=false

# Colores para salida en terminal
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m'

show_help() {
    echo "Uso: $0 [OPCIONES]"
    echo ""
    echo "Opciones:"
    echo "  -b, --build-only      Compila el proyecto sin ejecutar el binario al finalizar."
    echo "  -c, --clean           Limpia compilaciones previas (make distclean) antes de compilar."
    echo "  -i, --install         Compila e instala la aplicación globalmente en /opt/qslackpkg/ y crea la entrada en el menú Sistema."
    echo "  -p, --package         Genera un paquete nativo de Slackware (.txz) en ./dist/ listo para installpkg."
    echo "      --tgz             Igual que --package pero empaqueta en formato .tgz clásico."
    echo "  -r, --installer       Genera un instalador binario autocontenido (.run) estilo linuxdeployqt/makeself en ./dist/."
    echo "  -v, --version <ver>   Define la versión del paquete (por defecto: $APP_VERSION)."
    echo "  -h, --help            Muestra esta ayuda."
    echo ""
    echo "Ejemplos:"
    echo "  $0                        # Compila y lanza ./qslackpkg"
    echo "  $0 --clean --install      # Limpia, recompila e instala en el sistema"
    echo "  $0 --clean --package      # Genera dist/qslackpkg-${APP_VERSION}-x86_64-${PKG_BUILD}.txz"
    echo "  $0 --clean --installer    # Genera dist/qslackpkg-${APP_VERSION}-x86_64.run"
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
        -i|--install)
            INSTALL_SYSTEM=true
            BUILD_ONLY=true
            shift
            ;;
        -p|--package)
            BUILD_SLACK_PKG=true
            PKG_EXT="txz"
            BUILD_ONLY=true
            shift
            ;;
        --tgz)
            BUILD_SLACK_PKG=true
            PKG_EXT="tgz"
            BUILD_ONLY=true
            shift
            ;;
        -r|--installer)
            BUILD_RUN_INSTALLER=true
            BUILD_ONLY=true
            shift
            ;;
        -v|--version)
            APP_VERSION="$2"
            shift 2
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

# Detectar arquitectura del sistema para el nombre del paquete
ARCH=$(uname -m)
case "$ARCH" in
    i?86) ARCH="i586" ;;
    arm*) ARCH="arm" ;;
esac

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
    rm -f "$BINARY_NAME" moc_* *.o qrc_*
fi

# Generar Makefile
echo -e "${GREEN}>>> Generando Makefile con $QMAKE_BIN...${NC}"
"$QMAKE_BIN" "$PROJECT_FILE"

# Compilar usando todos los núcleos disponibles
JOBS=$(nproc 2>/dev/null || echo 2)
echo -e "${GREEN}>>> Compilando $BINARY_NAME usando $JOBS hilos...${NC}"
make -j"$JOBS"

# Eliminar símbolos de depuración para reducir tamaño del binario final
strip --strip-unneeded "$BINARY_NAME" 2>/dev/null || true

echo -e "${GREEN}>>> Compilación exitosa: ./$BINARY_NAME${NC}"

# Función auxiliar para armar el árbol de instalación (staging)
prepare_staging_tree() {
    local STAGE_DIR="$1"
    local SRC_BIN="$2"
    local SRC_IMG="$3"

    mkdir -p "$STAGE_DIR/opt/qslackpkg/img"
    mkdir -p "$STAGE_DIR/usr/bin"
    mkdir -p "$STAGE_DIR/usr/share/applications"
    mkdir -p "$STAGE_DIR/usr/share/pixmaps"
    mkdir -p "$STAGE_DIR/install"

    cp -f "$SRC_BIN" "$STAGE_DIR/opt/qslackpkg/$BINARY_NAME"
    chmod 755 "$STAGE_DIR/opt/qslackpkg/$BINARY_NAME"

    # Enlace simbólico en /usr/bin para poder invocarlo desde cualquier terminal
    ln -sf "/opt/qslackpkg/$BINARY_NAME" "$STAGE_DIR/usr/bin/$BINARY_NAME"

    if [[ -f "$SRC_IMG" ]]; then
        cp -f "$SRC_IMG" "$STAGE_DIR/opt/qslackpkg/img/qslackpkg.png"
        cp -f "$SRC_IMG" "$STAGE_DIR/usr/share/pixmaps/qslackpkg.png"
        chmod 644 "$STAGE_DIR/opt/qslackpkg/img/qslackpkg.png"
        chmod 644 "$STAGE_DIR/usr/share/pixmaps/qslackpkg.png"
    fi

    cat << 'EOF' > "$STAGE_DIR/usr/share/applications/qslackpkg.desktop"
[Desktop Entry]
Type=Application
Name=QSlackpkg
GenericName=Slackware Package & Kernel Manager
Comment=Package & Kernel Manager for Slackware Linux
Exec=/opt/qslackpkg/qslackpkg
Icon=/opt/qslackpkg/img/qslackpkg.png
Categories=System;PackageManager;
Terminal=false
EOF
    chmod 644 "$STAGE_DIR/usr/share/applications/qslackpkg.desktop"

    # Archivo slack-desc estándar (11 líneas exactas con prefijo 'qslackpkg:')
    cat << 'EOF' > "$STAGE_DIR/install/slack-desc"
         |-----handy-ruler------------------------------------------------------|
qslackpkg: qslackpkg (Visual Package & Kernel Manager for Slackware Linux)
qslackpkg:
qslackpkg: QSlackpkg is a native Qt5/C++ graphical software center and system
qslackpkg: updater for Slackware Linux. It integrates official slackpkg
qslackpkg: updates, SlackBuilds.org (sbopkg) with automatic dependency queue
qslackpkg: resolution, AlienBOB precompiled binary packages, .new file diff
qslackpkg: management, and safe initrd/bootloader configuration.
qslackpkg:
qslackpkg: Homepage: https://github.com/develslack/qslackpkg
qslackpkg:
qslackpkg:
EOF

    # Script post-instalación oficial de Slackware (doinst.sh)
    cat << 'EOF' > "$STAGE_DIR/install/doinst.sh"
if [ -x /usr/bin/update-desktop-database ]; then
  /usr/bin/update-desktop-database -q usr/share/applications >/dev/null 2>&1
fi
EOF
}

SRC_BIN="$(pwd)/$BINARY_NAME"
SRC_IMG="$(pwd)/img/Linux-Slackware-Logo-Vector.png"
DIST_DIR="$(pwd)/dist"

# ==============================================================================
# OPCIÓN 1: EMPAQUETADO NATIVO SLACKWARE (.txz / .tgz) CON MAKEPKG
# ==============================================================================
if [[ "$BUILD_SLACK_PKG" == true ]]; then
    echo -e "${CYAN}>>> Generando paquete nativo de Slackware (.${PKG_EXT})...${NC}"

    if ! command -v makepkg >/dev/null 2>&1; then
        echo -e "${RED}Error: No se encontró '/sbin/makepkg'. Este paso debe ejecutarse en Slackware.${NC}"
        exit 1
    fi

    mkdir -p "$DIST_DIR"
    PKG_FILENAME="${BINARY_NAME}-${APP_VERSION}-${ARCH}-${PKG_BUILD}.${PKG_EXT}"
    OUT_PKG_PATH="$DIST_DIR/$PKG_FILENAME"
    STAGE_DIR=$(mktemp -d /tmp/qslackpkg_pkg.XXXXXX)

    prepare_staging_tree "$STAGE_DIR" "$SRC_BIN" "$SRC_IMG"

    # makepkg requiere permisos root:root dentro del paquete; si no somos root, usamos fakeroot o pkexec
    if [[ "$EUID" -eq 0 ]]; then
        (
            cd "$STAGE_DIR"
            chown -R root:root .
            /sbin/makepkg -l y -c n "$OUT_PKG_PATH"
        )
    elif command -v fakeroot >/dev/null 2>&1; then
        fakeroot /bin/sh -c "
            cd '$STAGE_DIR'
            chown -R root:root .
            /sbin/makepkg -l y -c n '$OUT_PKG_PATH'
        "
    else
        echo -e "${YELLOW}>>> Solicitando permisos de root para asignar propiedad root:root al paquete...${NC}"
        pkexec /bin/sh -c "
            cd '$STAGE_DIR'
            chown -R root:root .
            /sbin/makepkg -l y -c n '$OUT_PKG_PATH'
            chown $(id -u):$(id -g) '$OUT_PKG_PATH'
        "
    fi

    rm -rf "$STAGE_DIR"

    # Generar suma MD5 junto al paquete (estilo oficial Slackware / AlienBOB)
    (cd "$DIST_DIR" && md5sum "$PKG_FILENAME" > "${PKG_FILENAME}.md5")

    echo -e "${GREEN}>>> ¡Paquete Slackware creado con éxito!${NC}"
    echo -e "    Archivo : ${CYAN}$OUT_PKG_PATH${NC}"
    echo -e "    MD5     : ${CYAN}${OUT_PKG_PATH}.md5${NC}"
    echo -e "    Instalar: ${YELLOW}upgradepkg --install-new $OUT_PKG_PATH${NC}"
fi

# ==============================================================================
# OPCIÓN 2: INSTALADOR BINARIO AUTOCONTENIDO (.run) ESTILO LINUXDEPLOYQT / MAKESELF
# ==============================================================================
if [[ "$BUILD_RUN_INSTALLER" == true ]]; then
    echo -e "${CYAN}>>> Generando instalador binario autocontenido (.run)...${NC}"

    mkdir -p "$DIST_DIR"
    RUN_FILENAME="${BINARY_NAME}-${APP_VERSION}-${ARCH}.run"
    OUT_RUN_PATH="$DIST_DIR/$RUN_FILENAME"
    STAGE_DIR=$(mktemp -d /tmp/qslackpkg_run.XXXXXX)
    PAYLOAD_TAR=$(mktemp /tmp/qslackpkg_payload.XXXXXX.tar.xz)

    prepare_staging_tree "$STAGE_DIR" "$SRC_BIN" "$SRC_IMG"
    rm -rf "$STAGE_DIR/install"

    # Comprimir todo el árbol de instalación en un payload tar.xz
    tar -C "$STAGE_DIR" -cJf "$PAYLOAD_TAR" .
    rm -rf "$STAGE_DIR"

    # Escribir la cabecera ejecutable + payload binario concatenado
    cat << 'EOF' > "$OUT_RUN_PATH"
#!/bin/sh
# Instalador autocontenido de QSlackpkg para Slackware Linux
set -e

echo "=================================================================="
echo "  Instalador de QSlackpkg - Package & Kernel Manager (Slackware)  "
echo "=================================================================="

ARCHIVE_LINE=$(awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' "$0")
TMP_EXTRACT=$(mktemp -d /tmp/qslackpkg_install.XXXXXX)
trap 'rm -rf "$TMP_EXTRACT"' EXIT

tail -n+"$ARCHIVE_LINE" "$0" | tar -xJf - -C "$TMP_EXTRACT"

INSTALL_CMD="
    cp -rf '$TMP_EXTRACT/opt/'* /opt/
    cp -rf '$TMP_EXTRACT/usr/'* /usr/
    chown -R root:root /opt/qslackpkg /usr/share/applications/qslackpkg.desktop /usr/share/pixmaps/qslackpkg.png
    chmod 755 /opt/qslackpkg/qslackpkg
    update-desktop-database /usr/share/applications 2>/dev/null || true
"

if [ "$(id -u)" -eq 0 ]; then
    /bin/sh -c "$INSTALL_CMD"
elif command -v pkexec >/dev/null 2>&1; then
    echo "Solicitando contraseña de administrador (root) para instalar en /opt/qslackpkg..."
    pkexec /bin/sh -c "$INSTALL_CMD"
else
    echo "Ingresa la contraseña de root para instalar QSlackpkg:"
    su -c "$INSTALL_CMD"
fi

echo ""
echo ">>> ¡QSlackpkg instalado correctamente!"
echo ">>> Puedes abrirlo desde Aplicaciones -> Sistema -> QSlackpkg o ejecutando 'qslackpkg'."
exit 0
__ARCHIVE_BELOW__
EOF

    cat "$PAYLOAD_TAR" >> "$OUT_RUN_PATH"
    rm -f "$PAYLOAD_TAR"
    chmod 755 "$OUT_RUN_PATH"

    (cd "$DIST_DIR" && md5sum "$RUN_FILENAME" > "${RUN_FILENAME}.md5")

    echo -e "${GREEN}>>> ¡Instalador autocontenido creado con éxito!${NC}"
    echo -e "    Archivo : ${CYAN}$OUT_RUN_PATH${NC}"
    echo -e "    Ejecutar: ${YELLOW}./dist/$RUN_FILENAME${NC} (o doble clic desde el gestor de archivos)"
fi

# ==============================================================================
# OPCIÓN 3: INSTALACIÓN DIRECTA EN EL SISTEMA (--install)
# ==============================================================================
if [[ "$INSTALL_SYSTEM" == true ]]; then
    echo -e "${GREEN}>>> Instalando QSlackpkg en el sistema (solicitando permisos de superusuario)...${NC}"

    if [[ ! -f "$SRC_BIN" ]]; then
        echo -e "${RED}Error: No se encuentra el binario compilado en '$SRC_BIN'.${NC}"
        exit 1
    fi

    pkexec /bin/sh -c "
        mkdir -p /opt/qslackpkg/img
        cp -f '$SRC_BIN' /opt/qslackpkg/
        ln -sf /opt/qslackpkg/qslackpkg /usr/bin/qslackpkg
        if [ -f '$SRC_IMG' ]; then
            cp -f '$SRC_IMG' /opt/qslackpkg/img/
            cp -f '$SRC_IMG' /usr/share/pixmaps/qslackpkg.png
        else
            echo 'Aviso: No se encontró el logo PNG en $SRC_IMG'
        fi

        cat << 'EOF' > /usr/share/applications/qslackpkg.desktop
[Desktop Entry]
Type=Application
Name=QSlackpkg
GenericName=Slackware Package & Kernel Manager
Comment=Package & Kernel Manager for Slackware Linux
Exec=/opt/qslackpkg/qslackpkg
Icon=/opt/qslackpkg/img/Linux-Slackware-Logo-Vector.png
Categories=System;PackageManager;
Terminal=false
EOF
        chmod 644 /usr/share/applications/qslackpkg.desktop
        update-desktop-database 2>/dev/null || true
    "
    echo -e "${GREEN}>>> ¡Instalación completada con éxito! QSlackpkg ya se encuentra en el menú Aplicaciones -> Sistema.${NC}"
    exit 0
fi

# Ejecutar si no se indicó --build-only, --install, --package ni --installer
if [[ "$BUILD_ONLY" == false ]]; then
    echo -e "${GREEN}>>> Iniciando $BINARY_NAME...${NC}"
    ./"$BINARY_NAME"
fi
