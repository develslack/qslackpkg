#include "multilibwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSysInfo>

MultilibWidget::MultilibWidget(QNetworkAccessManager *netManager,
                               const QString &officialMirrorUrl,
                               QWidget *parent)
    : QWidget(parent),
      m_netManager(netManager),
      m_officialMirrorUrl(officialMirrorUrl)
{
    QString arch = QSysInfo::currentCpuArchitecture();
    m_is64Bit = (arch.contains("64") || arch == "x86_64");
    m_detectedBranch = detectSlackwareBranch();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Banner explicativo (KISS + Seguridad + Reversibilidad)
    QLabel *lblBanner = new QLabel(
        "<b>Soporte Multilib (Compatibilidad 32-bit en Slackware64):</b> "
        "Permite ejecutar software de 32 bits (como <b>Steam</b>, <b>Wine</b> o aplicaciones heredadas) "
        "mediante los paquetes oficiales de <b>AlienBOB</b>. Incluye protección automática en "
        "<code>/etc/slackpkg/blacklist</code> y la opción de <b>revertir completamente a Slackware64 puro</b> en cualquier momento.",
        this
    );
    lblBanner->setWordWrap(true);
    lblBanner->setStyleSheet(
        "background-color: #e3f2fd; color: #0d47a1; border: 1px solid #90caf9; "
        "border-left: 5px solid #1565c0; border-radius: 4px; padding: 8px; font-size: 9.2pt;"
    );
    mainLayout->addWidget(lblBanner);

    // ========================================================================
    // 1. ESTADO ACTUAL DEL SISTEMA MULTILIB
    // ========================================================================
    m_grpStatus = new QGroupBox("1. Diagnóstico Actual de Arquitectura y Librerías 32-bit", this);
    QGridLayout *statGrid = new QGridLayout(m_grpStatus);
    statGrid->setHorizontalSpacing(18);
    statGrid->setVerticalSpacing(6);

    m_lblArchInfo = new QLabel("Detectando...", m_grpStatus);
    m_lblCoreStatus = new QLabel("Detectando...", m_grpStatus);
    m_lblCompat32Count = new QLabel("Detectando...", m_grpStatus);
    m_lblBlacklistStatus = new QLabel("Detectando...", m_grpStatus);

    statGrid->addWidget(new QLabel("<b>Arquitectura / Rama:</b>", m_grpStatus), 0, 0);
    statGrid->addWidget(m_lblArchInfo, 0, 1);

    statGrid->addWidget(new QLabel("<b>Núcleo Multilib (gcc / glibc):</b>", m_grpStatus), 1, 0);
    statGrid->addWidget(m_lblCoreStatus, 1, 1);

    statGrid->addWidget(new QLabel("<b>Librerías compat32 instaladas:</b>", m_grpStatus), 0, 2);
    statGrid->addWidget(m_lblCompat32Count, 0, 3);

    statGrid->addWidget(new QLabel("<b>Protección en Blacklist:</b>", m_grpStatus), 1, 2);
    statGrid->addWidget(m_lblBlacklistStatus, 1, 3);

    statGrid->setColumnStretch(1, 1);
    statGrid->setColumnStretch(3, 1);

    m_btnRescan = new QPushButton("Re-escanear estado", m_grpStatus);
    m_btnRescan->setCursor(Qt::PointingHandCursor);
    statGrid->addWidget(m_btnRescan, 2, 3, Qt::AlignRight);

    mainLayout->addWidget(m_grpStatus);

    // ========================================================================
    // 2. ACCIONES: INSTALAR/ACTUALIZAR O VOLVER ATRÁS A 64-BIT PURO
    // ========================================================================
    m_grpActions = new QGroupBox("2. Gestión Segura de Multilib (Requiere contraseña de root)", this);
    QVBoxLayout *actLayout = new QVBoxLayout(m_grpActions);

    m_chkAutoBlacklist = new QCheckBox(
        "Proteger automáticamente [0-9]+compat32 y [0-9]+alien en /etc/slackpkg/blacklist (Evita conflictos con slackpkg)",
        m_grpActions
    );
    m_chkAutoBlacklist->setChecked(true);
    m_chkAutoBlacklist->setStyleSheet("font-weight: bold; color: #1b5e20;");
    actLayout->addWidget(m_chkAutoBlacklist);

    QHBoxLayout *btnRow = new QHBoxLayout();
    m_btnInstallMultilib = new QPushButton("Instalar / Actualizar Multilib Completo (AlienBOB)", m_grpActions);
    m_btnInstallMultilib->setCursor(Qt::PointingHandCursor);
    m_btnInstallMultilib->setStyleSheet(
        "QPushButton { background-color: #1565c0; color: white; font-weight: bold; "
        "padding: 8px 16px; border-radius: 4px; }"
        "QPushButton:hover:enabled { background-color: #0d47a1; }"
    );

    m_btnRevertPure64 = new QPushButton("Desinstalar Multilib y Restaurar Slackware64 Puro (Volver atrás)", m_grpActions);
    m_btnRevertPure64->setCursor(Qt::PointingHandCursor);
    m_btnRevertPure64->setStyleSheet(
        "QPushButton { background-color: #ffebee; color: #c62828; border: 1px solid #ef9a9a; "
        "font-weight: bold; padding: 8px 16px; border-radius: 4px; }"
        "QPushButton:hover:enabled { background-color: #c62828; color: white; }"
        "QPushButton:disabled { background-color: #f5f5f5; color: #bdbdbd; border-color: #e0e0e0; }"
    );

    btnRow->addWidget(m_btnInstallMultilib);
    btnRow->addSpacing(12);
    btnRow->addWidget(m_btnRevertPure64);
    btnRow->addStretch();
    actLayout->addLayout(btnRow);

    mainLayout->addWidget(m_grpActions);

    // ========================================================================
    // 3. LISTADO DE PAQUETES MULTILIB / COMPAT32 INSTALADOS EN EL SISTEMA
    // ========================================================================
    m_pkgTable = new QTableWidget(0, 3, this);
    m_pkgTable->setHorizontalHeaderLabels({
        "Paquete Instalado",
        "Tipo de Componente",
        "Estado de Protección"
    });
    m_pkgTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_pkgTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_pkgTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_pkgTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pkgTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pkgTable->setAlternatingRowColors(true);
    mainLayout->addWidget(m_pkgTable, 1);

    // ========================================================================
    // 4. PROGRESO Y CONSOLA DE OPERACIONES
    // ========================================================================
    QHBoxLayout *progRow = new QHBoxLayout();
    m_lblProgressStatus = new QLabel("Listo.", this);
    m_lblProgressStatus->setStyleSheet("font-weight: bold; color: #37474f;");
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setMinimumWidth(240);
    progRow->addWidget(m_lblProgressStatus, 1);
    progRow->addWidget(m_progressBar);
    mainLayout->addLayout(progRow);

    m_consoleLog = new QTextEdit(this);
    m_consoleLog->setReadOnly(true);
    m_consoleLog->setMaximumHeight(130);
    m_consoleLog->setStyleSheet(
        "background-color: #1e1e1e; color: #d4d4d4; font-family: monospace; font-size: 8.8pt; border-radius: 4px;"
    );
    m_consoleLog->setPlaceholderText("Registro de descarga, verificación MD5 e instalación/reversión de Multilib...");
    mainLayout->addWidget(m_consoleLog);

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_btnRescan, &QPushButton::clicked, this, &MultilibWidget::scanMultilibStatus);
    connect(m_btnInstallMultilib, &QPushButton::clicked, this, &MultilibWidget::runInstallOrUpdateMultilib);
    connect(m_btnRevertPure64, &QPushButton::clicked, this, &MultilibWidget::runRevertToPure64);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MultilibWidget::readProcessOutput);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MultilibWidget::onProcessFinished);

    scanMultilibStatus();
}

