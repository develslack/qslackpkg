#include "kernelconfigdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QRegularExpression>
#include <QScrollBar>
#include <QCloseEvent>
#include <QThread>
#include <QApplication>
#include <unistd.h>
#include <QStandardItemModel>

KernelConfigDialog::KernelConfigDialog(QNetworkAccessManager *netManager,
                                       const QString &rootDevice,
                                       const QString &rootFs,
                                       const QString &bootloaderType,
                                       const QString &bootloaderConfigPath,
                                       const QString &efiSlackwareDir,
                                       bool keepFallbackEntry,
                                       QWidget *parent)
    : QDialog(parent),
      m_netManager(netManager),
      m_rootDevice(rootDevice),
      m_rootFs(rootFs),
      m_bootloaderType(bootloaderType),
      m_bootloaderConfigPath(bootloaderConfigPath),
      m_efiSlackwareDir(efiSlackwareDir),
      m_keepFallbackEntry(keepFallbackEntry)
{
    setWindowTitle("Asistente de Descarga, Configuración y Compilación de Kernel - QSlackpkg");
    resize(1260, 850);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // ========================================================================
    // CABECERA SUPERIOR + BOTÓN DE CONFIGURACIÓN AVANZADA DE NUEVO KERNEL
    // ========================================================================
    QHBoxLayout *topHeaderRow = new QHBoxLayout();

    m_lblTopBanner = new QLabel(this);
    m_lblTopBanner->setWordWrap(true);

    m_btnToggleAdvancedMode = new QPushButton("Configuración Avanzada de Nuevo Kernel", this);
    m_btnToggleAdvancedMode->setCursor(Qt::PointingHandCursor);
    m_btnToggleAdvancedMode->setMinimumHeight(42);
    m_btnToggleAdvancedMode->setToolTip(
        "Por defecto se heredan y protegen los módulos del kernel actual.\n"
        "Pulse aquí solo si es un usuario avanzado y desea modificar manualmente los módulos y opciones Kconfig."
    );

    topHeaderRow->addWidget(m_lblTopBanner, 1);
    topHeaderRow->addWidget(m_btnToggleAdvancedMode);
    mainLayout->addLayout(topHeaderRow);

    // ========================================================================
    // PASO 1: FUENTES DEL KERNEL (LOCAL /USR/SRC O DESCARGA DESDE KERNEL.ORG)
    // ========================================================================
    m_grpStep1 = new QGroupBox("Paso 1: Selección o Descarga de Código Fuente del Kernel", this);
    QVBoxLayout *step1Layout = new QVBoxLayout(m_grpStep1);

    QHBoxLayout *srcRow = new QHBoxLayout();
    m_comboSourceSelect = new QComboBox(m_grpStep1);
    m_comboSourceSelect->setMinimumWidth(420);

    m_btnFetchKernelOrg = new QPushButton("Consultar versiones en Kernel.org", m_grpStep1);
    m_btnFetchKernelOrg->setCursor(Qt::PointingHandCursor);

    m_btnDownloadSource = new QPushButton("Descargar y Extraer en /usr/src (root)", m_grpStep1);
    m_btnDownloadSource->setCursor(Qt::PointingHandCursor);
    m_btnDownloadSource->setEnabled(false);
    m_btnDownloadSource->setStyleSheet(
        "QPushButton { background-color: #1565c0; color: white; font-weight: bold; padding: 5px 12px; border-radius: 4px; }"
        "QPushButton:hover:enabled { background-color: #0d47a1; }"
        "QPushButton:disabled { background-color: #eceff1; color: #90a4ae; }"
    );

    srcRow->addWidget(new QLabel("<b>Árbol / Versión:</b>", m_grpStep1));
    srcRow->addWidget(m_comboSourceSelect, 1);
    srcRow->addWidget(m_btnFetchKernelOrg);
    srcRow->addWidget(m_btnDownloadSource);
    step1Layout->addLayout(srcRow);

    m_lblSourceStatus = new QLabel("Escaneando fuentes locales en /usr/src...", m_grpStep1);
    m_lblSourceStatus->setStyleSheet("color: #37474f; font-size: 9pt;");
    step1Layout->addWidget(m_lblSourceStatus);
    mainLayout->addWidget(m_grpStep1);

    // ========================================================================
    // PASO 2: HERENCIA INTELIGENTE DE CONFIGURACIÓN ACTUAL
    // ========================================================================
    m_grpStep2 = new QGroupBox("Paso 2: Base de Módulos (Heredar Configuración Funcional del Sistema Actual)", this);
    QHBoxLayout *step2Layout = new QHBoxLayout(m_grpStep2);

    m_btnInheritFullConfig = new QPushButton("1. Heredar todos los módulos del kernel actual (Recomendado / Seguro)", m_grpStep2);
    m_btnInheritFullConfig->setCursor(Qt::PointingHandCursor);
    m_btnInheritFullConfig->setToolTip(
        "Toma la configuración exacta del kernel con el que arrancó el sistema (/proc/config.gz) "
        "y adapta las opciones nuevas mediante 'make olddefconfig'."
    );
    m_btnInheritFullConfig->setStyleSheet(
        "QPushButton { background-color: #e8f5e9; color: #1b5e20; border: 1px solid #81c784; "
        "font-weight: bold; padding: 6px 12px; border-radius: 4px; }"
        "QPushButton:hover:enabled { background-color: #c8e6c9; }"
    );

    m_btnInheritLocalModConfig = new QPushButton("2. Recortar módulos no cargados (localmodconfig - Avanzado)", m_grpStep2);
    m_btnInheritLocalModConfig->setCursor(Qt::PointingHandCursor);
    m_btnInheritLocalModConfig->setToolTip(
        "Disponible solo en Modo Avanzado: desactiva los drivers que no estén cargados actualmente en lsmod."
    );
    m_btnInheritLocalModConfig->setStyleSheet(
        "QPushButton { background-color: #ffebee; color: #b71c1c; border: 1px solid #ef9a9a; "
        "font-weight: bold; padding: 6px 12px; border-radius: 4px; }"
        "QPushButton:hover:enabled { background-color: #ffcdd2; }"
        "QPushButton:disabled { background-color: #eceff1; color: #90a4ae; border-color: #cfd8dc; }"
    );

    m_lblConfigStatus = new QLabel("Usando módulos heredados del kernel en ejecución.", m_grpStep2);
    m_lblConfigStatus->setStyleSheet("color: #1b5e20; font-weight: bold; padding-left: 8px;");

    step2Layout->addWidget(m_btnInheritFullConfig);
    step2Layout->addWidget(m_btnInheritLocalModConfig);
    step2Layout->addWidget(m_lblConfigStatus, 1);
    mainLayout->addWidget(m_grpStep2);

    // ========================================================================
    // PASO 3: CONFIGURADOR VISUAL DE KERNEL (SOLO LECTURA EN MODO SEGURO)
    // ========================================================================
    m_grpStep3 = new QGroupBox("Paso 3: Inspección y Personalización de Módulos del Kernel (Kconfig)", this);
    QVBoxLayout *step3Layout = new QVBoxLayout(m_grpStep3);

    QHBoxLayout *filterBar = new QHBoxLayout();
    m_editKconfigSearch = new QLineEdit(m_grpStep3);
    m_editKconfigSearch->setPlaceholderText("Buscar driver, sistema de archivos o símbolo CONFIG_ (ej: nvme, amdgpu, btrfs, wireguard, preempt)...");
    m_editKconfigSearch->setClearButtonEnabled(true);

    m_chkShowEnabledOnly = new QCheckBox("Solo opciones activas ([*] o [M])", m_grpStep3);
    m_lblOptionsCounter = new QLabel("0 opciones cargadas", m_grpStep3);
    m_lblOptionsCounter->setStyleSheet("color: #37474f; font-weight: bold;");

    filterBar->addWidget(new QLabel("<b>Buscar:</b>", m_grpStep3));
    filterBar->addWidget(m_editKconfigSearch, 1);
    filterBar->addWidget(m_chkShowEnabledOnly);
    filterBar->addSpacing(8);
    filterBar->addWidget(m_lblOptionsCounter);
    step3Layout->addLayout(filterBar);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, m_grpStep3);

    m_kconfigTree = new QTreeWidget(splitter);
    m_kconfigTree->setColumnCount(3);
    m_kconfigTree->setHeaderLabels({"Subsistema / Opción del Kernel", "Estado", "Símbolo CONFIG_"});
    m_kconfigTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_kconfigTree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_kconfigTree->header()->setSectionResizeMode(2, QHeaderView::Interactive);
    m_kconfigTree->setColumnWidth(1, 115);
    m_kconfigTree->setColumnWidth(2, 220);
    m_kconfigTree->setAlternatingRowColors(true);

    QWidget *rightPane = new QWidget(splitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(8, 0, 0, 0);

    m_lblOptTitle = new QLabel("Seleccione una opción en el árbol de la izquierda", rightPane);
    m_lblOptTitle->setWordWrap(true);
    m_lblOptTitle->setStyleSheet("font-size: 10.5pt; font-weight: bold; color: #1a237e;");

    m_lblOptSymbol = new QLabel("Símbolo: —", rightPane);
    m_lblOptSymbol->setStyleSheet("font-family: monospace; color: #37474f; font-size: 9pt;");

    m_lblOptDepends = new QLabel("Dependencias: —", rightPane);
    m_lblOptDepends->setWordWrap(true);
    m_lblOptDepends->setStyleSheet("color: #616161; font-size: 8.8pt;");

    QHBoxLayout *stateEditRow = new QHBoxLayout();
    stateEditRow->addWidget(new QLabel("<b>Estado del módulo:</b>", rightPane));

    m_comboOptTriState = new QComboBox(rightPane);
    m_comboOptTriState->addItem("[   ] Deshabilitado (is not set)", "n");
    m_comboOptTriState->addItem("[ * ] Integrado en el Kernel (=y)", "y");
    m_comboOptTriState->addItem("[ M ] Compilar como Módulo (=m)", "m");
    m_comboOptTriState->setEnabled(false);

    m_editOptValue = new QLineEdit(rightPane);
    m_editOptValue->setVisible(false);
    m_editOptValue->setEnabled(false);

    stateEditRow->addWidget(m_comboOptTriState, 1);
    stateEditRow->addWidget(m_editOptValue, 1);

    m_txtOptHelp = new QTextEdit(rightPane);
    m_txtOptHelp->setReadOnly(true);
    m_txtOptHelp->setStyleSheet(
        "background-color: #fafafa; color: #212121; font-size: 9.2pt; border: 1px solid #cfd8dc; border-radius: 4px;"
    );

    rightLayout->addWidget(m_lblOptTitle);
    rightLayout->addWidget(m_lblOptSymbol);
    rightLayout->addWidget(m_lblOptDepends);
    rightLayout->addLayout(stateEditRow);
    rightLayout->addWidget(new QLabel("<b>Ayuda y descripción técnica (Kconfig help):</b>", rightPane));
    rightLayout->addWidget(m_txtOptHelp, 1);

    splitter->addWidget(m_kconfigTree);
    splitter->addWidget(rightPane);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    step3Layout->addWidget(splitter, 1);

    mainLayout->addWidget(m_grpStep3, 1);

    // ========================================================================
    // PASO 4: COMPILACIÓN DE BZIMAGE, INITRD E INSTALACIÓN EN BOOTLOADER
    // ========================================================================
    m_grpStep4 = new QGroupBox("Paso 4: Compilación de bzImage, Generación de initrd e Instalación en Bootloader", this);
    QHBoxLayout *step4Layout = new QHBoxLayout(m_grpStep4);

    int maxCores = QThread::idealThreadCount();
    if (maxCores < 1) maxCores = 2;

    m_spinJobs = new QSpinBox(m_grpStep4);
    m_spinJobs->setRange(1, maxCores * 2);
    m_spinJobs->setValue(maxCores);
    m_spinJobs->setToolTip("Cantidad de hilos paralelos para make -jN");

    m_chkBuildInitrdAfter = new QCheckBox("Generar /boot/initrd-<ver>.gz automáticamente", m_grpStep4);
    m_chkBuildInitrdAfter->setChecked(true);

    m_chkUpdateBootAfter = new QCheckBox(
        QString("Actualizar %1 (conservando entradas previas)").arg(m_bootloaderType),
        m_grpStep4
    );
    m_chkUpdateBootAfter->setChecked(m_bootloaderType != "No detectado");

    m_btnSaveConfigOnly = new QPushButton("Guardar .config (root)", m_grpStep4);
    m_btnSaveConfigOnly->setCursor(Qt::PointingHandCursor);
    m_btnSaveConfigOnly->setVisible(false);

    m_btnCompileAndInstall = new QPushButton("Compilar e Instalar Nuevo Kernel (root)", m_grpStep4);
    m_btnCompileAndInstall->setCursor(Qt::PointingHandCursor);
    m_btnCompileAndInstall->setStyleSheet(
        "QPushButton { background-color: #2e7d32; color: white; font-weight: bold; "
        "padding: 7px 16px; border-radius: 4px; font-size: 9.8pt; }"
        "QPushButton:hover:enabled { background-color: #1b5e20; }"
        "QPushButton:disabled { background-color: #a5d6a7; color: #f1f8e9; }"
    );

    m_btnCloseDialog = new QPushButton("Cerrar", m_grpStep4);

    step4Layout->addWidget(new QLabel("<b>Hilos CPU (-j):</b>", m_grpStep4));
    step4Layout->addWidget(m_spinJobs);
    step4Layout->addSpacing(8);
    step4Layout->addWidget(m_chkBuildInitrdAfter);
    step4Layout->addWidget(m_chkUpdateBootAfter);
    step4Layout->addStretch();
    step4Layout->addWidget(m_btnSaveConfigOnly);
    step4Layout->addWidget(m_btnCompileAndInstall);
    step4Layout->addWidget(m_btnCloseDialog);

    mainLayout->addWidget(m_grpStep4);

    // ========================================================================
    // CONSOLA INTERNA Y PROGRESO PERSISTENTE (NUNCA SE CIERRA AL EJECUTAR)
    // ========================================================================
    QHBoxLayout *progHeaderRow = new QHBoxLayout();
    m_lblTaskProgressStatus = new QLabel("Estado: Listo.", this);
    m_lblTaskProgressStatus->setStyleSheet("font-weight: bold; color: #37474f; font-size: 9pt;");

    m_internalProgress = new QProgressBar(this);
    m_internalProgress->setRange(0, 0);
    m_internalProgress->setMaximumWidth(280);
    m_internalProgress->setVisible(false);

    progHeaderRow->addWidget(m_lblTaskProgressStatus, 1);
    progHeaderRow->addWidget(m_internalProgress);
    mainLayout->addLayout(progHeaderRow);

    m_internalConsole = new QTextEdit(this);
    m_internalConsole->setReadOnly(true);
    m_internalConsole->setMaximumHeight(145);
    m_internalConsole->setStyleSheet(
        "background-color: #1e1e1e; color: #d4d4d4; font-family: monospace; font-size: 8.8pt; border-radius: 4px;"
    );
    m_internalConsole->setPlaceholderText("Aquí se mostrará en vivo el progreso de descarga, configuración y compilación del kernel...");
    mainLayout->addWidget(m_internalConsole);

    m_internalProcess = new QProcess(this);
    m_internalProcess->setProcessChannelMode(QProcess::MergedChannels);

    // Conexiones
    connect(m_btnToggleAdvancedMode, &QPushButton::clicked, this, &KernelConfigDialog::toggleAdvancedKernelMode);

    connect(m_btnFetchKernelOrg, &QPushButton::clicked, this, &KernelConfigDialog::fetchKernelOrgReleases);
    connect(m_comboSourceSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &KernelConfigDialog::onSourceSelectionChanged);
    connect(m_btnDownloadSource, &QPushButton::clicked, this, &KernelConfigDialog::downloadAndExtractSelectedKernel);

    connect(m_btnInheritFullConfig, &QPushButton::clicked, this, [this]() {
        inheritCurrentKernelConfig(false);
    });
    connect(m_btnInheritLocalModConfig, &QPushButton::clicked, this, [this]() {
        inheritCurrentKernelConfig(true);
    });

    connect(m_editKconfigSearch, &QLineEdit::textChanged, this, &KernelConfigDialog::filterKconfigTree);
    connect(m_chkShowEnabledOnly, &QCheckBox::toggled, this, [this]() {
        filterKconfigTree(m_editKconfigSearch->text());
    });
    connect(m_kconfigTree, &QTreeWidget::currentItemChanged, this, &KernelConfigDialog::onTreeItemSelected);
    connect(m_comboOptTriState, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &KernelConfigDialog::onOptionStateChanged);
    connect(m_editOptValue, &QLineEdit::editingFinished, this, &KernelConfigDialog::onOptionStateChanged);

    connect(m_btnSaveConfigOnly, &QPushButton::clicked, this, &KernelConfigDialog::saveDotConfigOnly);
    connect(m_btnCompileAndInstall, &QPushButton::clicked, this, &KernelConfigDialog::compileAndInstallCustomKernel);
    connect(m_btnCloseDialog, &QPushButton::clicked, this, &QDialog::reject);

    connect(m_internalProcess, &QProcess::readyReadStandardOutput,
            this, &KernelConfigDialog::readInternalProcessOutput);
    connect(m_internalProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &KernelConfigDialog::onInternalProcessFinished);

    // Aplicar tema inicial (Modo Seguro) y cargar datos
    applyDialogModeTheme();
    scanLocalKernelSources();
    fetchKernelOrgReleases();
}

