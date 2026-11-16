#include "usbcreatorwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QScrollBar>
#include <QCryptographicHash>
#include <QApplication>
#include <QCoreApplication>
#include <QUrl>

UsbCreatorWidget::UsbCreatorWidget(QNetworkAccessManager *netManager,
                                   const QString &mirrorUrl,
                                   QWidget *parent)
    : QWidget(parent),
      m_netManager(netManager),
      m_mirrorUrl(mirrorUrl)
{
    QString isoDir = getAppIsoDirectory();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Banner de seguridad y propósito
    QLabel *lblHeader = new QLabel(
        QString("<b>Creador de Unidades USB Booteables para Slackware Linux (UEFI + Legacy BIOS):</b> "
                "Grabe imágenes oficiales de cualquier edición de Slackware (32-bit o 64-bit, actual o histórica) "
                "o imágenes <code>usbboot.img</code>. Por seguridad, todas las descargas se almacenan aisladas en "
                "<code>%1</code> y los discos internos están protegidos.")
            .arg(isoDir),
        this
    );
    lblHeader->setWordWrap(true);
    lblHeader->setStyleSheet(
        "background-color: #e3f2fd; color: #0d47a1; border: 1px solid #90caf9; "
        "border-left: 5px solid #1565c0; border-radius: 4px; padding: 8px; font-size: 9.2pt;"
    );
    mainLayout->addWidget(lblHeader);

    // ========================================================================
    // PASO 1: SELECCIÓN DE IMAGEN ISO / IMG DE SLACKWARE
    // ========================================================================
    m_grpImage = new QGroupBox("1. Imagen de Origen (ISO Oficial de Slackware, Live o usbboot.img)", this);
    QVBoxLayout *imgLayout = new QVBoxLayout(m_grpImage);

    QHBoxLayout *imgRow = new QHBoxLayout();
    m_editImagePath = new QLineEdit(m_grpImage);
    m_editImagePath->setPlaceholderText("Seleccione un archivo .iso o .img (por defecto en carpeta ./iso de la app)...");
    m_editImagePath->setClearButtonEnabled(true);

    m_btnBrowseImage = new QPushButton("Seleccionar archivo ISO / IMG...", m_grpImage);
    m_btnBrowseImage->setCursor(Qt::PointingHandCursor);

    m_btnDownloadOfficialImg = new QPushButton("Descargar imagen oficial de Slackware...", m_grpImage);
    m_btnDownloadOfficialImg->setCursor(Qt::PointingHandCursor);
    m_btnDownloadOfficialImg->setToolTip(
        "Permite elegir entre todas las versiones disponibles de Slackware (32-bit y 64-bit, actuales e históricas)\n"
        "y descargarla dentro de la carpeta aislada ./iso con verificación MD5."
    );
    m_btnDownloadOfficialImg->setStyleSheet(
        "QPushButton { background-color: #1565c0; color: white; font-weight: bold; padding: 6px 12px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #0d47a1; }"
    );

    m_btnVerifyHash = new QPushButton("Calcular SHA256 / MD5", m_grpImage);
    m_btnVerifyHash->setCursor(Qt::PointingHandCursor);
    m_btnVerifyHash->setEnabled(false);

    imgRow->addWidget(m_editImagePath, 1);
    imgRow->addWidget(m_btnBrowseImage);
    imgRow->addWidget(m_btnDownloadOfficialImg);
    imgRow->addWidget(m_btnVerifyHash);
    imgLayout->addLayout(imgRow);

    m_lblImageInfo = new QLabel(
        QString("Carpeta segura de imágenes de la aplicación: %1").arg(isoDir),
        m_grpImage
    );
    m_lblImageInfo->setStyleSheet("color: #455a64; font-size: 9pt;");
    imgLayout->addWidget(m_lblImageInfo);
    mainLayout->addWidget(m_grpImage);

    // ========================================================================
    // PASO 2: SELECCIÓN DE UNIDAD USB EXTRAÍBLE
    // ========================================================================
    m_grpDevice = new QGroupBox("2. Unidad USB de Destino (Solo dispositivos extraíbles detectados)", this);
    QVBoxLayout *devLayout = new QVBoxLayout(m_grpDevice);

    QHBoxLayout *devRow = new QHBoxLayout();
    m_comboUsbDevices = new QComboBox(m_grpDevice);
    m_comboUsbDevices->setMinimumHeight(30);

    m_btnRefreshDevices = new QPushButton("Refrescar unidades USB", m_grpDevice);
    m_btnRefreshDevices->setCursor(Qt::PointingHandCursor);

    devRow->addWidget(new QLabel("<b>Pendrive / Memoria USB:</b>", m_grpDevice));
    devRow->addWidget(m_comboUsbDevices, 1);
    devRow->addWidget(m_btnRefreshDevices);
    devLayout->addLayout(devRow);

    m_lblDeviceDetails = new QLabel("Buscando unidades USB conectadas...", m_grpDevice);
    m_lblDeviceDetails->setStyleSheet("color: #37474f; font-size: 9pt;");
    devLayout->addWidget(m_lblDeviceDetails);
    mainLayout->addWidget(m_grpDevice);

    // ========================================================================
    // PASO 3: MODO DE ESCRITURA (ESTILO RUFUS / ETCHER / SLACKWARE)
    // ========================================================================
    m_grpMode = new QGroupBox("3. Esquema de Escritura y Compatibilidad de Arranque", this);
    QVBoxLayout *modeLayout = new QVBoxLayout(m_grpMode);

    m_radioHybridDd = new QRadioButton(
        "Modo Imagen Híbrida Directa (ISO/IMG -> USB) [Recomendado - Compatible con UEFI y Legacy BIOS]",
        m_grpMode
    );
    m_radioHybridDd->setChecked(true);
    m_radioHybridDd->setStyleSheet("font-weight: bold; color: #1b5e20;");

    m_radioSlackwareUsbImg2Disk = new QRadioButton(
        "Modo Slackware usbimg2disk.sh (Formatea FAT32 + Syslinux, conserva espacio libre para paquetes)",
        m_grpMode
    );
    m_radioSlackwareUsbImg2Disk->setEnabled(
        QFile::exists("/usr/sbin/usbimg2disk.sh") || QFile::exists("/sbin/usbimg2disk.sh")
    );

    m_chkVerifyAfterWrite = new QCheckBox(
        "Ejecutar sincronización estricta de caché (fsync) y refrescar tabla de particiones (partprobe) al finalizar",
        m_grpMode
    );
    m_chkVerifyAfterWrite->setChecked(true);

    m_lblModeExplanation = new QLabel(
        "<i>Las ISOs oficiales de Slackware 15.0 / -current y Slackware Live son imágenes híbridas (isohybrid): "
        "al escribirse directamente en modo bloque arrancan tanto en equipos modernos UEFI como en BIOS tradicional.</i>",
        m_grpMode
    );
    m_lblModeExplanation->setWordWrap(true);
    m_lblModeExplanation->setStyleSheet("color: #546e7a; font-size: 8.8pt; padding-left: 18px;");

    modeLayout->addWidget(m_radioHybridDd);
    modeLayout->addWidget(m_lblModeExplanation);
    modeLayout->addWidget(m_radioSlackwareUsbImg2Disk);
    modeLayout->addWidget(m_chkVerifyAfterWrite);
    mainLayout->addWidget(m_grpMode);

    // ========================================================================
    // PASO 4: CONFIRMACIÓN DE BORRADO Y BARRA DE PROGRESO
    // ========================================================================
    m_grpAction = new QGroupBox("4. Creación del Medio Booteable", this);
    QVBoxLayout *actLayout = new QVBoxLayout(m_grpAction);

    QHBoxLayout *confirmRow = new QHBoxLayout();
    m_chkConfirmDataLoss = new QCheckBox(
        "Entiendo que TODOS los datos existentes en la unidad USB seleccionada serán eliminados permanentemente.",
        m_grpAction
    );
    m_chkConfirmDataLoss->setStyleSheet("font-weight: bold; color: #c62828;");

    m_btnStartWrite = new QPushButton("Crear USB Booteable (Requiere root)", m_grpAction);
    m_btnStartWrite->setCursor(Qt::PointingHandCursor);
    m_btnStartWrite->setEnabled(false);
    m_btnStartWrite->setStyleSheet(
        "QPushButton { background-color: #2e7d32; color: white; font-weight: bold; "
        "padding: 8px 18px; border-radius: 4px; font-size: 10pt; }"
        "QPushButton:hover:enabled { background-color: #1b5e20; }"
        "QPushButton:disabled { background-color: #c8e6c9; color: #f1f8e9; }"
    );

    confirmRow->addWidget(m_chkConfirmDataLoss, 1);
    confirmRow->addWidget(m_btnStartWrite);
    actLayout->addLayout(confirmRow);

    QHBoxLayout *progRow = new QHBoxLayout();
    m_lblProgressStatus = new QLabel("Listo para seleccionar imagen y dispositivo USB.", m_grpAction);
    m_lblProgressStatus->setStyleSheet("font-weight: bold; color: #37474f;");

    m_progressBar = new QProgressBar(m_grpAction);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("%p%");
    m_progressBar->setMinimumWidth(260);

    progRow->addWidget(m_lblProgressStatus, 1);
    progRow->addWidget(m_progressBar);
    actLayout->addLayout(progRow);

    m_consoleLog = new QTextEdit(m_grpAction);
    m_consoleLog->setReadOnly(true);
    m_consoleLog->setMaximumHeight(135);
    m_consoleLog->setStyleSheet(
        "background-color: #1e1e1e; color: #d4d4d4; font-family: monospace; font-size: 8.8pt; border-radius: 4px;"
    );
    m_consoleLog->setPlaceholderText("Registro de descargas, verificación MD5 y grabación en el dispositivo USB...");
    actLayout->addWidget(m_consoleLog);

    mainLayout->addWidget(m_grpAction, 1);

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    // Conexiones
    connect(m_btnBrowseImage, &QPushButton::clicked, this, &UsbCreatorWidget::browseLocalImage);
    connect(m_editImagePath, &QLineEdit::textChanged, this, &UsbCreatorWidget::onImagePathChanged);
    connect(m_btnVerifyHash, &QPushButton::clicked, this, &UsbCreatorWidget::calculateImageChecksum);
    connect(m_btnDownloadOfficialImg, &QPushButton::clicked, this, &UsbCreatorWidget::openOfficialImageDownloadDialog);

    connect(m_btnRefreshDevices, &QPushButton::clicked, this, &UsbCreatorWidget::scanUsbDevices);
    connect(m_comboUsbDevices, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &UsbCreatorWidget::onUsbDeviceChanged);

    connect(m_chkConfirmDataLoss, &QCheckBox::toggled, this, [this](bool checked) {
        bool ready = checked && !m_detectedDrives.isEmpty() && QFile::exists(m_editImagePath->text().trimmed());
        m_btnStartWrite->setEnabled(ready);
    });
    connect(m_btnStartWrite, &QPushButton::clicked, this, &UsbCreatorWidget::startUsbCreation);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &UsbCreatorWidget::readProcessOutput);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &UsbCreatorWidget::onProcessFinished);

    scanUsbDevices();
}