MultilibWidget::~MultilibWidget() {
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished(3000);
    }
}

bool MultilibWidget::isBusy() const {
    return m_process->state() != QProcess::NotRunning;
}

QString MultilibWidget::detectSlackwareBranch() const {
    QFile verFile("/etc/slackware-version");
    if (verFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(verFile.readAll()).trimmed();
        verFile.close();
        if (content.contains('+') || content.contains("current", Qt::CaseInsensitive)) {
            return "current";
        }
        QRegularExpression rx("(\\d+\\.\\d+)");
        QRegularExpressionMatch m = rx.match(content);
        if (m.hasMatch()) {
            return m.captured(1);
        }
    }
    return "15.0";
}

bool MultilibWidget::isRuleInBlacklist(const QString &rule) const {
    QFile file("/etc/slackpkg/blacklist");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QTextStream in(&file);
    while (!in.atEnd()) {
        if (in.readLine().trimmed() == rule) return true;
    }
    return false;
}

void MultilibWidget::scanMultilibStatus() {
    m_installedCoreMultilibList.clear();
    m_installedCompat32List.clear();
    m_pkgTable->setRowCount(0);

    if (!m_is64Bit) {
        m_lblArchInfo->setStyleSheet("color: #1565c0; font-weight: bold;");
        m_lblArchInfo->setText("32-bit nativo (x86) — No requiere capa Multilib");
        m_lblCoreStatus->setText("Nativo 32-bit");
        m_lblCompat32Count->setText("No aplica");
        m_lblBlacklistStatus->setText("No aplica");
        m_grpActions->setEnabled(false);
        return;
    }

    m_lblArchInfo->setStyleSheet("color: #0d47a1; font-weight: bold;");
    m_lblArchInfo->setText(QString("Slackware64 (%1) [x86_64]").arg(m_detectedBranch));

    QDir pkgDir("/var/log/packages");
    QStringList allPkgs = pkgDir.entryList(QDir::Files, QDir::Name);

    for (const QString &pkg : allPkgs) {
        if (pkg.contains("multilib", Qt::CaseInsensitive)) {
            m_installedCoreMultilibList.append(pkg);
        } else if (pkg.contains("-compat32-", Qt::CaseInsensitive) || pkg.endsWith("compat32", Qt::CaseInsensitive)) {
            m_installedCompat32List.append(pkg);
        }
    }

    bool hasCore = !m_installedCoreMultilibList.isEmpty();
    bool hasCompat = !m_installedCompat32List.isEmpty();
    bool isMultilibActive = hasCore || hasCompat;

    if (hasCore) {
        m_lblCoreStatus->setStyleSheet("color: #1b5e20; font-weight: bold;");
        m_lblCoreStatus->setText(QString("Activo (%1 paquete(s) gcc/glibc multilib)").arg(m_installedCoreMultilibList.size()));
    } else {
        m_lblCoreStatus->setStyleSheet("color: #546e7a; font-weight: bold;");
        m_lblCoreStatus->setText("Slackware64 puro (sin multilib)");
    }

    m_lblCompat32Count->setStyleSheet(hasCompat ? "color: #1b5e20; font-weight: bold;" : "color: #546e7a;");
    m_lblCompat32Count->setText(QString("%1 paquete(s) compat32").arg(m_installedCompat32List.size()));

    bool blCompat = isRuleInBlacklist("[0-9]+compat32");
    bool blAlien = isRuleInBlacklist("[0-9]+alien");
    if (blCompat && blAlien) {
        m_lblBlacklistStatus->setStyleSheet("color: #1b5e20; font-weight: bold;");
        m_lblBlacklistStatus->setText("Protegido ([0-9]+compat32 y [0-9]+alien)");
    } else if (isMultilibActive) {
        m_lblBlacklistStatus->setStyleSheet("color: #e65100; font-weight: bold;");
        m_lblBlacklistStatus->setText("¡Atención! Falta regla en /etc/slackpkg/blacklist");
    } else {
        m_lblBlacklistStatus->setStyleSheet("color: #546e7a;");
        m_lblBlacklistStatus->setText("No requerida (sistema 64-bit puro)");
    }

    m_btnRevertPure64->setEnabled(isMultilibActive);

    // Poblar tabla
    for (const QString &pkg : m_installedCoreMultilibList) {
        int r = m_pkgTable->rowCount();
        m_pkgTable->insertRow(r);
        QTableWidgetItem *pItem = new QTableWidgetItem(pkg);
        QFont f = pItem->font(); f.setBold(true); pItem->setFont(f);
        m_pkgTable->setItem(r, 0, pItem);
        m_pkgTable->setItem(r, 1, new QTableWidgetItem("Núcleo Multilib (Toolchain / glibc)"));
        m_pkgTable->setItem(r, 2, new QTableWidgetItem(blAlien ? "Protegido (blacklist)" : "Sin proteger"));
    }
    for (const QString &pkg : m_installedCompat32List) {
        int r = m_pkgTable->rowCount();
        m_pkgTable->insertRow(r);
        m_pkgTable->setItem(r, 0, new QTableWidgetItem(pkg));
        m_pkgTable->setItem(r, 1, new QTableWidgetItem("Librería 32-bit (compat32)"));
        m_pkgTable->setItem(r, 2, new QTableWidgetItem(blCompat ? "Protegido (blacklist)" : "Sin proteger"));
    }
}