KernelConfigDialog::~KernelConfigDialog() {
    if (m_internalProcess->state() != QProcess::NotRunning) {
        m_internalProcess->terminate();
        m_internalProcess->waitForFinished(3000);
    }
}

void KernelConfigDialog::closeEvent(QCloseEvent *event) {
    if (m_internalProcess->state() != QProcess::NotRunning) {
        QMessageBox::warning(
            this,
            "Proceso en curso",
            "Hay una operación de kernel en ejecución. Espere a que finalice para no dejar archivos incompletos."
        );
        event->ignore();
        return;
    }
    event->accept();
}

// ============================================================================
// MODO SEGURO POR DEFECTO VS CONFIGURACIÓN AVANZADA DE NUEVO KERNEL (VENTANA ROJA)
// ============================================================================

void KernelConfigDialog::applyDialogModeTheme() {
    if (!m_advancedMode) {
        // MODO SEGURO (Por defecto: hereda los módulos funcionales del kernel actual)
        setStyleSheet("");
        m_lblTopBanner->setText(
            "<b>MODO SEGURO ACTIVO (Recomendado):</b> Por defecto se toman automáticamente los módulos y drivers "
            "del kernel actualmente en ejecución (que sabemos que funcionan en este equipo) para garantizar que nada se rompa. "
            "Puede inspeccionar el árbol o pulsar <b>Compilar e Instalar Nuevo Kernel</b> directamente."
        );
        m_lblTopBanner->setStyleSheet(
            "background-color: #e8f5e9; color: #1b5e20; border: 1px solid #81c784; "
            "border-left: 5px solid #2e7d32; border-radius: 4px; padding: 8px; font-size: 9.2pt;"
        );

        m_btnToggleAdvancedMode->setText("Configuración Avanzada de Nuevo Kernel");
        m_btnToggleAdvancedMode->setStyleSheet(
            "QPushButton { background-color: #c62828; color: white; font-weight: bold; "
            "padding: 6px 14px; border-radius: 4px; }"
            "QPushButton:hover:enabled { background-color: #b71c1c; }"
        );

        m_btnInheritLocalModConfig->setEnabled(false);
        m_btnSaveConfigOnly->setVisible(false);
        m_comboOptTriState->setEnabled(false);
        m_editOptValue->setEnabled(false);
    } else {
        // MODO AVANZADO (VENTANA ROJA DE PRECAUCIÓN)
        setStyleSheet(
            "QDialog { background-color: #ffebee; border: 3px solid #c62828; }"
            "QGroupBox { background-color: #fff5f5; border: 1px solid #e57373; border-radius: 5px; "
            "margin-top: 10px; font-weight: bold; color: #b71c1c; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
        );

        m_lblTopBanner->setText(
            "<b>⚠️ MODO DE CONFIGURACIÓN AVANZADA ACTIVO — PRECAUCIÓN:</b> Ha habilitado la edición manual de los módulos "
            "del nuevo kernel. Desactivar controladores de disco (NVMe/SATA), sistemas de archivos o ACPI puede impedir que "
            "Slackware arranque (<i>Kernel Panic</i>). Revise cuidadosamente cada cambio antes de compilar."
        );
        m_lblTopBanner->setStyleSheet(
            "background-color: #b71c1c; color: #ffffff; border: 2px solid #7f0000; "
            "border-radius: 4px; padding: 9px; font-size: 9.4pt;"
        );

        m_btnToggleAdvancedMode->setText("Volver a Modo Seguro (Restaurar módulos heredados)");
        m_btnToggleAdvancedMode->setStyleSheet(
            "QPushButton { background-color: #2e7d32; color: white; font-weight: bold; "
            "padding: 6px 14px; border-radius: 4px; }"
            "QPushButton:hover:enabled { background-color: #1b5e20; }"
        );

        bool hasSrc = !m_activeSourceDir.isEmpty();
        m_btnInheritLocalModConfig->setEnabled(hasSrc);
        m_btnSaveConfigOnly->setVisible(true);
    }

    // Refrescar estado de controles del ítem seleccionado actualmente
    onTreeItemSelected(m_kconfigTree->currentItem(), nullptr);
}