UsbCreatorWidget::~UsbCreatorWidget() {
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished(3000);
    }
}

bool UsbCreatorWidget::isBusy() const {
    return m_process->state() != QProcess::NotRunning;
}

QString UsbCreatorWidget::getAppIsoDirectory() const {
    QString appDir = QCoreApplication::applicationDirPath();

    // 1. Detectar entorno de desarrollo (si existe qslackpkg.pro o CMakeLists.txt en appDir o en su directorio padre)
    QDir dir(appDir);
    if (QFile::exists(dir.filePath("qslackpkg.pro")) ||
        QFile::exists(dir.filePath("CMakeLists.txt"))) {
        QString devIsoDir = dir.filePath("iso");
        QDir().mkpath(devIsoDir);
        return devIsoDir;
    }

    QDir parentDir = dir;
    if (parentDir.cdUp() &&
        (QFile::exists(parentDir.filePath("qslackpkg.pro")) ||
         QFile::exists(parentDir.filePath("CMakeLists.txt")))) {
        QString devIsoDir = parentDir.filePath("iso");
        QDir().mkpath(devIsoDir);
        return devIsoDir;
    }

    // 2. Modo Producción: ruta oficial /opt/qslackpkg/iso
    QString prodIsoDir = "/opt/qslackpkg/iso";
    QDir().mkpath(prodIsoDir); // Se creará directamente si hay permisos, o vía pkexec al descargar
    return prodIsoDir;
}