void MultilibWidget::setControlsEnabled(bool enabled) {
    m_grpStatus->setEnabled(enabled);
    m_grpActions->setEnabled(enabled && m_is64Bit);
    if (enabled && m_is64Bit) {
        m_btnRevertPure64->setEnabled(!m_installedCoreMultilibList.isEmpty() || !m_installedCompat32List.isEmpty());
    }
}

void MultilibWidget::runInstallOrUpdateMultilib() {
    if (isBusy() || !m_is64Bit) return;

    QString baseUrl = QString("http://www.slackware.com/~alien/multilib/%1/").arg(m_detectedBranch);

    QMessageBox::StandardButton confirm = QMessageBox::question(
        this,
        "Instalar / Actualizar Multilib Completo (Requiere root)",
        QString("Se descargará e instalará la capa Multilib oficial de Eric Hameleers (AlienBOB) para <b>Slackware64 %1</b>:\n\n"
                "• Origen: %2\n"
                "• Paso 1: Verificación de integridad MD5 oficial.\n"
                "• Paso 2: Protección en /etc/slackpkg/blacklist (con respaldo .bak).\n"
                "• Paso 3: Actualización de glibc/gcc multilib e instalación de librerías compat32.\n\n"
                "Esta operación es 100% reversible desde el botón 'Restaurar Slackware64 Puro'.\n"
                "¿Desea continuar e ingresar la contraseña de root?")
            .arg(m_detectedBranch, baseUrl),
        QMessageBox::Yes | QMessageBox::No
    );
    if (confirm != QMessageBox::Yes) return;

    m_currentTask = TaskInstallUpdate;
    m_consoleLog->clear();
    m_progressBar->setRange(0, 0);
    setControlsEnabled(false);
    m_lblProgressStatus->setText("Descargando y verificando paquetes Multilib de AlienBOB...");

    QString blacklistBlock;
    if (m_chkAutoBlacklist->isChecked()) {
        blacklistBlock =
            "echo '>>> Protegiendo reglas multilib en /etc/slackpkg/blacklist...'; "
            "cp -p /etc/slackpkg/blacklist /etc/slackpkg/blacklist.bak.$(date +%Y%m%d%H%M%S); "
            "grep -qxF '[0-9]+compat32' /etc/slackpkg/blacklist || echo '[0-9]+compat32' >> /etc/slackpkg/blacklist; "
            "grep -qxF '[0-9]+alien' /etc/slackpkg/blacklist || echo '[0-9]+alien' >> /etc/slackpkg/blacklist; ";
    }

    QString script = QString(
        "set -e; "
        "TMPD=$(mktemp -d /tmp/qslackpkg_multilib.XXXXXX); "
        "trap 'rm -rf \"$TMPD\"' EXIT; "
        "cd \"$TMPD\"; "
        "%1"
        "echo '>>> Descargando paquetes Multilib desde %2 ...'; "
        "wget -r -np -nH --cut-dirs=3 -R 'index.html*' -e robots=off '%2'; "
        "echo '>>> Verificando integridad MD5 oficial...'; "
        "if [ -f CHECKSUMS.md5 ]; then "
        "  grep -E '\\.t[gx]z$' CHECKSUMS.md5 | md5sum -c -; "
        "fi; "
        "echo '>>> Instalando paquetes núcleo Multilib (gcc / glibc)...'; "
        "/sbin/upgradepkg --reinstall --install-new *.t?z; "
        "if [ -d slackware64-compat32 ]; then "
        "  echo '>>> Instalando librerías de compatibilidad 32-bit (compat32)...'; "
        "  /sbin/upgradepkg --install-new slackware64-compat32/*-compat32/*.t?z; "
        "fi; "
        "echo '>>> MULTILIB INSTALADO Y VERIFICADO EXITOSAMENTE'"
    ).arg(blacklistBlock, baseUrl);

    m_process->start("pkexec", {"--user", "root", "/bin/sh", "-c", script});
}