void KernelConfigDialog::toggleAdvancedKernelMode() {
    if (!m_advancedMode) {
        QMessageBox::StandardButton resp = QMessageBox::warning(
            this,
            "⚠️ Advertencia de Riesgo - Configuración Avanzada del Kernel",
            "Está a punto de activar la <b>Configuración Avanzada de Nuevo Kernel</b>.\n\n"
            "• Por defecto, QSlackpkg utiliza los módulos del kernel actual porque ya están probados y garantizan el arranque.\n"
            "• Si modifica o desactiva manualmente drivers esenciales (almacenamiento, USB, sistema de archivos raíz o chipset), "
            "el nuevo kernel podría no iniciar.\n"
            "• No obstante, su kernel anterior permanecerá disponible en el menú de arranque (Slackware-Prev) como respaldo.\n\n"
            "¿Desea habilitar la manipulación manual de los módulos del kernel?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

        if (resp != QMessageBox::Yes) return;
        m_advancedMode = true;
    } else {
        m_advancedMode = false;
        // Restaurar inmediatamente la configuración heredada segura del kernel en ejecución
        loadCurrentDotConfigValues("/proc/config.gz");
        loadKconfigTree();
        m_lblConfigStatus->setText("Modo Seguro restaurado: módulos heredados del kernel en ejecución.");
    }

    applyDialogModeTheme();
}

void KernelConfigDialog::setDialogBusy(bool busy, const QString &statusText) {
    m_grpStep1->setEnabled(!busy);
    m_grpStep2->setEnabled(!busy);
    m_grpStep3->setEnabled(!busy);
    m_grpStep4->setEnabled(!busy);
    m_btnToggleAdvancedMode->setEnabled(!busy);

    m_internalProgress->setVisible(busy);
    if (!statusText.isEmpty()) {
        m_lblTaskProgressStatus->setText("Estado: " + statusText);
    }
}

void KernelConfigDialog::runInternalPrivilegedTask(TaskType type, const QString &script, const QString &statusMsg) {
    if (m_internalProcess->state() != QProcess::NotRunning) return;

    m_currentTask = type;
    m_internalConsole->clear();
    m_internalConsole->append(">>> Solicitando autenticación obligatoria de usuario ROOT...\n");
    setDialogBusy(true, statusMsg);

    QStringList args;
    args << "--user" << "root" << "/bin/sh" << "-c" << script;
    m_internalProcess->start("pkexec", args);
}

void KernelConfigDialog::readInternalProcessOutput() {
    QByteArray data = m_internalProcess->readAllStandardOutput();
    QString text = QString::fromUtf8(data);
    static QRegularExpression ansiRegex("\x1b\\[[0-9;]*[a-zA-Z]");
    text.remove(ansiRegex);

    m_internalConsole->moveCursor(QTextCursor::End);
    m_internalConsole->insertPlainText(text);
    m_internalConsole->verticalScrollBar()->setValue(m_internalConsole->verticalScrollBar()->maximum());
}

void KernelConfigDialog::onInternalProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    TaskType finishedTask = m_currentTask;
    m_currentTask = TaskNone;

    setDialogBusy(false, exitCode == 0 ? "Operación completada con éxito." : QString("Finalizado con código %1.").arg(exitCode));
    m_internalConsole->append(QString("\n>>> Proceso finalizado (Código: %1)").arg(exitCode));

    if (exitCode == 126 || exitCode == 127) {
        QMessageBox::warning(
            this,
            "Autenticación de ROOT Requerida",
            "No se ingresó la contraseña de root o la solicitud fue cancelada. No se realizó ningún cambio."
        );
        return;
    }

    if (exitCode != 0) {
        QMessageBox::critical(
            this,
            "Error en la operación",
            QString("El proceso finalizó con errores (Código: %1).\nRevise el registro de consola en la parte inferior de esta ventana.").arg(exitCode)
        );
        return;
    }

    if (finishedTask == TaskDownloadSource) {
        scanLocalKernelSources();
        QMessageBox::information(
            this,
            "Código fuente listo",
            "El nuevo kernel se descargó y extrajo correctamente en /usr/src.\n"
            "Se han cargado automáticamente los módulos de su kernel actual como base segura."
        );
    } else if (finishedTask == TaskInheritConfig || finishedTask == TaskSaveConfig) {
        if (!m_activeSourceDir.isEmpty() && QFile::exists(m_activeSourceDir + "/.config")) {
            loadCurrentDotConfigValues(m_activeSourceDir + "/.config");
            loadKconfigTree();
            m_lblConfigStatus->setText("Archivo .config actualizado y cargado desde " + m_activeSourceDir + "/.config");
        }
    } else if (finishedTask == TaskCompileKernel) {
        emit environmentChanged();
        emit kernelInstalledNotification(m_pendingCompiledVersion);

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Nuevo Kernel Compilado e Instalado con Éxito");
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText(
            QString("<b>¡El nuevo kernel (%1) se ha compilado e instalado correctamente!</b>")
                .arg(m_pendingCompiledVersion)
        );
        msgBox.setInformativeText(
            "El gestor de arranque ha sido actualizado conservando su kernel anterior como respaldo.\n"
            "Se ha dejado un aviso en la bandeja del sistema.\n\n"
            "¿Desea reiniciar el equipo ahora para iniciar con el nuevo kernel o hacerlo más tarde?"
        );
        QPushButton *btnRebootNow = msgBox.addButton("Reiniciar ahora", QMessageBox::AcceptRole);
        QPushButton *btnRebootLater = msgBox.addButton("Reiniciar más tarde", QMessageBox::RejectRole);
        msgBox.setDefaultButton(btnRebootLater);

        msgBox.exec();
        if (msgBox.clickedButton() == btnRebootNow) {
            runInternalPrivilegedTask(
                TaskRebootSystem,
                "/sbin/shutdown -r now || /sbin/reboot",
                "Reiniciando el sistema como root..."
            );
        }
    }
}

