# QSlackpkg — Centro de Software y Gestor Visual de Actualizaciones para Slackware

**QSlackpkg** es una interfaz gráfica nativa desarrollada en **C++ y Qt5** diseñada para modernizar y simplificar la administración de paquetes en **Slackware Linux** (15.0 / -current), manteniendo intacta la filosofía, solidez y herramientas oficiales de la distribución.

Está pensada tanto para veteranos de Slackware que desean un monitor silencioso en la bandeja del sistema, como para usuarios provenientes de distribuciones como **Linux Mint** o **Ubuntu** que buscan instalar software y resolver dependencias sin tener que gestionar manualmente scripts y colas en la terminal.

---

## Características Principales

### 1. Gestión del Sistema Oficial (`slackpkg`)
* **Sincronización exacta:** Consulta directamente el motor de `slackpkg` para garantizar 100% de coincidencia entre la interfaz gráfica y la terminal.
* **Vista estructurada en 6 columnas:** Diferencia claramente el nombre del paquete, el paquete base de la rama principal (`slackware64`), los paquetes compilados desde SlackBuilds (`_SBo`), la versión instalada actualmente (resaltada en verde cuando está al día) y las actualizaciones disponibles.
* **Desinstalación directa:** Incluye un botón `Remove` por cada paquete instalado (`slackpkg remove <paquete>`) con confirmación de seguridad.
* **Monitor en segundo plano (Tray Icon):** Al cerrar la ventana, la aplicación permanece minimizada en la bandeja del sistema verificando periódicamente el `ChangeLog.txt` del mirror configurado y notificando mediante popups cuando existen nuevas actualizaciones.

### 2. Tienda y Gestión de SlackBuilds.org (`sbopkg`)
* **Catálogo categorizado y buscador en tiempo real:** Lee el índice `SLACKBUILDS.TXT` local para filtrar miles de recetas por categoría (`audio`, `office`, `multimedia`, `development`, `network`, etc.), por nombre o por descripción.
* **Detección de actualizaciones SBo:** Compara la versión instalada localmente con la disponible en el árbol de SlackBuilds y permite filtrar únicamente los paquetes desactualizados.
* **Resolución automática de dependencias transitivas:** Analiza recursivamente las claves `REQUIRES` de cada archivo `.info`, realiza un ordenamiento topológico de las dependencias faltantes, genera automáticamente el archivo de cola (`.sqf`) y ejecuta `sbopkg -B -i` en el orden exacto de compilación.

### 3. Soporte Nativo de Proxy y Seguridad
* **Detección automática de Proxy:** Lee `/etc/environment` (y las variables del entorno actual) para inyectar `http_proxy`, `https_proxy`, `ftp_proxy` y `no_proxy` tanto en las peticiones de red internas como a través de `pkexec` mediante `/usr/bin/env`.
* **Elevación de privilegios segura:** Utiliza `pkexec` (Polkit) únicamente al ejecutar operaciones administrativas (`update`, `upgrade-all`, `remove`, `sbopkg`), manteniendo la interfaz y el monitor en segundo plano corriendo como usuario sin privilegios.

---

## Tecnologías Utilizadas

* **Lenguaje:** C++17
* **Framework GUI y Red:** Qt 5 (`QtCore`, `QtGui`, `QtWidgets`, `QtNetwork`)
* **Sistema de Construcción:** `qmake` (`qmake-qt5`) y GNU `make`
* **Backends del Sistema:**
  * `/usr/sbin/slackpkg` (gestión de paquetes oficiales)
  * `/usr/sbin/sbopkg` (compilación e instalación de SlackBuilds)
  * `pkexec` / Polkit (autenticación gráfica)

---

## Requisitos Previos en Slackware

1. **Slackware 15.0** (o `-current`) con los paquetes de desarrollo base (`d/`), `qt5` (`l/qt5`) y `polkit` instalados.
2. Un mirror descomentado en `/etc/slackpkg/mirrors`.
3. *(Opcional)* Tener instalado **`sbopkg`** (`/usr/sbin/sbopkg`) para habilitar la pestaña de SlackBuilds.org.

---

## Compilación y Ejecución

Puedes compilar el proyecto desde **Qt Creator** abriendo directamente el archivo `qslackpkg.pro`, o utilizar el script automatizado incluido para compilar y ejecutar desde la terminal.

### Uso del script `build_and_run.sh`

1. Otorga permisos de ejecución al script (solo la primera vez):
   ```bash
   chmod +x build_and_run.sh
   ```

2. Ejecuta el script según lo que necesites:

   * **Compilar y ejecutar la aplicación directamente:**
     ```bash
     ./build_and_run.sh
     ```
   * **Solo compilar (sin abrir la interfaz gráfica):**
     ```bash
     ./build_and_run.sh --build-only
     ```
   * **Limpiar binarios/archivos temporales anteriores y recompilar desde cero:**
     ```bash
     ./build_and_run.sh --clean
     ```
   * **Ver todas las opciones disponibles:**
     ```bash
     ./build_and_run.sh --help
     ```

---

## Estructura del Proyecto

```text
qslackpkg/
├── qslackpkg.pro       # Archivo de configuración del proyecto para qmake / Qt Creator
├── main.cpp            # Punto de entrada de la aplicación Qt
├── mainwindow.h        # Declaración de la ventana principal, estructuras y slots
├── mainwindow.cpp      # Lógica de UI, slackpkg, sbopkg, proxy y resolutor de dependencias
├── build_and_run.sh    # Script automatizado de compilación y ejecución
└── README.md           # Documentación del proyecto
```

---

## Ejecución Automática al Iniciar Sesión (Opcional)

Si deseas que **QSlackpkg** inicie automáticamente en segundo plano al entrar a tu entorno de escritorio (XFCE, KDE Plasma, MATE, etc.), crea el siguiente archivo `~/.config/autostart/qslackpkg.desktop`:

```ini
[Desktop Entry]
Type=Application
Name=QSlackpkg
Comment=Monitor de actualizaciones y gestor de paquetes para Slackware
Exec=/ruta/completa/a/qslackpkg
Icon=system-software-update
Terminal=false
Categories=System;PackageManager;
```