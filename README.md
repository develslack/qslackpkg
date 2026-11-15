# QSlackpkg — Centro de Software, Repositorios y Gestor de Kernel para Slackware

**QSlackpkg** es una interfaz gráfica nativa desarrollada en **C++ y Qt5** diseñada para modernizar y simplificar la administración integral de paquetes, repositorios de terceros y configuración del arranque/kernel en **Slackware Linux** (15.0 / -current), manteniendo intacta la filosofía, solidez y herramientas oficiales de la distribución.

Está pensada tanto para veteranos de Slackware que desean un monitor silencioso en la bandeja del sistema con control total sobre cada comando ejecutado, como para usuarios provenientes de distribuciones como **Linux Mint** o **Ubuntu** que buscan instalar software, resolver dependencias o actualizar el kernel sin riesgo a dejar el sistema sin arranque.

---

## Características Principales

### 1. Gestión del Sistema Oficial (`slackpkg`) y Herramientas de Mantenimiento
* **Sincronización exacta:** Consulta directamente el motor de `slackpkg` para garantizar 100% de coincidencia entre la interfaz gráfica y la terminal.
* **Vista estructurada en 6 columnas con buscador y filtros en tiempo real:** Permite filtrar instantáneamente entre todos los paquetes instalados, actualizaciones pendientes, paquetes `_SBo`, paquetes externos (AlienBOB) o bloqueados (`blacklist`).
* **Inspector de Paquetes (`/var/log/packages`):** Hacer doble clic sobre cualquier paquete instalado despliega su descripción oficial (`slack-desc`), tamaño comprimido/instalado y la lista completa de archivos instalados en el sistema.
* **Gestor Visual de Archivos `.new` (`new-config`):** Escanea `/etc` en busca de archivos de configuración `*.new` generados tras una actualización, muestra las diferencias mediante `diff -u` y permite **Conservar el actual** (eliminar `.new`) o **Aplicar el nuevo** (creando un respaldo automático `.orig`).
* **Editor Rápido de Blacklist (`/etc/slackpkg/blacklist`):** Permite proteger paquetes críticos (`kernel-*`, `_SBo`, `alien`) con un clic y guardar cambios creando respaldo `.bak`.
* **Desinstalación directa y Monitor en Segundo Plano:** Incluye botón `Remove` por paquete y permanece minimizado en la bandeja del sistema verificando periódicamente `ChangeLog.txt` con contador numérico en vivo.

### 2. Tienda y Gestión de SlackBuilds.org (`sbopkg`)
* **Verificación e instalación automática de `sbopkg`:** Detecta si `sbopkg` está ausente y ofrece un asistente directo para descargarlo desde GitHub e instalarlo con un clic.
* **Catálogo categorizado y buscador en tiempo real:** Lee el índice `SLACKBUILDS.TXT` local para filtrar miles de recetas por categoría (`audio`, `office`, `multimedia`, `development`, `network`, etc.), nombre o descripción.
* **Detección de actualizaciones SBo:** Compara la versión instalada localmente con la disponible en el árbol de SlackBuilds y permite filtrar únicamente los paquetes desactualizados.
* **Resolución automática de dependencias transitivas:** Analiza recursivamente las claves `REQUIRES`, realiza un ordenamiento topológico de las dependencias faltantes, genera el archivo de cola (`.sqf`) y ejecuta `sbopkg -B -i` en el orden estricto de compilación.

### 3. Repositorio de Binarios Precompilados AlienBOB (`~alien/slackbuilds`)
* **Instalación rápida sin compilar:** Sincroniza el catálogo oficial `PACKAGES.TXT` de Eric Hameleers (AlienBOB) sin necesidad de permisos de administrador para explorar software listo para usar (`vlc`, `libreoffice`, `chromium`, `wine`, `obs-studio`, `calibre`, etc.).
* **Autodetección de rama y arquitectura:** Detecta automáticamente si el sistema corresponde a `pkg64/15.0`, `pkg64/current`, `pkg/15.0` o `pkg/current`, permitiendo también explorar otras ramas.
* **Verificación criptográfica y de integridad:** Cada descarga verifica automáticamente su suma `.md5` y su firma oficial `GPG-KEY` antes de ejecutar `/sbin/upgradepkg --install-new`.
* **Actualización en lote (`Actualizar todos`) y desinstalación:** Detecta paquetes AlienBOB instalados con versiones nuevas disponibles para actualizarlos en secuencia o desinstalarlos individualmente (`removepkg`).