// ============================================================================
// PASO 1: FUENTES LOCALES Y DESCARGA DESDE KERNEL.ORG
// ============================================================================

QString KernelConfigDialog::readKernelVersionFromMakefile(const QString &srcDir) const {
    QFile makefile(srcDir + "/Makefile");
    if (!makefile.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();

    QString ver, patch, sub, extra;
    QTextStream in(&makefile);
    int linesRead = 0;
    while (!in.atEnd() && linesRead < 15) {
        QString line = in.readLine().trimmed();
        linesRead++;
        if (line.startsWith("VERSION =")) ver = line.mid(9).trimmed();
        else if (line.startsWith("PATCHLEVEL =")) patch = line.mid(12).trimmed();
        else if (line.startsWith("SUBLEVEL =")) sub = line.mid(10).trimmed();
        else if (line.startsWith("EXTRAVERSION =")) extra = line.mid(14).trimmed();
    }
    makefile.close();

    if (!ver.isEmpty() && !patch.isEmpty()) {
        return QString("%1.%2.%3%4").arg(ver, patch, sub, extra);
    }
    return QString();
}

void KernelConfigDialog::scanLocalKernelSources() {
    m_comboSourceSelect->blockSignals(true);
    m_comboSourceSelect->clear();

    QDir usrSrc("/usr/src");
    QStringList dirs = usrSrc.entryList({"linux*"}, QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);

    for (const QString &dirName : dirs) {
        QString fullPath = usrSrc.filePath(dirName);
        if (!QFile::exists(fullPath + "/Kconfig")) continue;

        QString ver = readKernelVersionFromMakefile(fullPath);
        QFileInfo fi(fullPath);
        QString tag = fi.isSymLink() ? " [Enlace activo /usr/src/linux]" : " [Local en /usr/src]";
        QString label = QString("%1 (Kernel %2)%3").arg(fullPath, ver.isEmpty() ? "?" : ver, tag);
        m_comboSourceSelect->addItem(label, "LOCAL:" + fullPath);
    }

    for (const KernelReleaseInfo &rel : m_remoteReleases) {
        QString label = QString("[Descargar Kernel.org] Linux %1 (%2 - %3)")
                            .arg(rel.version, rel.moniker, rel.released);
        m_comboSourceSelect->addItem(label, "REMOTE:" + rel.version);
    }

    m_comboSourceSelect->blockSignals(false);
    onSourceSelectionChanged();
}

void KernelConfigDialog::fetchKernelOrgReleases() {
    m_btnFetchKernelOrg->setEnabled(false);
    m_lblSourceStatus->setText("Consultando versiones oficiales disponibles en https://www.kernel.org/releases.json ...");

    QNetworkRequest req(QUrl("https://www.kernel.org/releases.json"));
    req.setHeader(QNetworkRequest::UserAgentHeader, "QSlackpkg/1.0 (Slackware Linux)");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_netManager->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onKernelOrgReleasesDownloaded(reply);
    });
}

void KernelConfigDialog::onKernelOrgReleasesDownloaded(QNetworkReply *reply) {
    m_btnFetchKernelOrg->setEnabled(true);

    if (reply->error() != QNetworkReply::NoError) {
        m_lblSourceStatus->setText("No se pudo conectar con kernel.org (se usarán las fuentes locales disponibles).");
        reply->deleteLater();
        return;
    }

    QByteArray raw = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (!doc.isObject()) return;

    m_remoteReleases.clear();
    QJsonArray releases = doc.object().value("releases").toArray();
    for (const QJsonValue &val : releases) {
        QJsonObject obj = val.toObject();
        QString sourceUrl = obj.value("source").toString();
        if (sourceUrl.isEmpty() || !sourceUrl.endsWith(".tar.xz")) continue;

        KernelReleaseInfo info;
        info.version = obj.value("version").toString();
        info.moniker = obj.value("moniker").toString();
        info.released = obj.value("released").toObject().value("isodate").toString();
        info.sourceUrl = sourceUrl;
        m_remoteReleases.append(info);
    }

    scanLocalKernelSources();
    m_lblSourceStatus->setText(
        QString("Listo: %1 árbol(es) local(es) y %2 versión(es) oficiales de kernel.org disponibles.")
            .arg(QDir("/usr/src").entryList({"linux*"}, QDir::Dirs | QDir::NoDotAndDotDot).size())
            .arg(m_remoteReleases.size())
    );
}

void KernelConfigDialog::onSourceSelectionChanged() {
    QString data = m_comboSourceSelect->currentData().toString();
    if (data.startsWith("LOCAL:")) {
        m_activeSourceDir = data.mid(6);
        m_btnDownloadSource->setEnabled(false);
        m_btnInheritFullConfig->setEnabled(true);
        m_btnInheritLocalModConfig->setEnabled(m_advancedMode);

        // Por defecto SIEMPRE partimos de los módulos funcionales del kernel actual
        // salvo que el usuario esté en modo avanzado y ya tenga un .config editado
        if (m_advancedMode && QFile::exists(m_activeSourceDir + "/.config")) {
            m_lblConfigStatus->setText("Configuración personalizada cargada desde " + m_activeSourceDir + "/.config");
            loadCurrentDotConfigValues(m_activeSourceDir + "/.config");
        } else {
            m_lblConfigStatus->setText("Base segura: módulos heredados del kernel actualmente en ejecución.");
            loadCurrentDotConfigValues("/proc/config.gz");
        }
        loadKconfigTree();
    } else if (data.startsWith("REMOTE:")) {
        m_activeSourceDir.clear();
        m_btnDownloadSource->setEnabled(true);
        m_btnInheritFullConfig->setEnabled(false);
        m_btnInheritLocalModConfig->setEnabled(false);
        m_kconfigTree->clear();
        m_lblSourceStatus->setText("Versión remota seleccionada. Pulse 'Descargar y Extraer en /usr/src (root)' para prepararla.");
    }
}

void KernelConfigDialog::downloadAndExtractSelectedKernel() {
    QString data = m_comboSourceSelect->currentData().toString();
    if (!data.startsWith("REMOTE:")) return;

    QString targetVer = data.mid(7);
    KernelReleaseInfo selectedRel;
    for (const auto &r : m_remoteReleases) {
        if (r.version == targetVer) {
            selectedRel = r;
            break;
        }
    }
    if (selectedRel.sourceUrl.isEmpty()) return;

    QMessageBox::StandardButton resp = QMessageBox::question(
        this,
        "Descargar Código Fuente del Kernel (Requiere root)",
        QString("Se descargará y extraerá el código fuente oficial desde Kernel.org:\n\n"
                "• Versión: Linux %1 (%2)\n"
                "• URL: %3\n"
                "• Destino: /usr/src/linux-%1 (y enlace /usr/src/linux)\n\n"
                "La ventana permanecerá abierta mostrando el progreso hasta finalizar.\n"
                "Se solicitará contraseña de root. ¿Continuar?")
            .arg(selectedRel.version, selectedRel.moniker, selectedRel.sourceUrl),
        QMessageBox::Yes | QMessageBox::No
    );
    if (resp != QMessageBox::Yes) return;

    QString tarFile = QString("/tmp/linux-%1.tar.xz").arg(selectedRel.version);
    QString cmd = QString(
        "set -e; "
        "wget -O '%1' '%2'; "
        "mkdir -p /usr/src; "
        "tar -xJf '%1' -C /usr/src/; "
        "rm -f '%1'; "
        "rm -f /usr/src/linux; "
        "ln -sf '/usr/src/linux-%3' /usr/src/linux; "
        "echo '>>> Código fuente extraído en /usr/src/linux-%3'"
    ).arg(tarFile, selectedRel.sourceUrl, selectedRel.version);

    runInternalPrivilegedTask(
        TaskDownloadSource,
        cmd,
        QString("Descargando y extrayendo Linux %1 en /usr/src...").arg(selectedRel.version)
    );
}

// ============================================================================
// PASO 2: HERENCIA DE MÓDULOS DEL KERNEL EXISTENTE
// ============================================================================

