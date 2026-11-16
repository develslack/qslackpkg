#include "kernelconfigdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QRegularExpression>
#include <QProcess>
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
    setWindowTitle("Asistente Visual de Configuración y Compilación de Kernel (Kconfig / bzImage)");
    resize(1240, 790);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Banner de seguridad y privilegios
    QString privNote = (geteuid() == 0)
        ? "Sesión activa como <b>root (UID 0)</b>."
        : "Modo informativo: se solicitará contraseña de <b>root</b> al descargar en <code>/usr/src</code>, guardar <code>.config</code> o compilar/instalar.";

    QLabel *lblBanner = new QLabel(
        QString("<b>CONFIGURADOR VISUAL DE KERNEL A MEDIDA:</b> Herede los módulos activos de su sistema actual, "
                "personalice drivers y subsistemas sin usar ncurses, y compile su nueva imagen <code>bzImage</code> + <code>initrd</code>. %1")
            .arg(privNote),
        this
    );
    lblBanner->setWordWrap(true);
    lblBanner->setStyleSheet(
        "background-color: #e3f2fd; color: #0d47a1; border: 1px solid #90caf9; "
        "border-left: 5px solid #1565c0; border-radius: 4px; padding: 8px; font-size: 9.2pt;"
    );
    mainLayout->addWidget(lblBanner);

    // ========================================================================
    // PASO 1: FUENTES DEL KERNEL (LOCAL /USR/SRC O DESCARGA DESDE KERNEL.ORG)
    // ========================================================================
    QGroupBox *grpStep1 = new QGroupBox("Paso 1: Selección o Descarga de Código Fuente del Kernel", this);
    QVBoxLayout *step1Layout = new QVBoxLayout(grpStep1);

    QHBoxLayout *srcRow = new QHBoxLayout();
    m_comboSourceSelect = new QComboBox(grpStep1);
    m_comboSourceSelect->setMinimumWidth(420);

    m_btnFetchKernelOrg = new QPushButton("Consultar versiones en Kernel.org", grpStep1);
    m_btnFetchKernelOrg->setCursor(Qt::PointingHandCursor);

    m_btnDownloadSource = new QPushButton("Descargar y Extraer en /usr/src (root)", grpStep1);
    m_btnDownloadSource->setCursor(Qt::PointingHandCursor);
    m_btnDownloadSource->setEnabled(false);
    m_btnDownloadSource->setStyleSheet(
        "QPushButton { background-color: #1565c0; color: white; font-weight: bold; padding: 5px 12px; border-radius: 4px; }"
        "QPushButton:hover:enabled { background-color: #0d47a1; }"
        "QPushButton:disabled { background-color: #eceff1; color: #90a4ae; }"
    );

    srcRow->addWidget(new QLabel("<b>Árbol / Versión:</b>", grpStep1));
    srcRow->addWidget(m_comboSourceSelect, 1);
    srcRow->addWidget(m_btnFetchKernelOrg);
    srcRow->addWidget(m_btnDownloadSource);
    step1Layout->addLayout(srcRow);

    m_lblSourceStatus = new QLabel("Escaneando fuentes locales en /usr/src...", grpStep1);
    m_lblSourceStatus->setStyleSheet("color: #37474f; font-size: 9pt;");
    step1Layout->addWidget(m_lblSourceStatus);
    mainLayout->addWidget(grpStep1);

    // ========================================================================
    // PASO 2: HERENCIA INTELIGENTE DE CONFIGURACIÓN ACTUAL
    // ========================================================================
    QGroupBox *grpStep2 = new QGroupBox("Paso 2: Base de Configuración (Heredar Módulos del Sistema Actual)", this);
    QHBoxLayout *step2Layout = new QHBoxLayout(grpStep2);

    m_btnInheritFullConfig = new QPushButton("1. Heredar configuración actual (/proc/config.gz + olddefconfig)", grpStep2);
    m_btnInheritFullConfig->setCursor(Qt::PointingHandCursor);
    m_btnInheritFullConfig->setToolTip(
        "Importa todos los drivers y módulos del kernel en ejecución actual y adapta las nuevas opciones con valores seguros."
    );
    m_btnInheritFullConfig->setStyleSheet(
        "QPushButton { background-color: #e8f5e9; color: #1b5e20; border: 1px solid #81c784; "
        "font-weight: bold; padding: 6px 12px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #c8e6c9; }"
    );

    m_btnInheritLocalModConfig = new QPushButton("2. Optimizar solo para mi hardware (localmodconfig)", grpStep2);
    m_btnInheritLocalModConfig->setCursor(Qt::PointingHandCursor);
    m_btnInheritLocalModConfig->setToolTip(
        "Inspecciona los módulos cargados actualmente (lsmod) y desactiva los miles de drivers de hardware que este equipo NO tiene.\n"
        "Reduce el tiempo de compilación de ~45 minutos a ~6 minutos."
    );
    m_btnInheritLocalModConfig->setStyleSheet(
        "QPushButton { background-color: #fff3e0; color: #e65100; border: 1px solid #ffb74d; "
        "font-weight: bold; padding: 6px 12px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #ffe0b2; }"
    );

    m_lblConfigStatus = new QLabel("Sin configuración cargada aún.", grpStep2);
    m_lblConfigStatus->setStyleSheet("color: #37474f; font-weight: bold; padding-left: 8px;");

    step2Layout->addWidget(m_btnInheritFullConfig);
    step2Layout->addWidget(m_btnInheritLocalModConfig);
    step2Layout->addWidget(m_lblConfigStatus, 1);
    mainLayout->addWidget(grpStep2);

    // ========================================================================
    // PASO 3: CONFIGURADOR VISUAL DE KERNEL (ÁRBOL KCONFIG + INSPECTOR)
    // ========================================================================
    QGroupBox *grpStep3 = new QGroupBox("Paso 3: Personalización Visual del Kernel (Kconfig Interactivo)", this);
    QVBoxLayout *step3Layout = new QVBoxLayout(grpStep3);

    QHBoxLayout *filterBar = new QHBoxLayout();
    m_editKconfigSearch = new QLineEdit(grpStep3);
    m_editKconfigSearch->setPlaceholderText("Buscar driver, sistema de archivos o símbolo CONFIG_ (ej: nvme, amdgpu, btrfs, wireguard, preempt)...");
    m_editKconfigSearch->setClearButtonEnabled(true);

    m_chkShowEnabledOnly = new QCheckBox("Solo opciones activas ([*] o [M])", grpStep3);
    m_lblOptionsCounter = new QLabel("0 opciones cargadas", grpStep3);
    m_lblOptionsCounter->setStyleSheet("color: #37474f; font-weight: bold;");

    filterBar->addWidget(new QLabel("<b>Buscar opción:</b>", grpStep3));
    filterBar->addWidget(m_editKconfigSearch, 1);
    filterBar->addWidget(m_chkShowEnabledOnly);
    filterBar->addSpacing(8);
    filterBar->addWidget(m_lblOptionsCounter);
    step3Layout->addLayout(filterBar);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, grpStep3);

    // Panel Izquierdo: Árbol de categorías y opciones Kconfig
    m_kconfigTree = new QTreeWidget(splitter);
    m_kconfigTree->setColumnCount(3);
    m_kconfigTree->setHeaderLabels({"Subsistema / Opción del Kernel", "Estado", "Símbolo CONFIG_"});
    m_kconfigTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_kconfigTree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_kconfigTree->header()->setSectionResizeMode(2, QHeaderView::Interactive);
    m_kconfigTree->setColumnWidth(1, 115);
    m_kconfigTree->setColumnWidth(2, 220);
    m_kconfigTree->setAlternatingRowColors(true);

    // Panel Derecho: Editor de estado y ayuda oficial del kernel
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
    stateEditRow->addWidget(new QLabel("<b>Configuración:</b>", rightPane));

    m_comboOptTriState = new QComboBox(rightPane);
    m_comboOptTriState->addItem("[   ] Deshabilitado (is not set)", "n");
    m_comboOptTriState->addItem("[ * ] Integrado en el Kernel (=y)", "y");
    m_comboOptTriState->addItem("[ M ] Compilar como Módulo (=m)", "m");
    m_comboOptTriState->setEnabled(false);

    m_editOptValue = new QLineEdit(rightPane);
    m_editOptValue->setVisible(false);

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

    mainLayout->addWidget(grpStep3, 1);

    // ========================================================================
    // PASO 4: COMPILACIÓN DE BZIMAGE, INITRD E INSTALACIÓN EN BOOTLOADER
    // ========================================================================
    QGroupBox *grpStep4 = new QGroupBox("Paso 4: Compilación de bzImage, Generación de initrd e Instalación en Bootloader", this);
    QHBoxLayout *step4Layout = new QHBoxLayout(grpStep4);

    int maxCores = QThread::idealThreadCount();
    if (maxCores < 1) maxCores = 2;

    m_spinJobs = new QSpinBox(grpStep4);
    m_spinJobs->setRange(1, maxCores * 2);
    m_spinJobs->setValue(maxCores);
    m_spinJobs->setToolTip("Cantidad de hilos paralelos para make -jN");

    m_chkBuildInitrdAfter = new QCheckBox("Generar /boot/initrd-<ver>.gz automáticamente", grpStep4);
    m_chkBuildInitrdAfter->setChecked(true);

    m_chkUpdateBootAfter = new QCheckBox(
        QString("Actualizar %1 (conservando Slackware-Prev)").arg(m_bootloaderType),
        grpStep4
    );
    m_chkUpdateBootAfter->setChecked(m_bootloaderType != "No detectado");

    m_btnSaveConfigOnly = new QPushButton("Guardar .config (root)", grpStep4);
    m_btnSaveConfigOnly->setCursor(Qt::PointingHandCursor);

    m_btnCompileAndInstall = new QPushButton("Compilar bzImage e Instalar Kernel (root)", grpStep4);
    m_btnCompileAndInstall->setCursor(Qt::PointingHandCursor);
    m_btnCompileAndInstall->setStyleSheet(
        "QPushButton { background-color: #2e7d32; color: white; font-weight: bold; "
        "padding: 7px 16px; border-radius: 4px; font-size: 9.8pt; }"
        "QPushButton:hover { background-color: #1b5e20; }"
    );

    QPushButton *btnClose = new QPushButton("Cerrar", grpStep4);

    step4Layout->addWidget(new QLabel("<b>Hilos CPU (-j):</b>", grpStep4));
    step4Layout->addWidget(m_spinJobs);
    step4Layout->addSpacing(8);
    step4Layout->addWidget(m_chkBuildInitrdAfter);
    step4Layout->addWidget(m_chkUpdateBootAfter);
    step4Layout->addStretch();
    step4Layout->addWidget(m_btnSaveConfigOnly);
    step4Layout->addWidget(m_btnCompileAndInstall);
    step4Layout->addWidget(btnClose);

    mainLayout->addWidget(grpStep4);

    // Conexiones
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
    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);

    // Carga inicial
    scanLocalKernelSources();
    fetchKernelOrgReleases();
}

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
        m_btnInheritLocalModConfig->setEnabled(true);

        if (QFile::exists(m_activeSourceDir + "/.config")) {
            m_lblConfigStatus->setText("Configuración existente detectada en " + m_activeSourceDir + "/.config");
            loadCurrentDotConfigValues(m_activeSourceDir + "/.config");
        } else {
            m_lblConfigStatus->setText("Sin .config en este árbol. Use el Paso 2 para heredar su configuración actual.");
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

    emit requestPrivilegedExecution(
        "/bin/sh", {"-c", cmd},
        QString("Descargando y extrayendo Linux %1 en /usr/src como root...").arg(selectedRel.version)
    );
    accept();
}