### 4. Administrador de Kernel y Gestor de Arranque (`initrd` / `ELILO` / `LILO` / `GRUB`)
* **Diagnóstico completo del entorno de arranque:** Detecta automáticamente el kernel en ejecución (`uname -r`), los kernels instalados en `/boot`, las fuentes en `/usr/src/linux`, el modo de firmware (**UEFI / EFI** o **Legacy BIOS / MBR**), la partición raíz (`findmnt`), el bootloader activo (**ELILO**, **LILO** o **GRUB**) y sistemas operativos en **Dual-Boot**.
* **Alerta preventiva post-actualización:** Si `slackpkg` instala un kernel nuevo distinto al que está en ejecución y aún no tiene su `initrd` o entrada en el gestor de arranque, muestra una alerta visual inmediata para configurarlo antes de reiniciar.
* **Generación segura de `initrd` (`/sbin/mkinitrd`):** Autodetecta módulos necesarios (`mkinitrd_command_generator.sh`), dispositivo raíz y sistema de archivos, manteniendo estos parámetros **bloqueados por seguridad** para evitar errores críticos (con opción de desbloqueo explícito en *Modo Avanzado*).
* **Soporte de compilación `bzImage`:** Si `/usr/src/linux` está presente, permite compilar una imagen `bzImage` personalizada e instalar sus módulos.
* **Actualización automática del Bootloader con respaldo:**
  * **ELILO:** Copia `vmlinuz` e `initrd` a la partición EFI (`/boot/efi/EFI/Slackware`), respalda `elilo.conf` y actualiza la entrada principal.
  * **LILO:** Actualiza `/etc/lilo.conf` (con respaldo `.bak`) y ejecuta `/sbin/lilo -v`.
  * **GRUB:** Regenera `/boot/grub/grub.cfg` mediante `grub-mkconfig`.
  * Incluye opción para conservar el kernel anterior como entrada arrancable de emergencia (`Slackware-Prev`) y exige una ventana de confirmación explícita detallando cada comando antes de solicitar la contraseña de `root`.

### 5. Soporte Nativo de Proxy y Seguridad
* **Detección automática de Proxy:** Lee `/etc/environment` y variables del entorno actual para inyectar `http_proxy`, `https_proxy`, `ftp_proxy` y `no_proxy` tanto en las peticiones HTTP/HTTPS internas como a través de `pkexec` mediante `/usr/bin/env`.
* **Control estricto de privilegios (`UID 0` / `pkexec`):** Muestra un badge permanente indicando si la sesión corre en *Modo Informativo* o como *ROOT (UID 0)*, solicitando autenticación gráfica vía Polkit únicamente al ejecutar cambios en el sistema.

---

## Tecnologías Utilizadas

* **Lenguaje:** C++17
* **Framework GUI y Red:** Qt 5 (`QtCore`, `QtGui`, `QtWidgets`, `QtNetwork`)
* **Sistema de Construcción:** `qmake` (`qmake-qt5`) y GNU `make`
* **Backends del Sistema:**
  * `/usr/sbin/slackpkg` (paquetes oficiales)
  * `/usr/sbin/sbopkg` (SlackBuilds.org)
  * `/sbin/upgradepkg`, `/sbin/installpkg`, `/sbin/removepkg` (paquetes binarios AlienBOB)
  * `/sbin/mkinitrd`, `/sbin/lilo`, `/usr/sbin/grub-mkconfig` (gestión de kernel y arranque)
  * `pkexec` / Polkit (autenticación gráfica)