void KernelConfigDialog::inheritCurrentKernelConfig(bool runLocalModConfig) {
    if (m_activeSourceDir.isEmpty() || !QDir(m_activeSourceDir).exists()) {
        QMessageBox::warning(this, "Sin árbol de fuentes", "Seleccione primero un directorio válido en /usr/src.");
        return;
    }

    QString targetMode = runLocalModConfig ? "localmodconfig" : "olddefconfig";
    QString modeDesc = runLocalModConfig
        ? "Heredará su configuración actual y desactivará los drivers que NO estén cargados en este momento (lsmod)."
        : "Heredará exactamente todos los módulos y drivers funcionales del kernel con el que inició el sistema (olddefconfig).";

    QMessageBox::StandardButton resp = QMessageBox::question(
        this,
        QString("Generar .config (%1)").arg(targetMode),
        QString("Árbol destino: %1\n\n%2\n\n"
                "Se ejecutará 'make %3' como root manteniendo esta ventana abierta.\n"
                "¿Continuar?")
            .arg(m_activeSourceDir, modeDesc, targetMode),
        QMessageBox::Yes | QMessageBox::No
    );
    if (resp != QMessageBox::Yes) return;

    QString escapedDir = QString(m_activeSourceDir).replace("'", "'\\''");
    QString cmd = QString(
        "set -e; "
        "cd '%1'; "
        "if [ -f /proc/config.gz ]; then "
        "  zcat /proc/config.gz > .config; "
        "elif [ -f \"/boot/config-$(uname -r)\" ]; then "
        "  cp -f \"/boot/config-$(uname -r)\" .config; "
        "else "
        "  make defconfig; "
        "fi; "
        "if [ '%2' = 'localmodconfig' ]; then "
        "  LSMOD_FILE=$(mktemp /tmp/qslackpkg_lsmod.XXXXXX); "
        "  /sbin/lsmod > \"$LSMOD_FILE\"; "
        "  yes '' | make LSMOD=\"$LSMOD_FILE\" localmodconfig; "
        "  rm -f \"$LSMOD_FILE\"; "
        "else "
        "  make olddefconfig; "
        "fi; "
        "chmod 644 .config; "
        "echo '>>> Archivo .config generado exitosamente en %1/.config'"
    ).arg(escapedDir, targetMode);

    runInternalPrivilegedTask(
        TaskInheritConfig,
        cmd,
        QString("Aplicando %1 en %2...").arg(targetMode, m_activeSourceDir)
    );
}

// ============================================================================
// PASO 3: PARSER Y ÁRBOL VISUAL KCONFIG
// ============================================================================

void KernelConfigDialog::loadCurrentDotConfigValues(const QString &dotConfigPath) {
    m_dotConfigValues.clear();

    QByteArray content;
    if (dotConfigPath.endsWith(".gz") && QFile::exists(dotConfigPath)) {
        QProcess zcat;
        zcat.start("zcat", {dotConfigPath});
        if (zcat.waitForFinished(3000)) {
            content = zcat.readAllStandardOutput();
        }
    } else if (QFile::exists(dotConfigPath)) {
        QFile file(dotConfigPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            content = file.readAll();
            file.close();
        }
    }

    // Fallback si /proc/config.gz no estuviera habilitado en el kernel actual
    if (content.isEmpty()) {
        QProcess uname;
        uname.start("uname", {"-r"});
        if (uname.waitForFinished(2000)) {
            QString runningVer = QString::fromUtf8(uname.readAllStandardOutput()).trimmed();
            QFile bootCfg("/boot/config-" + runningVer);
            if (bootCfg.open(QIODevice::ReadOnly | QIODevice::Text)) {
                content = bootCfg.readAll();
                bootCfg.close();
            }
        }
    }

    if (content.isEmpty()) return;

    QStringList lines = QString::fromUtf8(content).split('\n');
    QRegularExpression notSetRx("^#\\s*CONFIG_([A-Za-z0-9_]+)\\s+is not set$");

    for (const QString &rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.startsWith("CONFIG_")) {
            int eq = line.indexOf('=');
            if (eq > 7) {
                QString sym = line.mid(7, eq - 7);
                QString val = line.mid(eq + 1).trimmed();
                m_dotConfigValues.insert(sym, val);
            }
        } else {
            QRegularExpressionMatch m = notSetRx.match(line);
            if (m.hasMatch()) {
                m_dotConfigValues.insert(m.captured(1), "n");
            }
        }
    }
}