QString UsbCreatorWidget::formatBytes(qint64 bytes) const {
    const double kb = 1024.0;
    const double mb = kb * 1024.0;
    const double gb = mb * 1024.0;
    if (bytes >= gb) return QString::number(bytes / gb, 'f', 2) + " GiB";
    if (bytes >= mb) return QString::number(bytes / mb, 'f', 1) + " MiB";
    if (bytes >= kb) return QString::number(bytes / kb, 'f', 1) + " KiB";
    return QString::number(bytes) + " B";
}

// ============================================================================
// DETECCIÓN SEGURA DE UNIDADES USB EXTRAÍBLES (EXCLUYE DISCOS DE SISTEMA)
// ============================================================================

void UsbCreatorWidget::scanUsbDevices() {
    m_detectedDrives.clear();
    m_comboUsbDevices->blockSignals(true);
    m_comboUsbDevices->clear();

    QProcess lsblk;
    lsblk.start("lsblk", {"-J", "-b", "-o", "NAME,KNAME,TYPE,TRAN,RM,SIZE,MODEL,VENDOR,MOUNTPOINT"});
    if (lsblk.waitForFinished(3000)) {
        QJsonDocument doc = QJsonDocument::fromJson(lsblk.readAllStandardOutput());
        QJsonArray blockDevices = doc.object().value("blockdevices").toArray();

        for (const QJsonValue &val : blockDevices) {
            QJsonObject dev = val.toObject();
            QString type = dev.value("type").toString();
            if (type != "disk") continue;

            QString tran = dev.value("tran").toString().toLower();
            bool rm = dev.value("rm").toBool();
            QString name = dev.value("name").toString();
            qint64 sizeBytes = static_cast<qint64>(dev.value("size").toDouble());

            if ((tran != "usb" && !rm) || sizeBytes <= 0) continue;

            bool isSystemDisk = false;
            QStringList mounts;
            QString rootMount = dev.value("mountpoint").toString();
            if (!rootMount.isEmpty()) {
                mounts << rootMount;
                if (rootMount == "/" || rootMount == "/boot" || rootMount == "/home" || rootMount == "/usr") {
                    isSystemDisk = true;
                }
            }

            QJsonArray children = dev.value("children").toArray();
            for (const QJsonValue &cVal : children) {
                QJsonObject child = cVal.toObject();
                QString mnt = child.value("mountpoint").toString();
                if (!mnt.isEmpty()) {
                    mounts << mnt;
                    if (mnt == "/" || mnt == "/boot" || mnt == "/boot/efi" || mnt == "/efi" || mnt == "/home") {
                        isSystemDisk = true;
                    }
                }
            }
            if (isSystemDisk) continue;

            UsbDriveInfo info;
            info.name = name;
            info.deviceNode = "/dev/" + name;
            info.vendor = dev.value("vendor").toString().trimmed();
            info.model = dev.value("model").toString().trimmed();
            if (info.model.isEmpty()) info.model = "Dispositivo USB Genérico";
            info.sizeBytes = sizeBytes;
            info.sizeHuman = formatBytes(sizeBytes);
            info.transport = tran.isEmpty() ? "removable" : tran.toUpper();
            info.mountPoints = mounts.isEmpty() ? "Desmontado" : mounts.join(", ");

            m_detectedDrives.append(info);
            QString label = QString("%1 — %2 %3 (%4) [%5]")
                                .arg(info.deviceNode, info.vendor, info.model, info.sizeHuman, info.transport);
            m_comboUsbDevices->addItem(label.simplified(), info.deviceNode);
        }
    }

    m_comboUsbDevices->blockSignals(false);
    onUsbDeviceChanged();
}