---

## Requisitos Previos en Slackware

1. **Slackware 15.0** (o `-current`) con los paquetes de desarrollo base (`d/`), `qt5` (`l/qt5`) y `polkit` instalados.
2. Un mirror descomentado en `/etc/slackpkg/mirrors`.

---

## Compilación, Instalación y Empaquetado Distribuible

Puedes abrir `qslackpkg.pro` directamente en **Qt Creator**, o utilizar el script `build_and_run.sh` para compilar, instalar o generar paquetes distribuibles listos para compartir.

### Uso del script `build_and_run.sh`

1. Otorga permisos de ejecución al script (solo la primera vez):
   ```bash
   chmod +x build_and_run.sh
   ```

2. Ejecuta el script según lo que necesites:

   * **Compilar y ejecutar la aplicación directamente en local:**
     ```bash
     ./build_and_run.sh
     ```
   * **Solo compilar (sin abrir la interfaz gráfica):**
     ```bash
     ./build_and_run.sh --build-only
     ```
   * **Limpiar binarios anteriores y recompilar desde cero:**
     ```bash
     ./build_and_run.sh --clean
     ```
   * **Instalar globalmente en el sistema** (instala en `/opt/qslackpkg/`, crea el enlace `/usr/bin/qslackpkg` y añade la entrada en **Aplicaciones -> Sistema**):
     ```bash
     ./build_and_run.sh --install
     ```
   * **Generar paquete nativo oficial de Slackware (`.txz` o `.tgz`)** en `./dist/` usando `makepkg` (incluye `slack-desc`, `doinst.sh` y suma `.md5`):
     ```bash
     ./build_and_run.sh --clean --package
     # O en formato clásico .tgz:
     ./build_and_run.sh --clean --tgz
     ```
   * **Generar instalador binario autocontenido (`.run`)** estilo `linuxdeployqt` / `makeself` en `./dist/` (un único archivo ejecutable que al abrirse solicita clave de `root` con `pkexec` e instala la aplicación automáticamente sin requerir herramientas de compilación):
     ```bash
     ./build_and_run.sh --clean --installer
     ```
   * **Definir número de versión personalizado y generar ambos formatos:**
     ```bash
     ./build_and_run.sh --clean --version 1.1.0 --package --installer
     ```
   * **Ver todas las opciones disponibles:**
     ```bash
     ./build_and_run.sh --help
     ```

---

## Estructura del Proyecto

```text
qslackpkg/
├── qslackpkg.pro           # Archivo de configuración para qmake / Qt Creator
├── main.cpp                # Punto de entrada de la aplicación Qt
├── mainwindow.h            # Declaración de la ventana principal, estructuras y módulos
├── mainwindow.cpp          # Lógica de UI, slackpkg, sbopkg, AlienBOB y Kernel/Bootloader
├── resources.qrc           # Sistema de recursos Qt (empaqueta el logotipo)
├── img/
│   └── Linux-Slackware-Logo-Vector.png # Logotipo oficial de Slackware
├── build_and_run.sh        # Script automatizado de compilación, instalación y empaquetado
├── dist/                   # Directorio de salida para paquetes .txz/.tgz e instaladores .run
└── README.md               # Documentación del proyecto
```

---

## Ejecución Automática al Iniciar Sesión (Opcional)

Si deseas que **QSlackpkg** inicie automáticamente en segundo plano al entrar a tu entorno de escritorio (XFCE, KDE Plasma, MATE, etc.), crea el siguiente archivo `~/.config/autostart/qslackpkg.desktop`:

```ini
[Desktop Entry]
Type=Application
Name=QSlackpkg
GenericName=Slackware Package & Kernel Manager
Comment=Package & Kernel Manager for Slackware Linux
Exec=/opt/qslackpkg/qslackpkg
Icon=/opt/qslackpkg/img/Linux-Slackware-Logo-Vector.png
Terminal=false
Categories=System;PackageManager;
```