void KernelConfigDialog::loadKconfigTree() {
    m_kconfigTree->clear();
    m_kconfigOptions.clear();
    m_symbolToItem.clear();

    if (m_activeSourceDir.isEmpty() || !QFile::exists(m_activeSourceDir + "/Kconfig")) {
        m_lblOptionsCounter->setText("0 opciones cargadas");
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_kconfigTree->setUpdatesEnabled(false);

    QSet<QString> visitedFiles;
    parseKconfigRecursive("Kconfig", nullptr, visitedFiles, 0);

    m_kconfigTree->setUpdatesEnabled(true);
    QApplication::restoreOverrideCursor();

    m_lblOptionsCounter->setText(QString("%1 símbolos Kconfig cargados").arg(m_kconfigOptions.size()));
    filterKconfigTree(m_editKconfigSearch->text());
}

void KernelConfigDialog::parseKconfigRecursive(const QString &kconfigRelPath,
                                               QTreeWidgetItem *parentItem,
                                               QSet<QString> &visitedFiles,
                                               int depth)
{
    if (depth > 14) return;

    QString resolvedPath = kconfigRelPath;
    resolvedPath.replace("$(SRCARCH)", "x86");
    resolvedPath.replace("$SRCARCH", "x86");
    resolvedPath.replace("$(ARCH)", "x86_64");

    QString fullPath = m_activeSourceDir + "/" + resolvedPath;
    if (!QFile::exists(fullPath) || visitedFiles.contains(fullPath)) return;
    visitedFiles.insert(fullPath);

    QFile file(fullPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
    QList<QTreeWidgetItem*> menuStack;
    if (parentItem) menuStack.append(parentItem);

    KconfigOption currentOpt;
    bool inConfigBlock = false;
    bool inHelpBlock = false;
    int helpIndent = -1;

    auto flushCurrentOption = [&]() {
        if (!inConfigBlock || currentOpt.symbol.isEmpty()) return;

        if (m_dotConfigValues.contains(currentOpt.symbol)) {
            currentOpt.value = m_dotConfigValues.value(currentOpt.symbol);
        } else if (!currentOpt.defaultVal.isEmpty()) {
            currentOpt.value = currentOpt.defaultVal;
        } else {
            currentOpt.value = "n";
        }

        if (currentOpt.prompt.isEmpty()) {
            currentOpt.prompt = currentOpt.symbol;
        }

        QTreeWidgetItem *targetParent = menuStack.isEmpty() ? nullptr : menuStack.last();
        QTreeWidgetItem *item = targetParent
            ? new QTreeWidgetItem(targetParent)
            : new QTreeWidgetItem(m_kconfigTree);

        item->setData(0, Qt::UserRole, currentOpt.symbol);
        updateTreeItemVisual(item, currentOpt);

        m_kconfigOptions.insert(currentOpt.symbol, currentOpt);
        m_symbolToItem.insert(currentOpt.symbol, item);

        if (currentOpt.isMenuOnly) {
            menuStack.append(item);
        }

        currentOpt = KconfigOption();
        inConfigBlock = false;
        inHelpBlock = false;
        helpIndent = -1;
    };

    while (!in.atEnd()) {
        QString rawLine = in.readLine();
        int indent = 0;
        while (indent < rawLine.length() && (rawLine[indent] == ' ' || rawLine[indent] == '\t')) {
            indent += (rawLine[indent] == '\t') ? 8 : 1;
        }
        QString trimmed = rawLine.trimmed();

        if (inHelpBlock) {
            if (trimmed.isEmpty()) {
                currentOpt.helpText += "\n";
                continue;
            }
            if (helpIndent == -1) helpIndent = indent;
            if (indent >= helpIndent && !trimmed.startsWith("config ") &&
                !trimmed.startsWith("menuconfig ") && !trimmed.startsWith("menu ") &&
                !trimmed.startsWith("endmenu") && !trimmed.startsWith("source ")) {
                currentOpt.helpText += trimmed + " ";
                continue;
            } else {
                inHelpBlock = false;
            }
        }

        if (trimmed.isEmpty() || trimmed.startsWith('#')) continue;

        if (trimmed.startsWith("source ")) {
            flushCurrentOption();
            QString subPath = trimmed.mid(7).trimmed();
            if ((subPath.startsWith('"') && subPath.endsWith('"')) ||
                (subPath.startsWith('\'') && subPath.endsWith('\''))) {
                subPath = subPath.mid(1, subPath.length() - 2);
            }
            QTreeWidgetItem *curParent = menuStack.isEmpty() ? parentItem : menuStack.last();
            parseKconfigRecursive(subPath, curParent, visitedFiles, depth + 1);
        } else if (trimmed.startsWith("menu ")) {
            flushCurrentOption();
            QString menuTitle = trimmed.mid(5).trimmed();
            if (menuTitle.startsWith('"') && menuTitle.endsWith('"')) {
                menuTitle = menuTitle.mid(1, menuTitle.length() - 2);
            }
            QTreeWidgetItem *curParent = menuStack.isEmpty() ? nullptr : menuStack.last();
            QTreeWidgetItem *menuItem = curParent
                ? new QTreeWidgetItem(curParent)
                : new QTreeWidgetItem(m_kconfigTree);
            menuItem->setText(0, "📁 " + menuTitle);
            menuItem->setText(1, "Submenú");
            QFont boldFont = menuItem->font(0);
            boldFont.setBold(true);
            menuItem->setFont(0, boldFont);
            menuItem->setForeground(0, QColor("#0d47a1"));
            menuStack.append(menuItem);
        } else if (trimmed == "endmenu") {
            flushCurrentOption();
            if (!menuStack.isEmpty() && menuStack.last() != parentItem) {
                menuStack.removeLast();
            }
        } else if (trimmed.startsWith("config ") || trimmed.startsWith("menuconfig ")) {
            flushCurrentOption();
            inConfigBlock = true;
            currentOpt = KconfigOption();
            currentOpt.sourceFile = resolvedPath;
            if (trimmed.startsWith("menuconfig ")) {
                currentOpt.symbol = trimmed.mid(11).trimmed();
                currentOpt.isMenuOnly = true;
            } else {
                currentOpt.symbol = trimmed.mid(7).trimmed();
            }
        } else if (inConfigBlock) {
            auto extractQuoted = [](const QString &s) -> QString {
                int firstQ = s.indexOf('"');
                int lastQ = s.lastIndexOf('"');
                if (firstQ >= 0 && lastQ > firstQ) {
                    return s.mid(firstQ + 1, lastQ - firstQ - 1);
                }
                return QString();
            };

            if (trimmed.startsWith("bool") || trimmed.startsWith("def_bool")) {
                currentOpt.type = "bool";
                QString p = extractQuoted(trimmed);
                if (!p.isEmpty()) currentOpt.prompt = p;
            } else if (trimmed.startsWith("tristate") || trimmed.startsWith("def_tristate")) {
                currentOpt.type = "tristate";
                QString p = extractQuoted(trimmed);
                if (!p.isEmpty()) currentOpt.prompt = p;
            } else if (trimmed.startsWith("string")) {
                currentOpt.type = "string";
                QString p = extractQuoted(trimmed);
                if (!p.isEmpty()) currentOpt.prompt = p;
            } else if (trimmed.startsWith("int")) {
                currentOpt.type = "int";
                QString p = extractQuoted(trimmed);
                if (!p.isEmpty()) currentOpt.prompt = p;
            } else if (trimmed.startsWith("hex")) {
                currentOpt.type = "hex";
                QString p = extractQuoted(trimmed);
                if (!p.isEmpty()) currentOpt.prompt = p;
            } else if (trimmed.startsWith("prompt ")) {
                QString p = extractQuoted(trimmed);
                if (!p.isEmpty()) currentOpt.prompt = p;
            } else if (trimmed.startsWith("default ")) {
                if (currentOpt.defaultVal.isEmpty()) {
                    QString def = trimmed.mid(8).trimmed();
                    int ifIdx = def.indexOf(" if ");
                    if (ifIdx > 0) def = def.left(ifIdx).trimmed();
                    currentOpt.defaultVal = def;
                }
            } else if (trimmed.startsWith("depends on ")) {
                if (!currentOpt.dependsOn.isEmpty()) currentOpt.dependsOn += " && ";
                currentOpt.dependsOn += trimmed.mid(11).trimmed();
            } else if (trimmed.startsWith("select ")) {
                if (!currentOpt.selects.isEmpty()) currentOpt.selects += ", ";
                currentOpt.selects += trimmed.mid(7).trimmed();
            } else if (trimmed == "help" || trimmed == "---help---") {
                inHelpBlock = true;
                helpIndent = -1;
            }
        }
    }

    flushCurrentOption();
    file.close();
}

void KernelConfigDialog::updateTreeItemVisual(QTreeWidgetItem *item, const KconfigOption &opt) {
    item->setText(0, opt.prompt);
    item->setText(2, "CONFIG_" + opt.symbol);

    if (opt.value == "y") {
        item->setText(1, "[ * ] Integrado");
        item->setForeground(1, QColor("#1b5e20"));
        item->setBackground(1, QColor("#e8f5e9"));
    } else if (opt.value == "m") {
        item->setText(1, "[ M ] Módulo");
        item->setForeground(1, QColor("#0d47a1"));
        item->setBackground(1, QColor("#e3f2fd"));
    } else if (opt.value == "n" || opt.value.isEmpty()) {
        item->setText(1, "[   ] Off");
        item->setForeground(1, QColor("#757575"));
        item->setBackground(1, Qt::transparent);
    } else {
        item->setText(1, opt.value);
        item->setForeground(1, QColor("#4a148c"));
        item->setBackground(1, QColor("#f3e5f5"));
    }
}

void KernelConfigDialog::onTreeItemSelected(QTreeWidgetItem *current, QTreeWidgetItem *previous) {
    Q_UNUSED(previous);
    if (!current) {
        m_comboOptTriState->setEnabled(false);
        m_editOptValue->setEnabled(false);
        return;
    }

    QString symbol = current->data(0, Qt::UserRole).toString();
    if (symbol.isEmpty() || !m_kconfigOptions.contains(symbol)) {
        m_lblOptTitle->setText(current->text(0));
        m_lblOptSymbol->setText("Categoría / Submenú del Kernel");
        m_lblOptDepends->setText("Expanda este nodo para ver sus drivers y subsistemas.");
        m_comboOptTriState->setEnabled(false);
        m_comboOptTriState->setVisible(true);
        m_editOptValue->setVisible(false);
        m_txtOptHelp->clear();
        return;
    }

    const KconfigOption &opt = m_kconfigOptions[symbol];
    m_updatingUi = true;

    m_lblOptTitle->setText(opt.prompt);
    m_lblOptSymbol->setText(QString("Símbolo: CONFIG_%1  |  Tipo: %2  |  Archivo: %3")
                                .arg(opt.symbol, opt.type.isEmpty() ? "bool" : opt.type, opt.sourceFile));
    m_lblOptDepends->setText(
        QString("Depende de: %1%2")
            .arg(opt.dependsOn.isEmpty() ? "Ninguna" : opt.dependsOn,
                 opt.selects.isEmpty() ? "" : "  |  Activa automáticamente: " + opt.selects)
    );

    if (opt.type == "string" || opt.type == "int" || opt.type == "hex") {
        m_comboOptTriState->setVisible(false);
        m_editOptValue->setVisible(true);
        m_editOptValue->setEnabled(m_advancedMode);
        m_editOptValue->setText(opt.value);
    } else {
        m_editOptValue->setVisible(false);
        m_comboOptTriState->setVisible(true);
        m_comboOptTriState->setEnabled(m_advancedMode);

        QStandardItemModel *model = qobject_cast<QStandardItemModel*>(m_comboOptTriState->model());
        if (model && model->item(2)) {
            model->item(2)->setEnabled(opt.type != "bool");
        }

        int idx = m_comboOptTriState->findData(opt.value.isEmpty() ? "n" : opt.value);
        if (idx < 0) idx = 0;
        m_comboOptTriState->setCurrentIndex(idx);
    }

    QString lockNotice = m_advancedMode
        ? ""
        : "\n\n[Modo Seguro: Este módulo está bloqueado en solo lectura con el valor heredado de su kernel actual. "
          "Pulse 'Configuración Avanzada de Nuevo Kernel' si desea modificarlo.]";

    m_txtOptHelp->setPlainText(
        (opt.helpText.trimmed().isEmpty()
            ? "Sin descripción extendida disponible para este símbolo en Kconfig."
            : opt.helpText.trimmed()) + lockNotice
    );

    m_updatingUi = false;
}

void KernelConfigDialog::onOptionStateChanged() {
    if (m_updatingUi || !m_advancedMode) return;
    QTreeWidgetItem *current = m_kconfigTree->currentItem();
    if (!current) return;

    QString symbol = current->data(0, Qt::UserRole).toString();
    if (symbol.isEmpty() || !m_kconfigOptions.contains(symbol)) return;

    KconfigOption &opt = m_kconfigOptions[symbol];
    if (m_editOptValue->isVisible()) {
        opt.value = m_editOptValue->text().trimmed();
    } else {
        opt.value = m_comboOptTriState->currentData().toString();
    }

    m_dotConfigValues.insert(symbol, opt.value);
    updateTreeItemVisual(current, opt);
}

bool KernelConfigDialog::filterTreeRecursive(QTreeWidgetItem *item, const QString &query) {
    bool onlyEnabled = m_chkShowEnabledOnly->isChecked();
    QString symbol = item->data(0, Qt::UserRole).toString();

    bool childVisible = false;
    for (int i = 0; i < item->childCount(); ++i) {
        if (filterTreeRecursive(item->child(i), query)) {
            childVisible = true;
        }
    }

    bool selfMatches = true;
    if (!symbol.isEmpty() && m_kconfigOptions.contains(symbol)) {
        const KconfigOption &opt = m_kconfigOptions[symbol];
        if (onlyEnabled && (opt.value == "n" || opt.value.isEmpty())) {
            selfMatches = false;
        }
        if (selfMatches && !query.isEmpty()) {
            selfMatches = opt.prompt.contains(query, Qt::CaseInsensitive) ||
                          opt.symbol.contains(query, Qt::CaseInsensitive) ||
                          opt.helpText.contains(query, Qt::CaseInsensitive);
        }
    } else {
        if (!query.isEmpty() || onlyEnabled) {
            selfMatches = childVisible;
        }
    }

    bool visible = selfMatches || childVisible;
    item->setHidden(!visible);
    if (visible && (!query.isEmpty() || onlyEnabled) && childVisible) {
        item->setExpanded(true);
    }
    return visible;
}

void KernelConfigDialog::filterKconfigTree(const QString &query) {
    m_kconfigTree->setUpdatesEnabled(false);
    for (int i = 0; i < m_kconfigTree->topLevelItemCount(); ++i) {
        filterTreeRecursive(m_kconfigTree->topLevelItem(i), query.trimmed());
    }
    m_kconfigTree->setUpdatesEnabled(true);
}

QString KernelConfigDialog::generateDotConfigContent() const {
    QStringList lines;
    lines << "#";
    lines << "# Automatically generated by QSlackpkg Kernel Configurator";
    lines << "#";

    for (auto it = m_dotConfigValues.constBegin(); it != m_dotConfigValues.constEnd(); ++it) {
        const QString &sym = it.key();
        const QString &val = it.value();
        if (val == "n" || val.isEmpty()) {
            lines << QString("# CONFIG_%1 is not set").arg(sym);
        } else {
            lines << QString("CONFIG_%1=%2").arg(sym, val);
        }
    }
    return lines.join("\n") + "\n";
}

// ============================================================================
// GENERACIÓN Y ACTUALIZACIÓN LIMPIA DE LILO.CONF (SIN %S CORRUPTOS)
// ============================================================================

QString KernelConfigDialog::buildBootloaderPreviewSnippet(const QString &kernelFileName,
                                                          const QString &initrdFileName,
                                                          bool doInitrd) const
{
    if (m_bootloaderType == "LILO") {
        QString snippet = QString(
            "# Se clonará /etc/lilo.conf -> /etc/lilo.conf.bak.<fecha>\n"
            "# Se conservarán intactas todas las cabeceras globales y entradas anteriores (Slackware-Prev)\n"
            "# Nueva entrada insertada al inicio de las imágenes:\n\n"
            "image = /boot/%1\n"
            "  root = %2\n"
            "  label = Slackware\n"
        ).arg(kernelFileName, m_rootDevice);
        if (doInitrd) {
            snippet += QString("  initrd = /boot/%1\n").arg(initrdFileName);
        }
        snippet += "  read-only\n";
        snippet += "\n# Finalmente se ejecutará: /sbin/lilo -v";
        return snippet;
    } else if (m_bootloaderType == "ELILO") {
        QString snippet = QString(
            "# Se clonará %1 -> %1.bak.<fecha>\n"
            "image=%2\n"
            "\tlabel=Slackware\n"
        ).arg(m_bootloaderConfigPath, kernelFileName);
        if (doInitrd) {
            snippet += QString("\tinitrd=%1\n").arg(initrdFileName);
        }
        snippet += QString("\tread-only\n\tappend=\"root=%1 vga=normal ro\"\n").arg(m_rootDevice);
        return snippet;
    } else if (m_bootloaderType == "GRUB") {
        return "grub-mkconfig -o /boot/grub/grub.cfg (con respaldo previo .bak)";
    }
    return "Sin gestor de arranque estándar detectado.";
}

QString KernelConfigDialog::buildLiloUpdateShellScript(const QString &kernelFileName,
                                                       const QString &initrdFileName,
                                                       bool doInitrd) const
{
    // Creamos la nueva entrada en un archivo temporal limpio para evitar cualquier fallo de escape o %s
    QString newEntryPath = "/tmp/qslackpkg_lilo_new_entry.txt";
    QFile f(newEntryPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&f);
        ts << "image = /boot/" << kernelFileName << "\n";
        ts << "  root = " << m_rootDevice << "\n";
        ts << "  label = Slackware\n";
        if (doInitrd) {
            ts << "  initrd = /boot/" << initrdFileName << "\n";
        }
        ts << "  read-only\n";
        f.close();
    }

    QString keepFlag = m_keepFallbackEntry ? "1" : "0";

    // Script shell atómico con awk:
    // 1. Clona /etc/lilo.conf a /etc/lilo.conf.bak.<timestamp>
    // 2. Mantiene intacta la cabecera global (boot=, prompt, timeout, vga=, etc.)
    // 3. Inserta la nueva entrada desde /tmp/qslackpkg_lilo_new_entry.txt
    // 4. Conserva todas las entradas anteriores (renombrando la primera label=Slackware previa a Slackware-Prev)
    // 5. Ejecuta /sbin/lilo -v y si falla, restaura el respaldo automáticamente
    return QString(
        "LILO_BAK=\"/etc/lilo.conf.bak.$(date +%%Y%%m%%d%%H%%M%%S)\"; "
        "cp -p /etc/lilo.conf \"$LILO_BAK\"; "
        "LILO_TMP=$(mktemp /tmp/qslackpkg_lilo.XXXXXX); "
        "awk -v entry_file='%1' -v keep_prev='%2' ' "
        "BEGIN { in_images = 0; renamed_prev = 0 } "
        "/^[[:space:]]*(image|other)[[:space:]]*=/ { "
        "  if (!in_images) { "
        "    in_images = 1; "
        "    while ((getline line < entry_file) > 0) print line; "
        "    close(entry_file); "
        "    print \"\"; "
        "  } "
        "} "
        "{ "
        "  if (!in_images) { "
        "    print $0; "
        "  } else if (keep_prev == \"1\") { "
        "    if (!renamed_prev && $0 ~ /^[[:space:]]*label[[:space:]]*=[[:space:]]*Slackware[[:space:]]*$/) { "
        "      sub(/Slackware/, \"Slackware-Prev\"); "
        "      renamed_prev = 1; "
        "    } "
        "    print $0; "
        "  } "
        "} "
        "END { "
        "  if (!in_images) { "
        "    print \"\"; "
        "    while ((getline line < entry_file) > 0) print line; "
        "    close(entry_file); "
        "  } "
        "} ' \"$LILO_BAK\" > \"$LILO_TMP\"; "
        "rm -f '%1'; "
        "cat \"$LILO_TMP\" > /etc/lilo.conf; "
        "rm -f \"$LILO_TMP\"; "
        "if ! /sbin/lilo -v; then "
        "  echo 'ERROR: /sbin/lilo falló. Restaurando respaldo original...'; "
        "  cp -p \"$LILO_BAK\" /etc/lilo.conf; "
        "  exit 1; "
        "fi"
    ).arg(newEntryPath, keepFlag);
}

// ============================================================================
// PASO 4: INFORME PREVIO, GUARDADO Y COMPILACIÓN PERSISTENTE
// ============================================================================

void KernelConfigDialog::saveDotConfigOnly() {
    if (m_activeSourceDir.isEmpty()) return;

    QString tmpCfg = "/tmp/qslackpkg_kernel.config";
    QFile out(tmpCfg);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "No se pudo escribir el archivo temporal en /tmp.");
        return;
    }
    out.write(generateDotConfigContent().toUtf8());
    out.close();

    QString escapedDir = QString(m_activeSourceDir).replace("'", "'\\''");
    QString cmd = QString(
        "set -e; "
        "cd '%1'; "
        "if [ -f .config ]; then cp -p .config .config.bak; fi; "
        "install -m 644 '%2' .config && rm -f '%2'; "
        "make olddefconfig; "
        "echo '>>> Configuración .config guardada y validada correctamente.'"
    ).arg(escapedDir, tmpCfg);

    runInternalPrivilegedTask(
        TaskSaveConfig,
        cmd,
        QString("Guardando .config en %1...").arg(m_activeSourceDir)
    );
}