void MultilibWidget::runRevertToPure64() {
    if (isBusy() || !m_is64Bit) return;

    if (m_officialMirrorUrl.isEmpty()) {
        QMessageBox::warning(
            this,
            "Mirror Oficial No Configurado",
            "Para restaurar los paquetes originales gcc y glibc de 64 bits de forma segura, "
            "necesita tener un mirror activo descomentado en /etc/slackpkg/mirrors."
        );
        return;
    }

    QMessageBox::StandardButton confirm = QMessageBox::warning(
        this,
        "⚠️ Restaurar Slackware64 Puro (Eliminar Multilib)",
        "Esta operación devolverá su sistema a un entorno Slackware64 100% puro y limpio:\n\n"
        "1. Descargará y reinstalará primero los paquetes originales gcc y glibc de 64 bits desde su mirror oficial "
        "(garantizando que el sistema nunca quede sin librerías C/C++).\n"
        "2. Desinstalará con removepkg todos los paquetes *-compat32.\n"
        "3. Limpiará la regla [0-9]+compat32 de /etc/slackpkg/blacklist.\n\n"
        "¿Desea continuar e ingresar la contraseña de root?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    if (confirm != QMessageBox::Yes) return;

    m_currentTask = TaskRevertPure64;
    m_consoleLog->clear();
    m_progressBar->setRange(0, 0);
    setControlsEnabled(false);
    m_lblProgressStatus->setText("Restaurando paquetes originales de 64 bits y limpiando compat32...");

    QString mirror = m_officialMirrorUrl;
    if (!mirror.endsWith('/')) mirror += '/';

    QString script = QString(
        "set -e; "
        "TMPD=$(mktemp -d /tmp/qslackpkg_pure64.XXXXXX); "
        "trap 'rm -rf \"$TMPD\"' EXIT; "
        "cd \"$TMPD\"; "
        "echo '>>> [1/3] Obteniendo paquetes gcc y glibc oficiales de Slackware64...'; "
        "for SERIES in d l; do "
        "  HTML=$(wget -q -O - '%1slackware64/'$SERIES'/'); "
        "  for PKG in $(echo \"$HTML\" | grep -oE '(gcc[^\"<>]*|glibc[^\"<>]*)\\.t[gx]z' | sort -u); do "
        "    echo \"Descargando $PKG...\"; "
        "    wget -q -O \"$PKG\" '%1slackware64/'$SERIES'/'\"$PKG\"; "
        "  done; "
        "done; "
        "echo '>>> Reemplazando toolchain multilib por Slackware64 oficial puro...'; "
        "/sbin/upgradepkg --reinstall *.t?z; "
        "echo '>>> [2/3] Eliminando paquetes compat32...'; "
        "for C32 in /var/log/packages/*-compat32-*; do "
        "  [ -e \"$C32\" ] && /sbin/removepkg \"$(basename \"$C32\")\"; "
        "done; "
        "echo '>>> [3/3] Limpiando regla compat32 en /etc/slackpkg/blacklist...'; "
        "cp -p /etc/slackpkg/blacklist /etc/slackpkg/blacklist.bak.$(date +%Y%m%d%H%M%S); "
        "sed -i '/^\\[0-9\\]+compat32$/d' /etc/slackpkg/blacklist; "
        "echo '>>> SISTEMA RESTAURADO A SLACKWARE64 PURO EXITOSAMENTE'"
    ).arg(mirror);

    m_process->start("pkexec", {"--user", "root", "/bin/sh", "-c", script});
}

void MultilibWidget::readProcessOutput() {
    QString text = QString::fromUtf8(m_process->readAllStandardOutput());
    m_consoleLog->moveCursor(QTextCursor::End);
    m_consoleLog->insertPlainText(text);
    m_consoleLog->verticalScrollBar()->setValue(m_consoleLog->verticalScrollBar()->maximum());
}

void MultilibWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    MultilibTask finishedTask = m_currentTask;
    m_currentTask = TaskNone;

    m_progressBar->setRange(0, 100);
    setControlsEnabled(true);
    m_consoleLog->append(QString("\n>>> Proceso finalizado (Código: %1)").arg(exitCode));

    if (exitCode == 126 || exitCode == 127) {
        m_progressBar->setValue(0);
        m_lblProgressStatus->setText("Operación cancelada: no se ingresó contraseña de root.");
        return;
    }

    scanMultilibStatus();
    emit multilibStateChanged();

    if (exitCode != 0) {
        m_progressBar->setValue(0);
        m_lblProgressStatus->setText(QString("Error durante la operación (Código %1).").arg(exitCode));
        QMessageBox::critical(this, "Error en Multilib",
                              "La operación finalizó con errores. Revise el registro inferior.");
        return;
    }

    m_progressBar->setValue(100);
    if (finishedTask == TaskInstallUpdate) {
        m_lblProgressStatus->setText("Capa Multilib instalada y protegida en blacklist.");
        QMessageBox::information(this, "Multilib Listo",
                                 "La capa Multilib de 32 bits se ha instalado y protegido correctamente.");
    } else if (finishedTask == TaskRevertPure64) {
        m_lblProgressStatus->setText("Sistema restaurado a Slackware64 puro.");
        QMessageBox::information(this, "Restauración Completada",
                                 "Se eliminaron los paquetes compat32 y se restauraron gcc/glibc oficiales de 64 bits.");
    }
}