void KernelConfigDialog::inheritCurrentKernelConfig(bool runLocalModConfig) {
    if (m_activeSourceDir.isEmpty() || !QDir(m_activeSourceDir).exists()) {
        QMessageBox::warning(this, "Sin árbol de fuentes", "Seleccione primero un directorio válido en /usr/src.");
        return;
    }

    QString targetMode = runLocalModConfig ? "localmodconfig" : "olddefconfig";
    QString modeDesc = runLocalModConfig
        ? "Heredará su configuración actual y desactivará todos los drivers de hardware que NO estén cargados actualmente (lsmod)."
        : "Heredará exactamente todos los módulos y opciones de su kernel actual, actualizando símbolos nuevos con valores por defecto.";

    QMessageBox::StandardButton resp = QMessageBox::question(
        this,
        QString("Generar .config base (%1)").arg(targetMode),
        QString("Árbol destino: %1\n\n%2\n\n"
                "Esta operación ejecutará 'make %3' como root para preparar el archivo .config.\n"
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

    emit requestPrivilegedExecution(
        "/bin/sh", {"-c", cmd},
        QString("Generando configuración base (%1) en %2 como root...").arg(targetMode, m_activeSourceDir)
    );
    accept();
}

void KernelConfigDialog::loadCurrentDotConfigValues(const QString &dotConfigPath) {
    m_dotConfigValues.clear();

    QByteArray content;
    if (dotConfigPath.endsWith(".gz")) {
        QProcess zcat;
        zcat.start("zcat", {dotConfigPath});
        if (zcat.waitForFinished(3000)) {
            content = zcat.readAllStandardOutput();
        }
    } else {
        QFile file(dotConfigPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            content = file.readAll();
            file.close();
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
    if (!current) return;

    QString symbol = current->data(0, Qt::UserRole).toString();
    if (symbol.isEmpty() || !m_kconfigOptions.contains(symbol)) {
        m_lblOptTitle->setText(current->text(0));
        m_lblOptSymbol->setText("Categoría / Submenú del Kernel");
        m_lblOptDepends->setText("Expanda este nodo para configurar sus drivers y subsistemas.");
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
        m_editOptValue->setText(opt.value);
    } else {
        m_editOptValue->setVisible(false);
        m_comboOptTriState->setVisible(true);
        m_comboOptTriState->setEnabled(true);

        // Si es bool puro, deshabilitar la opción [M] Módulo
        QStandardItemModel *model = qobject_cast<QStandardItemModel*>(m_comboOptTriState->model());
        if (model && model->item(2)) {
            model->item(2)->setEnabled(opt.type != "bool");
        }

        int idx = m_comboOptTriState->findData(opt.value.isEmpty() ? "n" : opt.value);
        if (idx < 0) idx = 0;
        m_comboOptTriState->setCurrentIndex(idx);
    }

    m_txtOptHelp->setPlainText(
        opt.helpText.trimmed().isEmpty()
            ? "Sin descripción extendida disponible para este símbolo en Kconfig."
            : opt.helpText.trimmed()
    );

    m_updatingUi = false;
}

void KernelConfigDialog::onOptionStateChanged() {
    if (m_updatingUi) return;
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
        // Es un submenú puro: mostrarlo si coincide su título o si tiene hijos visibles
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
    lines << "# Automatically generated by QSlackpkg Visual Kernel Configurator";
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

    emit requestPrivilegedExecution(
        "/bin/sh", {"-c", cmd},
        QString("Guardando .config personalizado en %1 como root...").arg(m_activeSourceDir)
    );
    accept();
}

void KernelConfigDialog::compileAndInstallCustomKernel() {
    if (m_activeSourceDir.isEmpty()) return;

    QString targetVer = readKernelVersionFromMakefile(m_activeSourceDir);
    if (targetVer.isEmpty()) targetVer = "custom";

    int jobs = m_spinJobs->value();
    bool doInitrd = m_chkBuildInitrdAfter->isChecked();
    bool doBoot = m_chkUpdateBootAfter->isChecked();

    QMessageBox::StandardButton resp = QMessageBox::warning(
        this,
        "Confirmación Obligatoria de Usuario ROOT - Compilar e Instalar Kernel",
        QString("Se iniciará como usuario <b>root</b> el proceso completo de compilación e instalación:\n\n"
                "• Árbol fuente: %1 (Versión %2)\n"
                "• Hilos de compilación: %3 núcleos\n"
                "• Salida bzImage: /boot/vmlinuz-custom-%2\n"
                "• Módulos: /lib/modules/%2\n"
                "• Generar initrd: %4\n"
                "• Actualizar Bootloader (%5): %6 (con respaldo .bak y Slackware-Prev)\n\n"
                "¿Desea iniciar la compilación e ingresar la contraseña de root?")
            .arg(m_activeSourceDir, targetVer)
            .arg(jobs)
            .arg(doInitrd ? QString("Sí (/boot/initrd-%1.gz)").arg(targetVer) : "No")
            .arg(m_bootloaderType)
            .arg(doBoot ? "Sí" : "No"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    if (resp != QMessageBox::Yes) return;

    QString tmpCfg = "/tmp/qslackpkg_kernel.config";
    QFile out(tmpCfg);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) return;
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
        if (m_bootloaderType == "ELILO") {
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
        } else if (m_bootloaderType == "LILO") {
            cmds << "cp -v /etc/lilo.conf /etc/lilo.conf.bak.$(date +%Y%m%d%H%M%S)";
            QString liloStanza = QString("image = /boot/%1\\n  root = %2\\n  label = Slackware\\n")
                                     .arg(kernelFileName, m_rootDevice);
            if (doInitrd) {
                liloStanza += QString("  initrd = /boot/%1\\n").arg(initrdFileName);
            }
            liloStanza += "  read-only\\n";

            if (m_keepFallbackEntry) {
                cmds << QString(
                    "HEADER=$(awk '/^image[[:space:]]*=/{exit} {print}' /etc/lilo.conf); "
                    "OLD_STANZAS=$(awk '/^image[[:space:]]*=/{flag=1} flag{print}' /etc/lilo.conf | sed '0,/label[[:space:]]*=[[:space:]]*Slackware/s//label = Slackware-Prev/'); "
                    "printf '%%s\\n\\n%1\\n%%s\\n' \"$HEADER\" \"$OLD_STANZAS\" > /etc/lilo.conf"
                ).arg(liloStanza);
            } else {
                cmds << QString(
                    "HEADER=$(awk '/^image[[:space:]]*=/{exit} {print}' /etc/lilo.conf); "
                    "printf '%%s\\n\\n%1\\n' \"$HEADER\" > /etc/lilo.conf"
                ).arg(liloStanza);
            }
            cmds << "/sbin/lilo -v";
        } else if (m_bootloaderType == "GRUB") {
            cmds << "if [ -f /boot/grub/grub.cfg ]; then cp -v /boot/grub/grub.cfg /boot/grub/grub.cfg.bak.$(date +%Y%m%d%H%M%S); fi";
            cmds << "grub-mkconfig -o /boot/grub/grub.cfg";
        }
    }

    cmds << "echo '===================================================================='";
    cmds << "echo 'COMPILACIÓN E INSTALACIÓN DE KERNEL A MEDIDA COMPLETADA CON ÉXITO'";
    cmds << "echo '===================================================================='";

    emit requestPrivilegedExecution(
        "/bin/sh", {"-c", cmds.join(" && ")},
        QString("Compilando e instalando kernel a medida %1 como root...").arg(targetVer)
    );
    accept();
}