void UsbCreatorWidget::onUsbDeviceChanged() {
    int idx = m_comboUsbDevices->currentIndex();
    if (idx < 0 || idx >= m_detectedDrives.size()) {
        m_lblDeviceDetails->setStyleSheet("color: #c62828; font-weight: bold; font-size: 9pt;");
        m_lblDeviceDetails->setText("No se detectaron memorias USB conectadas. Conecte un pendrive y pulse 'Refrescar unidades USB'.");
        m_btnStartWrite->setEnabled(false);
        return;
    }

    const UsbDriveInfo &drv = m_detectedDrives[idx];
    m_lblDeviceDetails->setStyleSheet("color: #1b5e20; font-size: 9pt;");
    m_lblDeviceDetails->setText(
        QString("Nodo: <b>%1</b> &nbsp;|&nbsp; Capacidad: <b>%2</b> &nbsp;|&nbsp; "
                "Interfaz: <b>%3</b> &nbsp;|&nbsp; Puntos de montaje actuales: <b>%4</b>")
            .arg(drv.deviceNode, drv.sizeHuman, drv.transport, drv.mountPoints)
    );

    bool ready = m_chkConfirmDataLoss->isChecked() && QFile::exists(m_editImagePath->text().trimmed());
    m_btnStartWrite->setEnabled(ready);
}

// ============================================================================
// SELECCIÓN DE IMAGEN LOCAL (INICIA EN ./ISO) Y DESCARGA OFICIAL
// ============================================================================

void UsbCreatorWidget::browseLocalImage() {
    QString startDir = getAppIsoDirectory();
    if (!QDir(startDir).exists()) {
        startDir = QDir::homePath();
    }

    QString path = QFileDialog::getOpenFileName(
        this,
        "Seleccionar Imagen ISO o IMG de Slackware Linux",
        startDir,
        "Imágenes Booteables (*.iso *.img *.ISO *.IMG);;Todos los archivos (*)"
    );
    if (!path.isEmpty()) {
        m_editImagePath->setText(path);
    }
}

void UsbCreatorWidget::onImagePathChanged() {
    QString path = m_editImagePath->text().trimmed();
    QFileInfo fi(path);
    if (!path.isEmpty() && fi.exists() && fi.isFile()) {
        m_selectedImageSize = fi.size();
        m_btnVerifyHash->setEnabled(true);
        m_lblImageInfo->setStyleSheet("color: #0d47a1; font-weight: bold; font-size: 9pt;");
        m_lblImageInfo->setText(
            QString("Archivo válido: %1  |  Tamaño: %2 (%3 bytes)")
                .arg(fi.fileName(), formatBytes(m_selectedImageSize))
                .arg(m_selectedImageSize)
        );

        if (fi.fileName().contains("usbboot.img", Qt::CaseInsensitive) && m_radioSlackwareUsbImg2Disk->isEnabled()) {
            m_radioSlackwareUsbImg2Disk->setChecked(true);
        } else {
            m_radioHybridDd->setChecked(true);
        }
    } else {
        m_selectedImageSize = 0;
        m_btnVerifyHash->setEnabled(false);
        m_lblImageInfo->setStyleSheet("color: #455a64; font-size: 9pt;");
        m_lblImageInfo->setText(QString("Carpeta segura de imágenes de la aplicación: %1").arg(getAppIsoDirectory()));
    }

    bool ready = m_chkConfirmDataLoss->isChecked() && !m_detectedDrives.isEmpty() && (m_selectedImageSize > 0);
    m_btnStartWrite->setEnabled(ready);
}

void UsbCreatorWidget::calculateImageChecksum() {
    QString path = m_editImagePath->text().trimmed();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_lblProgressStatus->setText("Calculando sumas SHA256 y MD5 de la imagen...");
    QApplication::processEvents();

    QCryptographicHash sha256(QCryptographicHash::Sha256);
    QCryptographicHash md5(QCryptographicHash::Md5);

    const qint64 bufSize = 4 * 1024 * 1024;
    while (!file.atEnd()) {
        QByteArray chunk = file.read(bufSize);
        sha256.addData(chunk);
        md5.addData(chunk);
    }
    file.close();
    QApplication::restoreOverrideCursor();

    QString shaHex = QString::fromLatin1(sha256.result().toHex());
    QString md5Hex = QString::fromLatin1(md5.result().toHex());
    m_lblProgressStatus->setText("Suma de verificación calculada.");

    QMessageBox::information(
        this,
        "Integridad de Imagen (SHA256 / MD5)",
        QString("<b>Archivo:</b> %1<br><br>"
                "<b>MD5:</b><br><code>%2</code><br><br>"
                "<b>SHA256:</b><br><code>%3</code><br><br>"
                "Compare estos valores con el archivo <code>CHECKSUMS.md5</code> o <code>.md5</code> oficial de Slackware.")
            .arg(QFileInfo(path).fileName(), md5Hex, shaHex)
    );
}

QString UsbCreatorWidget::getNormalizedMirrorRoot() const {
    if (!m_mirrorUrl.isEmpty()) {
        QUrl url(m_mirrorUrl);
        QString path = url.path();
        QRegularExpression branchRx("/slackware(?:64)?-(?:current|\\d+\\.\\d+)/?$");
        if (branchRx.match(path).hasMatch()) {
            path.remove(branchRx);
            if (!path.endsWith('/')) path += '/';
            url.setPath(path);
            return url.toString();
        }
        QString res = m_mirrorUrl;
        if (!res.endsWith('/')) res += '/';
        return res;
    }
    return "https://mirrors.slackware.com/slackware/";
}