bool KernelConfigDialog::showPreCompileReportDialog(const QString &targetVer,
                                                    int jobs,
                                                    bool doInitrd,
                                                    bool doBoot)
{
    int builtInCount = 0;
    int moduleCount = 0;
    for (auto it = m_dotConfigValues.constBegin(); it != m_dotConfigValues.constEnd(); ++it) {
        if (it.value() == "y") builtInCount++;
        else if (it.value() == "m") moduleCount++;
    }

    QString kernelFileName = QString("vmlinuz-custom-%1").arg(targetVer);
    QString initrdFileName = QString("initrd-%1.gz").arg(targetVer);

    QDialog reportDialog(this);
    reportDialog.setWindowTitle("Informe Previo a la Compilación del Nuevo Kernel");
    reportDialog.resize(760, 560);

    QVBoxLayout *layout = new QVBoxLayout(&reportDialog);
    layout->setSpacing(10);

    QString modeBanner = m_advancedMode
        ? "<b style='color:#b71c1c;'>MODO AVANZADO (Personalizado manualmente)</b>"
        : "<b style='color:#1b5e20;'>MODO SEGURO (Módulos heredados del kernel funcional actual)</b>";

    QLabel *lblHeader = new QLabel(
        QString("<b>Revise atentamente el informe completo de su configuración antes de iniciar la compilación:</b><br>"
                "Perfil utilizado: %1").arg(modeBanner),
        &reportDialog
    );
    lblHeader->setWordWrap(true);
    lblHeader->setStyleSheet("background-color: #eceff1; padding: 10px; border-radius: 4px; font-size: 9.5pt;");
    layout->addWidget(lblHeader);

    QStringList reportLines;
    reportLines << QString("1. Árbol de código fuente: %1").arg(m_activeSourceDir);
    reportLines << QString("2. Versión del nuevo kernel: %1").arg(targetVer);
    reportLines << QString("3. Base de módulos: %1").arg(
        m_advancedMode
            ? "Configuración modificada manualmente por el usuario (Modo Avanzado)"
            : "Heredada íntegramente del kernel actual en ejecución (/proc/config.gz + olddefconfig)"
    );
    reportLines << QString("4. Resumen de símbolos activos: %1 integrados (=y) | %2 módulos cargables (=m)")
                       .arg(builtInCount).arg(moduleCount);
    reportLines << QString("5. Hilos paralelos de compilación: %1 núcleos (-j%1)").arg(jobs);
    reportLines << QString("6. Imagen de salida en /boot: /boot/%1").arg(kernelFileName);
    reportLines << QString("7. Módulos instalados en: /lib/modules/%1").arg(targetVer);
    reportLines << QString("8. Generación de initrd: %1").arg(
        doInitrd
            ? QString("Sí -> /boot/%1 (Raíz: %2, FS: %3)").arg(initrdFileName, m_rootDevice, m_rootFs)
            : "No"
    );
    reportLines << QString("9. Actualización de Bootloader (%1): %2").arg(
        m_bootloaderType,
        doBoot ? "Sí (clonando configuración anterior como respaldo)" : "No"
    );

    QTextEdit *txtReport = new QTextEdit(&reportDialog);
    txtReport->setReadOnly(true);
    txtReport->setMaximumHeight(195);
    txtReport->setPlainText(reportLines.join("\n"));
    txtReport->setStyleSheet("background-color: #fafafa; color: #212121; font-size: 9.2pt; border: 1px solid #cfd8dc;");
    layout->addWidget(txtReport);

    if (doBoot) {
        layout->addWidget(new QLabel(QString("<b>Vista previa de la actualización en %1:</b>").arg(m_bootloaderType), &reportDialog));
        QTextEdit *txtBootPreview = new QTextEdit(&reportDialog);
        txtBootPreview->setReadOnly(true);
        txtBootPreview->setMaximumHeight(135);
        txtBootPreview->setPlainText(buildBootloaderPreviewSnippet(kernelFileName, initrdFileName, doInitrd));
        txtBootPreview->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: monospace; font-size: 8.8pt;");
        layout->addWidget(txtBootPreview);
    }

    QCheckBox *chkConfirm = new QCheckBox(
        "He revisado el informe completo y estoy seguro de las configuraciones elegidas para compilar el kernel.",
        &reportDialog
    );
    chkConfirm->setStyleSheet("font-weight: bold; color: #0d47a1; padding: 4px;");
    layout->addWidget(chkConfirm);

    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *btnBack = new QPushButton("Volver y revisar", &reportDialog);
    QPushButton *btnStart = new QPushButton("Confirmar e Iniciar Compilación (root)", &reportDialog);
    btnStart->setEnabled(false);
    btnStart->setStyleSheet(
        "QPushButton { background-color: #2e7d32; color: white; font-weight: bold; padding: 7px 16px; border-radius: 4px; }"
        "QPushButton:hover:enabled { background-color: #1b5e20; }"
        "QPushButton:disabled { background-color: #a5d6a7; color: #f1f8e9; }"
    );

    btnRow->addStretch();
    btnRow->addWidget(btnBack);
    btnRow->addWidget(btnStart);
    layout->addLayout(btnRow);

    connect(chkConfirm, &QCheckBox::toggled, btnStart, &QPushButton::setEnabled);
    connect(btnBack, &QPushButton::clicked, &reportDialog, &QDialog::reject);
    connect(btnStart, &QPushButton::clicked, &reportDialog, &QDialog::accept);

    return reportDialog.exec() == QDialog::Accepted;
}