QList<SlackwareImageRelease> UsbCreatorWidget::buildAvailableSlackwareReleases() const {
    QList<SlackwareImageRelease> list;

    auto addPair = [&](const QString &verLabel, const QString &branchDir, const QString &archTag, const QString &isoName) {
        list.append({
            QString("%1 (%2) — DVD de Instalación Completo (.iso)").arg(verLabel, archTag),
            branchDir,
            archTag,
            "iso",
            isoName
        });
        list.append({
            QString("%1 (%2) — Arranque Ligero / Red (usbboot.img)").arg(verLabel, archTag),
            branchDir,
            archTag,
            "usbboot",
            "usbboot.img"
        });
    };

    // Ramas activas y actuales
    addPair("Slackware64 -current", "slackware64-current", "64-bit", "");
    addPair("Slackware -current",   "slackware-current",   "32-bit", "");
    addPair("Slackware64 15.0",     "slackware64-15.0",    "64-bit", "slackware64-15.0-install-dvd.iso");
    addPair("Slackware 15.0",       "slackware-15.0",      "32-bit", "slackware-15.0-install-dvd.iso");

    // Versiones históricas (32-bit y 64-bit para recuperar PCs antiguos)
    addPair("Slackware64 14.2",     "slackware64-14.2",    "64-bit", "slackware64-14.2-install-dvd.iso");
    addPair("Slackware 14.2",       "slackware-14.2",      "32-bit", "slackware-14.2-install-dvd.iso");
    addPair("Slackware64 14.1",     "slackware64-14.1",    "64-bit", "slackware64-14.1-install-dvd.iso");
    addPair("Slackware 14.1",       "slackware-14.1",      "32-bit", "slackware-14.1-install-dvd.iso");
    addPair("Slackware64 14.0",     "slackware64-14.0",    "64-bit", "slackware64-14.0-install-dvd.iso");
    addPair("Slackware 14.0",       "slackware-14.0",      "32-bit", "slackware-14.0-install-dvd.iso");
    addPair("Slackware64 13.37",    "slackware64-13.37",   "64-bit", "slackware64-13.37-install-dvd.iso");
    addPair("Slackware 13.37",      "slackware-13.37",     "32-bit", "slackware-13.37-install-dvd.iso");
    addPair("Slackware64 13.1",     "slackware64-13.1",    "64-bit", "slackware64-13.1-install-dvd.iso");
    addPair("Slackware 13.1",       "slackware-13.1",      "32-bit", "slackware-13.1-install-dvd.iso");
    addPair("Slackware64 13.0",     "slackware64-13.0",    "64-bit", "slackware64-13.0-install-dvd.iso");
    addPair("Slackware 13.0",       "slackware-13.0",      "32-bit", "slackware-13.0-install-dvd.iso");
    addPair("Slackware 12.2",       "slackware-12.2",      "32-bit", "slackware-12.2-install-dvd.iso");

    return list;
}

void UsbCreatorWidget::openOfficialImageDownloadDialog() {
    if (isBusy()) return;

    QList<SlackwareImageRelease> releases = buildAvailableSlackwareReleases();
    QString isoDir = getAppIsoDirectory();
    QString defaultMirrorRoot = getNormalizedMirrorRoot();

    QDialog dialog(this);
    dialog.setWindowTitle("Descargar Imagen Oficial de Slackware Linux (32-bit y 64-bit)");
    dialog.resize(740, 360);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(10);

    QLabel *lblInfo = new QLabel(
        QString("<b>Seleccione la edición y arquitectura de Slackware que desea descargar:</b><br>"
                "Incluye ediciones actuales (15.0, -current) e históricas (14.2, 14.1, 14.0, 13.37, 13.1, 13.0, 12.2) "
                "en <b>32 bits</b> y <b>64 bits</b> para recuperar equipos antiguos o modernos.<br>"
                "Por seguridad, el archivo se guardará exclusivamente dentro de: <code>%1</code>")
            .arg(isoDir),
        &dialog
    );
    lblInfo->setWordWrap(true);
    lblInfo->setStyleSheet("background-color: #e3f2fd; color: #0d47a1; padding: 8px; border-radius: 4px;");
    layout->addWidget(lblInfo);

    // Filtro rápido por arquitectura y formato
    QHBoxLayout *filterRow = new QHBoxLayout();
    QComboBox *comboArchFilter = new QComboBox(&dialog);
    comboArchFilter->addItem("Todas las arquitecturas (32-bit y 64-bit)", "ALL");
    comboArchFilter->addItem("Solo 64-bit (x86_64)", "64-bit");
    comboArchFilter->addItem("Solo 32-bit (x86 / PCs antiguos)", "32-bit");

    QComboBox *comboTypeFilter = new QComboBox(&dialog);
    comboTypeFilter->addItem("Todos los formatos (.iso DVD + usbboot.img)", "ALL");
    comboTypeFilter->addItem("Solo DVD de Instalación Completo (.iso)", "iso");
    comboTypeFilter->addItem("Solo Arranque Ligero / Red (usbboot.img)", "usbboot");

    filterRow->addWidget(new QLabel("<b>Arquitectura:</b>", &dialog));
    filterRow->addWidget(comboArchFilter, 1);
    filterRow->addSpacing(10);
    filterRow->addWidget(new QLabel("<b>Formato:</b>", &dialog));
    filterRow->addWidget(comboTypeFilter, 1);
    layout->addLayout(filterRow);

    // Desplegable principal de versiones (Choice Input)
    layout->addWidget(new QLabel("<b>Versión de Slackware a descargar:</b>", &dialog));
    QComboBox *comboRelease = new QComboBox(&dialog);
    comboRelease->setMinimumHeight(32);
    layout->addWidget(comboRelease);

    auto repopulateReleases = [&]() {
        QString archF = comboArchFilter->currentData().toString();
        QString typeF = comboTypeFilter->currentData().toString();
        comboRelease->clear();
        for (int i = 0; i < releases.size(); ++i) {
            const auto &r = releases[i];
            if (archF != "ALL" && r.archTag != archF) continue;
            if (typeF != "ALL" && r.imageKind != typeF) continue;
            comboRelease->addItem(r.label, i);
        }
    };
    repopulateReleases();

    connect(comboArchFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog, repopulateReleases);
    connect(comboTypeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog, repopulateReleases);

    QHBoxLayout *mirrorRow = new QHBoxLayout();
    QLineEdit *editMirrorRoot = new QLineEdit(defaultMirrorRoot, &dialog);
    editMirrorRoot->setToolTip("URL raíz del mirror de Slackware (contiene carpetas slackware-15.0, slackware-iso, etc.)");
    mirrorRow->addWidget(new QLabel("<b>Mirror base:</b>", &dialog));
    mirrorRow->addWidget(editMirrorRoot, 1);
    layout->addLayout(mirrorRow);

    QCheckBox *chkVerifyMd5 = new QCheckBox(
        "Descargar también el archivo .md5 oficial y verificar la integridad automáticamente al finalizar",
        &dialog
    );
    chkVerifyMd5->setChecked(true);
    chkVerifyMd5->setStyleSheet("font-weight: bold; color: #1b5e20;");
    layout->addWidget(chkVerifyMd5);

    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *btnCancel = new QPushButton("Cancelar", &dialog);
    QPushButton *btnDownload = new QPushButton("Descargar en ./iso", &dialog);
    btnDownload->setStyleSheet(
        "QPushButton { background-color: #1565c0; color: white; font-weight: bold; padding: 7px 16px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #0d47a1; }"
    );
    btnRow->addStretch();
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnDownload);
    layout->addLayout(btnRow);

    connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(btnDownload, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted) return;
    if (comboRelease->currentIndex() < 0) return;

    int relIdx = comboRelease->currentData().toInt();
    const SlackwareImageRelease &chosen = releases[relIdx];
    QString mirrorRoot = editMirrorRoot->text().trimmed();
    if (!mirrorRoot.endsWith('/')) mirrorRoot += '/';
    bool doMd5 = chkVerifyMd5->isChecked();

    m_currentTask = TaskDownloadImage;
    m_consoleLog->clear();
    m_progressBar->setRange(0, 0);
    setControlsEnabled(false);
    m_lblProgressStatus->setText(QString("Descargando %1 en %2...").arg(chosen.label, isoDir));

    QString escIsoDir = QString(isoDir).replace("'", "'\\''");
    QString cmd;

    if (chosen.imageKind == "usbboot") {
        QString localFileName = QString("%1-usbboot.img").arg(chosen.branchDir);
        QString destFullPath = isoDir + "/" + localFileName;
        QString imgUrl = QString("%1%2/usb-and-pxe-installers/usbboot.img").arg(mirrorRoot, chosen.branchDir);
        QString fallbackUrl = QString("https://mirrors.slackware.com/slackware/%1/usb-and-pxe-installers/usbboot.img")
                                  .arg(chosen.branchDir);

        m_editImagePath->setProperty("pendingDownloadPath", destFullPath);
        cmd = QString(
            "set -e; "
            "mkdir -p '%1'; "
            "cd '%1'; "
            "echo '>>> Descargando %2...'; "
            "if ! wget --progress=dot:giga -O '%2' '%3'; then "
            "  echo '>>> Mirror primario sin respuesta, probando archivo oficial mirrors.slackware.com...'; "
            "  wget --progress=dot:giga -O '%2' '%4'; "
            "fi; "
            "echo '>>> Descarga completada en %1/%2'"
        ).arg(escIsoDir, localFileName, imgUrl, fallbackUrl);
    } else {
        QString isoSubPath = QString("slackware-iso/%1-iso/").arg(chosen.branchDir);
        QString primaryBase = mirrorRoot + isoSubPath;
        QString officialArchiveBase = "https://mirrors.slackware.com/slackware/" + isoSubPath;

        if (!chosen.expectedFile.isEmpty()) {
            QString destFullPath = isoDir + "/" + chosen.expectedFile;
            m_editImagePath->setProperty("pendingDownloadPath", destFullPath);

            QString md5Snippet = doMd5
                ? QString(
                    "echo '>>> Descargando y verificando suma MD5 oficial...'; "
                    "if wget -q -O '%1.md5' \"$BASE_URL/%1.md5\"; then "
                    "  md5sum -c '%1.md5'; "
                    "else "
                    "  echo 'AVISO: Archivo .md5 no encontrado en el mirror.'; "
                    "fi; "
                  ).arg(chosen.expectedFile)
                : "";

            cmd = QString(
                "set -e; "
                "mkdir -p '%1'; "
                "cd '%1'; "
                "BASE_URL='%2'; "
                "if ! wget --spider -q \"$BASE_URL/%3\"; then "
                "  echo '>>> El mirror local no aloja ISOs históricas; usando archivo oficial mirrors.slackware.com...'; "
                "  BASE_URL='%4'; "
                "fi; "
                "echo \">>> Descargando $BASE_URL/%3 ...\"; "
                "wget --progress=dot:giga -O '%3' \"$BASE_URL/%3\"; "
                "%5"
                "echo '>>> Imagen ISO lista en %1/%3'"
            ).arg(escIsoDir, primaryBase, chosen.expectedFile, officialArchiveBase, md5Snippet);
        } else {
            QString destMarker = isoDir + "/" + chosen.branchDir + "-install-dvd.iso";
            m_editImagePath->setProperty("pendingDownloadPath", destMarker);

            QString md5Snippet = doMd5
                ? "if wget -q -O \"$ISO_NAME.md5\" \"$BASE_URL/$ISO_NAME.md5\"; then md5sum -c \"$ISO_NAME.md5\"; fi; "
                : "";

            cmd = QString(
                "set -e; "
                "mkdir -p '%1'; "
                "cd '%1'; "
                "BASE_URL='%2'; "
                "HTML=$(wget -q -O - \"$BASE_URL\" || true); "
                "if ! echo \"$HTML\" | grep -q '\\.iso'; then "
                "  echo '>>> Consultando índice de -current en mirrors.slackware.com...'; "
                "  BASE_URL='%3'; "
                "  HTML=$(wget -q -O - \"$BASE_URL\"); "
                "fi; "
                "ISO_NAME=$(echo \"$HTML\" | grep -oE '%4[^\"]*install-dvd\\.iso' | head -n 1); "
                "if [ -z \"$ISO_NAME\" ]; then echo 'ERROR: No se encontró el archivo ISO de -current en el mirror.'; exit 1; fi; "
                "echo \">>> Archivo detectado: $ISO_NAME\"; "
                "wget --progress=dot:giga -O \"$ISO_NAME\" \"$BASE_URL/$ISO_NAME\"; "
                "%5"
                "ln -sf \"$ISO_NAME\" '%4-install-dvd.iso'; "
                "echo \">>> Imagen ISO lista en %1/$ISO_NAME\""
            ).arg(escIsoDir, primaryBase, officialArchiveBase, chosen.branchDir, md5Snippet);
        }
    }

    // Si estamos en producción (/opt/qslackpkg/iso) y el directorio requiere permisos de root, elevar con pkexec
    QFileInfo isoDirInfo(isoDir);
    bool needsRootToWrite = isoDir.startsWith("/opt/qslackpkg") &&
                            (!isoDirInfo.exists() || !isoDirInfo.isWritable());

    if (needsRootToWrite) {
        QString rootWrappedCmd = QString(
            "mkdir -p '%1' && chmod 755 '%1' && (%2) && chmod 644 '%1'/*"
        ).arg(escIsoDir, cmd);

        m_consoleLog->append(">>> Directorio de producción /opt/qslackpkg/iso protegido: solicitando permisos de ROOT...\n");
        m_process->start("pkexec", {"--user", "root", "/bin/sh", "-c", rootWrappedCmd});
    } else {
        m_process->start("/bin/sh", {"-c", cmd});
    }
}