void KernelConfigDialog::compileAndInstallCustomKernel() {
    if (m_activeSourceDir.isEmpty()) {
        QMessageBox::warning(this, "Sin código fuente", "Seleccione o descargue primero el código fuente del kernel en el Paso 1.");
        return;
    }

    QString targetVer = readKernelVersionFromMakefile(m_activeSourceDir);
    if (targetVer.isEmpty()) targetVer = "custom";

    int jobs = m_spinJobs->value();
    bool doInitrd = m_chkBuildInitrdAfter->isChecked();
    bool doBoot = m_chkUpdateBootAfter->isChecked();

    // Mostrar el informe completo de configuraciones y pedir confirmación explícita
    if (!showPreCompileReportDialog(targetVer, jobs, doInitrd, doBoot)) {
        return;
    }

    m_pendingCompiledVersion = targetVer;

    QString tmpCfg = "/tmp/qslackpkg_kernel.config";
    QFile out(tmpCfg);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "No se pudo escribir la configuración temporal en /tmp.");
        return;
    }
    out.write(generateDotConfigContent().toUtf8());
    out.close();

    QString escapedDir = QString(m_activeSourceDir).replace("'", "'\\''");
    QString kernelFileName = QString("vmlinuz-custom-%1").arg(targetVer);
    QString initrdFileName = QString("initrd-%1.gz").arg(targetVer);

    QStringList cmds;
    cmds << "set -e";
    cmds << QString("cd '%1'").arg(escapedDir);
    cmds << "if [ -f .config ]; then cp -p .config .config.bak; fi";
    cmds << QString("install -m 644 '%1' .config && rm -f '%1'").arg(tmpCfg);
    cmds << "make olddefconfig";
    cmds << QString("echo '>>> [1/4] Compilando imagen bzImage con %1 hilos...'").arg(jobs);
    cmds << QString("make -j%1 bzImage").arg(jobs);
    cmds << QString("echo '>>> [2/4] Compilando módulos del kernel con %1 hilos...'").arg(jobs);
    cmds << QString("make -j%1 modules").arg(jobs);
    cmds << "echo '>>> [3/4] Instalando módulos en /lib/modules e imagen en /boot...'";
    cmds << "make modules_install";
    cmds << QString("cp -v arch/x86/boot/bzImage /boot/%1").arg(kernelFileName);
    cmds << QString("cp -v System.map /boot/System.map-custom-%1").arg(targetVer);
    cmds << QString("cp -v .config /boot/config-custom-%1").arg(targetVer);

    if (doInitrd) {
        cmds << QString("echo '>>> [4/4] Generando /boot/%1...'").arg(initrdFileName);
        cmds << QString("/sbin/mkinitrd -c -k '%1' -f '%2' -r '%3' -u -o '/boot/%4'")
                    .arg(targetVer, m_rootFs, m_rootDevice, initrdFileName);
    }

    if (doBoot) {
        if (m_bootloaderType == "LILO") {
            cmds << "echo '>>> Actualizando /etc/lilo.conf (preservando entradas anteriores) y ejecutando /sbin/lilo...'";
            cmds << buildLiloUpdateShellScript(kernelFileName, initrdFileName, doInitrd);
        } else if (m_bootloaderType == "ELILO") {
            cmds << QString("mkdir -p '%1'").arg(m_efiSlackwareDir);
            cmds << QString("cp -v '/boot/%1' '%2/'").arg(kernelFileName, m_efiSlackwareDir);
            if (doInitrd) {
                cmds << QString("cp -v '/boot/%1' '%2/'").arg(initrdFileName, m_efiSlackwareDir);
            }
            cmds << QString("if [ -f '%1' ]; then cp -v '%1' '%1.bak.$(date +%%Y%%m%%d%%H%%M%%S)'; fi")
                        .arg(m_bootloaderConfigPath);

            QString newStanza = QString("chooser=simple\\ndelay=1\\ntimeout=1\\n#\\ndefault=Slackware\\n#\\n"
                                        "image=%1\\n\\tlabel=Slackware\\n").arg(kernelFileName);
            if (doInitrd) {
                newStanza += QString("\\tinitrd=%1\\n").arg(initrdFileName);
            }
            newStanza += QString("\\tread-only\\n\\tappend=\"root=%1 vga=normal ro\"\\n").arg(m_rootDevice);

            if (m_keepFallbackEntry) {
                cmds << QString(
                    "PREV_STANZA=''; "
                    "if [ -f '%1' ]; then "
                    "  PREV_STANZA=$(awk '/^image=/{flag=1} flag{print}' '%1' | sed '0,/label=Slackware/s//label=Slackware-Prev/'); "
                    "fi; "
                    "printf '%2' > '%1'; "
                    "if [ -n \"$PREV_STANZA\" ]; then printf '\\n# Entrada anterior de respaldo\\n%%s\\n' \"$PREV_STANZA\" >> '%1'; fi"
                ).arg(m_bootloaderConfigPath, newStanza);
            } else {
                cmds << QString("printf '%2' > '%1'").arg(m_bootloaderConfigPath, newStanza);
            }
        } else if (m_bootloaderType == "GRUB") {
            cmds << "if [ -f /boot/grub/grub.cfg ]; then cp -v /boot/grub/grub.cfg /boot/grub/grub.cfg.bak.$(date +%Y%m%d%H%M%S); fi";
            cmds << "grub-mkconfig -o /boot/grub/grub.cfg";
        }
    }

    cmds << "echo '===================================================================='";
    cmds << "echo 'COMPILACIÓN E INSTALACIÓN DE KERNEL A MEDIDA COMPLETADA CON ÉXITO'";
    cmds << "echo '===================================================================='";

    runInternalPrivilegedTask(
        TaskCompileKernel,
        cmds.join(" && "),
        QString("Compilando e instalando Linux %1 (mantenga esta ventana abierta)...").arg(targetVer)
    );
}