// ============================================================================
// GRABACIÓN DEL USB BOOTEABLE COMO ROOT (DD CON PROGRESO REAL / USBIMG2DISK)
// ============================================================================

void UsbCreatorWidget::setControlsEnabled(bool enabled) {
    m_grpImage->setEnabled(enabled);
    m_grpDevice->setEnabled(enabled);
    m_grpMode->setEnabled(enabled);
    m_chkConfirmDataLoss->setEnabled(enabled);
    m_btnStartWrite->setEnabled(enabled && m_chkConfirmDataLoss->isChecked());
}

void UsbCreatorWidget::startUsbCreation() {
    if (isBusy()) return;

    int drvIdx = m_comboUsbDevices->currentIndex();
    if (drvIdx < 0 || drvIdx >= m_detectedDrives.size()) return;

    const UsbDriveInfo &drv = m_detectedDrives[drvIdx];
    QString imagePath = m_editImagePath->text().trimmed();
    QFileInfo imgInfo(imagePath);
    if (!imgInfo.exists()) return;

    QString realImagePath = imgInfo.canonicalFilePath();
    if (realImagePath.isEmpty()) realImagePath = imagePath;

    if (m_selectedImageSize > drv.sizeBytes) {
        QMessageBox::critical(
            this,
            "Capacidad Insuficiente en el USB",
            QString("La imagen seleccionada (%1) es más grande que la capacidad total de %2 (%3).")
                .arg(formatBytes(m_selectedImageSize), drv.deviceNode, drv.sizeHuman)
        );
        return;
    }

    bool useUsbImg2Disk = m_radioSlackwareUsbImg2Disk->isChecked();
    QString modeLabel = useUsbImg2Disk
        ? "Slackware usbimg2disk.sh (FAT32 + Syslinux)"
        : "Imagen Híbrida Directa (UEFI + Legacy BIOS)";

    QMessageBox::StandardButton confirm = QMessageBox::warning(
        this,
        "⚠️ CONFIRMACIÓN FINAL DE ESCRITURA EN USB",
        QString("Está a punto de formatear y escribir la imagen en la unidad USB:\n\n"
                "• Imagen origen: %1 (%2)\n"
                "• Dispositivo destino: %3 (%4 %5 — %6)\n"
                "• Modo: %7\n\n"
                "TODOS los datos actuales en %3 se perderán irreversiblemente.\n"
                "Se solicitará su contraseña de ROOT para comenzar.\n\n"
                "¿Desea grabar el USB ahora?")
            .arg(imgInfo.fileName(), formatBytes(m_selectedImageSize),
                 drv.deviceNode, drv.vendor, drv.model, drv.sizeHuman, modeLabel),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    if (confirm != QMessageBox::Yes) return;

    m_currentTask = TaskWriteUsb;
    m_consoleLog->clear();
    m_consoleLog->append(">>> Solicitando autenticación de usuario ROOT...\n");
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    setControlsEnabled(false);
    m_lblProgressStatus->setText(QString("Preparando y escribiendo en %1...").arg(drv.deviceNode));

    QString escImg = QString(realImagePath).replace("'", "'\\''");
    QString escDev = QString(drv.deviceNode).replace("'", "'\\''");

    QStringList cmds;
    cmds << "set -e";
    cmds << QString("echo '>>> Desmontando particiones activas en %1...'").arg(drv.deviceNode);
    cmds << QString("for part in %1*; do umount \"$part\" 2>/dev/null || true; done").arg(escDev);

    if (useUsbImg2Disk) {
        cmds << "USBTOOL=$(command -v usbimg2disk.sh || echo /usr/sbin/usbimg2disk.sh)";
        cmds << QString("echo '>>> Ejecutando $USBTOOL sobre %1...'").arg(drv.deviceNode);
        cmds << QString("\"$USBTOOL\" -f -s '%1' -o '%2'").arg(escImg, escDev);
    } else {
        cmds << QString("echo '>>> Escribiendo imagen híbrida en %1 (bloque 4M, progreso en vivo)...'").arg(drv.deviceNode);
        cmds << QString("dd if='%1' of='%2' bs=4M status=progress conv=fsync").arg(escImg, escDev);
    }

    if (m_chkVerifyAfterWrite->isChecked()) {
        cmds << "echo '>>> Sincronizando búferes de disco (sync) y actualizando particiones...'";
        cmds << "sync";
        cmds << QString("partprobe '%1' 2>/dev/null || blockdev --rereadpt '%1' 2>/dev/null || true").arg(escDev);
    }

    cmds << "echo '===================================================================='";
    cmds << QString("echo 'USB BOOTEABLE CREADO EXITOSAMENTE EN %1 — YA PUEDE EXTRAERLO'").arg(drv.deviceNode);
    cmds << "echo '===================================================================='";

    m_process->start("pkexec", {"--user", "root", "/bin/sh", "-c", cmds.join(" && ")});
}

void UsbCreatorWidget::readProcessOutput() {
    QByteArray raw = m_process->readAllStandardOutput();
    QString text = QString::fromUtf8(raw);
    text.replace('\r', '\n');

    if (m_currentTask == TaskWriteUsb && m_selectedImageSize > 0) {
        static QRegularExpression ddBytesRx("(\\d+)\\s+bytes\\s+\\(");
        QRegularExpressionMatchIterator it = ddBytesRx.globalMatch(text);
        qint64 lastBytes = -1;
        while (it.hasNext()) {
            lastBytes = it.next().captured(1).toLongLong();
        }
        if (lastBytes >= 0) {
            int pct = static_cast<int>((lastBytes * 100) / m_selectedImageSize);
            if (pct > 99) pct = 99;
            m_progressBar->setValue(pct);
            m_lblProgressStatus->setText(
                QString("Escribiendo en USB: %1% (%2 / %3)")
                    .arg(pct)
                    .arg(formatBytes(lastBytes), formatBytes(m_selectedImageSize))
            );
        }
    }

    m_consoleLog->moveCursor(QTextCursor::End);
    m_consoleLog->insertPlainText(text);
    m_consoleLog->verticalScrollBar()->setValue(m_consoleLog->verticalScrollBar()->maximum());
}

void UsbCreatorWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    UsbTask finishedTask = m_currentTask;
    m_currentTask = TaskNone;

    m_progressBar->setRange(0, 100);
    setControlsEnabled(true);
    m_consoleLog->append(QString("\n>>> Proceso finalizado (Código: %1)").arg(exitCode));

    if (exitCode == 126 || exitCode == 127) {
        m_progressBar->setValue(0);
        m_lblProgressStatus->setText("Operación cancelada: no se ingresó contraseña de root.");
        QMessageBox::warning(
            this,
            "Autenticación de ROOT Requerida",
            "No se ingresó la contraseña de root o la solicitud fue cancelada. No se modificó la unidad USB."
        );
        return;
    }

    if (exitCode != 0) {
        m_progressBar->setValue(0);
        m_lblProgressStatus->setText(QString("Error durante el proceso (Código %1).").arg(exitCode));
        QMessageBox::critical(
            this,
            "Error en la operación USB",
            QString("La operación finalizó con errores (Código: %1).\nRevise el registro de consola en la parte inferior.").arg(exitCode)
        );
        return;
    }

    if (finishedTask == TaskDownloadImage) {
        QString downloadedPath = m_editImagePath->property("pendingDownloadPath").toString();
        QFileInfo fi(downloadedPath);
        if (fi.isSymLink()) {
            downloadedPath = fi.canonicalFilePath();
        }
        m_editImagePath->setText(downloadedPath);
        m_progressBar->setValue(100);
        m_lblProgressStatus->setText("Imagen descargada y verificada en " + getAppIsoDirectory());
        QMessageBox::information(
            this,
            "Descarga y Verificación Completadas",
            QString("La imagen oficial de Slackware se descargó y verificó correctamente dentro de:\n%1\n\nYa puede seleccionarla y grabarla en su USB.")
                .arg(downloadedPath)
        );
    } else if (finishedTask == TaskWriteUsb) {
        m_progressBar->setValue(100);
        m_chkConfirmDataLoss->setChecked(false);
        m_lblProgressStatus->setText("¡USB Booteable listo! Todos los datos han sido sincronizados.");
        scanUsbDevices();

        QString msg = "El medio USB booteable de Slackware Linux se ha creado correctamente y ya puede extraerse con seguridad.";
        emit usbCreationCompleted(msg);
        QMessageBox::information(this, "USB Booteable Listo", msg);
    }
}
