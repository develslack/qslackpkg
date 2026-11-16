#include "systemhealthwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSysInfo>
#include <QCollator>

SystemHealthWidget::SystemHealthWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Banner de seguridad y propósito
    QLabel *lblBanner = new QLabel(
        "<b>Salud del Sistema, Integridad de Librerías y Limpieza Segura:</b> "
        "Audita binarios del sistema en busca de bibliotecas compartidas faltantes (<code>ldd</code>) "
        "identificando el paquete responsable y su solución, y permite liberar espacio en disco de cachés "
        "de compilación y descargas sin tocar jamás el kernel activo ni archivos críticos.",
        this
    );
    lblBanner->setWordWrap(true);
    lblBanner->setStyleSheet(
        "background-color: #e8f5e9; color: #1b5e20; border: 1px solid #a5d6a7; "
        "border-left: 5px solid #2e7d32; border-radius: 4px; padding: 8px; font-size: 9.2pt;"
    );
    mainLayout->addWidget(lblBanner);

    // ========================================================================
    // 1. AUDITORÍA DE LIBRERÍAS ROTAS (SOLO LECTURA / DIAGNÓSTICO PRECISO)
    // ========================================================================
    m_grpLdd = new QGroupBox("1. Auditoría de Integridad de Bibliotecas Compartidas (ldd)", this);
    QVBoxLayout *lddLayout = new QVBoxLayout(m_grpLdd);

    QHBoxLayout *lddTop = new QHBoxLayout();
    m_btnScanLibs = new QPushButton("Analizar librerías rotas en /usr/bin y /usr/local/bin", m_grpLdd);
    m_btnScanLibs->setCursor(Qt::PointingHandCursor);
    m_btnScanLibs->setStyleSheet(
        "QPushButton { background-color: #1565c0; color: white; font-weight: bold; "
        "padding: 6px 14px; border-radius: 4px; }"
        "QPushButton:hover:enabled { background-color: #0d47a1; }"
    );

    m_lblLddSummary = new QLabel("Pulse 'Analizar librerías rotas' para verificar la salud de los ejecutables instalados.", m_grpLdd);
    m_lblLddSummary->setStyleSheet("color: #37474f; font-weight: bold;");

    lddTop->addWidget(m_btnScanLibs);
    lddTop->addSpacing(12);
    lddTop->addWidget(m_lblLddSummary, 1);
    lddLayout->addLayout(lddTop);

    m_tableBrokenLibs = new QTableWidget(0, 4, m_grpLdd);
    m_tableBrokenLibs->setHorizontalHeaderLabels({
        "Binario Afectado",
        "Paquete Propietario",
        "Librería Faltante (not found)",
        "Paquete Proveedor Sugerido"
    });
    m_tableBrokenLibs->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableBrokenLibs->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableBrokenLibs->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableBrokenLibs->setAlternatingRowColors(true);
    lddLayout->addWidget(m_tableBrokenLibs);

    mainLayout->addWidget(m_grpLdd, 1);

    // ========================================================================
    // 2. LIMPIEZA SEGURA DE ESPACIO EN DISCO (CON PROTECCIÓN DEL KERNEL ACTIVO)
    // ========================================================================
    m_grpCleanup = new QGroupBox("2. Liberación Segura de Espacio en Disco (Requiere contraseña de root)", this);
    QVBoxLayout *cleanLayout = new QVBoxLayout(m_grpCleanup);

    m_chkCleanSlackpkgCache = new QCheckBox("Caché de paquetes descargados de slackpkg (/var/cache/packages): calculando...", m_grpCleanup);
    m_chkCleanSboTmp = new QCheckBox("Temporales y fuentes compiladas de sbopkg (/tmp/SBo y /var/cache/sbopkg): calculando...", m_grpCleanup);
    m_chkCleanOldKernels = new QCheckBox("Kernels antiguos inactivos en /boot (Protege el kernel en ejecución y el más reciente): calculando...", m_grpCleanup);
    m_chkCleanOldKernels->setStyleSheet("color: #c62828; font-weight: bold;");

    m_lblInactiveKernelsDetail = new QLabel("", m_grpCleanup);
    m_lblInactiveKernelsDetail->setStyleSheet("color: #546e7a; font-size: 8.8pt; padding-left: 20px;");
    m_lblInactiveKernelsDetail->setWordWrap(true);

    cleanLayout->addWidget(m_chkCleanSlackpkgCache);
    cleanLayout->addWidget(m_chkCleanSboTmp);
    cleanLayout->addWidget(m_chkCleanOldKernels);
    cleanLayout->addWidget(m_lblInactiveKernelsDetail);

    QHBoxLayout *cleanBtnRow = new QHBoxLayout();
    m_btnRefreshDisk = new QPushButton("Recalcular espacio", m_grpCleanup);
    m_btnRefreshDisk->setCursor(Qt::PointingHandCursor);

    m_btnExecuteCleanup = new QPushButton("Limpiar elementos seleccionados (Requiere root)", m_grpCleanup);
    m_btnExecuteCleanup->setCursor(Qt::PointingHandCursor);
    m_btnExecuteCleanup->setStyleSheet(
        "QPushButton { background-color: #2e7d32; color: white; font-weight: bold; "
        "padding: 7px 16px; border-radius: 4px; }"
        "QPushButton:hover:enabled { background-color: #1b5e20; }"
    );

    cleanBtnRow->addWidget(m_btnRefreshDisk);
    cleanBtnRow->addStretch();
    cleanBtnRow->addWidget(m_btnExecuteCleanup);
    cleanLayout->addLayout(cleanBtnRow);

    mainLayout->addWidget(m_grpCleanup);

    // ========================================================================
    // 3. PROGRESO Y CONSOLA
    // ========================================================================
    QHBoxLayout *progRow = new QHBoxLayout();
    m_lblStatus = new QLabel("Sistema listo.", this);
    m_lblStatus->setStyleSheet("font-weight: bold; color: #37474f;");
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setMinimumWidth(240);
    progRow->addWidget(m_lblStatus, 1);
    progRow->addWidget(m_progressBar);
    mainLayout->addLayout(progRow);

    m_consoleLog = new QTextEdit(this);
    m_consoleLog->setReadOnly(true);
    m_consoleLog->setMaximumHeight(115);
    m_consoleLog->setStyleSheet(
        "background-color: #1e1e1e; color: #d4d4d4; font-family: monospace; font-size: 8.8pt; border-radius: 4px;"
    );
    m_consoleLog->setPlaceholderText("Registro de operaciones de limpieza...");
    mainLayout->addWidget(m_consoleLog);

    m_auditProcess = new QProcess(this);
    m_rootProcess = new QProcess(this);
    m_rootProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_btnScanLibs, &QPushButton::clicked, this, &SystemHealthWidget::runBrokenLibsAudit);
    connect(m_auditProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SystemHealthWidget::onBrokenLibsProcessFinished);

    connect(m_btnRefreshDisk, &QPushButton::clicked, this, &SystemHealthWidget::scanDiskUsage);
    connect(m_btnExecuteCleanup, &QPushButton::clicked, this, &SystemHealthWidget::runSafeCleanup);

    connect(m_rootProcess, &QProcess::readyReadStandardOutput, this, &SystemHealthWidget::readRootProcessOutput);
    connect(m_rootProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SystemHealthWidget::onRootProcessFinished);

    scanDiskUsage();
}

SystemHealthWidget::~SystemHealthWidget() {
    if (m_auditProcess->state() != QProcess::NotRunning) {
        m_auditProcess->terminate();
        m_auditProcess->waitForFinished(2000);
    }
    if (m_rootProcess->state() != QProcess::NotRunning) {
        m_rootProcess->terminate();
        m_rootProcess->waitForFinished(2000);
    }
}

bool SystemHealthWidget::isBusy() const {
    return (m_auditProcess->state() != QProcess::NotRunning) ||
           (m_rootProcess->state() != QProcess::NotRunning);
}

QString SystemHealthWidget::formatBytes(qint64 bytes) const {
    const double kb = 1024.0;
    const double mb = kb * 1024.0;
    const double gb = mb * 1024.0;
    if (bytes >= gb) return QString::number(bytes / gb, 'f', 2) + " GiB";
    if (bytes >= mb) return QString::number(bytes / mb, 'f', 1) + " MiB";
    if (bytes >= kb) return QString::number(bytes / kb, 'f', 1) + " KiB";
    return QString::number(bytes) + " B";
}

qint64 SystemHealthWidget::calculateDirectorySize(const QString &dirPath) const {
    QDir dir(dirPath);
    if (!dir.exists()) return 0;

    qint64 total = 0;
    QDirIterator it(dirPath, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        total += it.fileInfo().size();
    }
    return total;
}

// ============================================================================
// AUDITORÍA DE LIBRERÍAS ROTAS (LDD)
// ============================================================================

void SystemHealthWidget::runBrokenLibsAudit() {
    if (isBusy()) return;

    m_tableBrokenLibs->setRowCount(0);
    m_btnScanLibs->setEnabled(false);
    m_progressBar->setRange(0, 0);
    m_lblLddSummary->setStyleSheet("color: #1565c0; font-weight: bold;");
    m_lblLddSummary->setText("Escaneando ejecutables ELF en /usr/bin y /usr/local/bin...");

    // Escanea ejecutables ELF y filtra únicamente líneas "=> not found"
    QString cmd =
        "for DIR in /usr/bin /usr/local/bin; do "
        "  [ -d \"$DIR\" ] || continue; "
        "  for BIN in \"$DIR\"/*; do "
        "    [ -f \"$BIN\" ] && [ -x \"$BIN\" ] || continue; "
        "    head -c 4 \"$BIN\" 2>/dev/null | grep -q $'\\x7fELF' || continue; "
        "    MISSING=$(LC_ALL=C ldd \"$BIN\" 2>/dev/null | awk '/=> not found/{print $1}'); "
        "    for LIB in $MISSING; do "
        "      echo \"$BIN|$LIB\"; "
        "    done; "
        "  done; "
        "done";

    m_auditProcess->start("/bin/sh", {"-c", cmd});
}

QString SystemHealthWidget::findOwnerPackageForFile(const QString &filePath) const {
    QString relPath = filePath;
    if (relPath.startsWith('/')) relPath = relPath.mid(1);

    QDir pkgDir("/var/log/packages");
    for (const QString &pkgName : pkgDir.entryList(QDir::Files)) {
        QFile f(pkgDir.filePath(pkgName));
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray data = f.readAll();
            f.close();
            if (data.contains(relPath.toUtf8())) {
                return pkgName;
            }
        }
    }
    return "Desconocido / Compilado manualmente";
}

QString SystemHealthWidget::findProviderForMissingLib(const QString &libName) const {
    // 1. Buscar primero en paquetes ya instalados (por si es un problema de ldconfig / symlink)
    QDir pkgDir("/var/log/packages");
    for (const QString &pkgName : pkgDir.entryList(QDir::Files)) {
        QFile f(pkgDir.filePath(pkgName));
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray data = f.readAll();
            f.close();
            if (data.contains(libName.toUtf8())) {
                return QString("Ya instalado en %1 (ejecutar ldconfig)").arg(pkgName);
            }
        }
    }

    // 2. Buscar en el índice local de slackpkg (/var/lib/slackpkg/pkglist)
    QFile pkgList("/var/lib/slackpkg/pkglist");
    if (pkgList.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString baseToken = libName.section(".so", 0, 0);
        QTextStream in(&pkgList);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.contains(baseToken, Qt::CaseInsensitive)) {
                QStringList cols = line.split(QRegularExpression("\\s+"));
                if (cols.size() >= 6) {
                    return QString("Posible paquete oficial: %1").arg(cols[5]);
                }
            }
        }
        pkgList.close();
    }

    return "Revisar dependencias SBo / AlienBOB";
}

void SystemHealthWidget::onBrokenLibsProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);

    m_btnScanLibs->setEnabled(true);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(100);

    QString out = QString::fromUtf8(m_auditProcess->readAllStandardOutput()).trimmed();
    QStringList lines = out.split('\n', Qt::SkipEmptyParts);

    if (lines.isEmpty()) {
        m_lblLddSummary->setStyleSheet("color: #1b5e20; font-weight: bold;");
        m_lblLddSummary->setText("¡Excelente! 0 librerías rotas detectadas en los ejecutables del sistema.");
        return;
    }

    for (const QString &line : lines) {
        QStringList parts = line.split('|');
        if (parts.size() < 2) continue;

        QString binPath = parts[0].trimmed();
        QString missingLib = parts[1].trimmed();
        QString ownerPkg = findOwnerPackageForFile(binPath);
        QString provider = findProviderForMissingLib(missingLib);

        int r = m_tableBrokenLibs->rowCount();
        m_tableBrokenLibs->insertRow(r);

        QTableWidgetItem *binItem = new QTableWidgetItem(binPath);
        QTableWidgetItem *ownItem = new QTableWidgetItem(ownerPkg);
        QTableWidgetItem *libItem = new QTableWidgetItem(missingLib);
        libItem->setForeground(QColor("#c62828"));
        QFont f = libItem->font(); f.setBold(true); libItem->setFont(f);
        QTableWidgetItem *provItem = new QTableWidgetItem(provider);

        m_tableBrokenLibs->setItem(r, 0, binItem);
        m_tableBrokenLibs->setItem(r, 1, ownItem);
        m_tableBrokenLibs->setItem(r, 2, libItem);
        m_tableBrokenLibs->setItem(r, 3, provItem);
    }

    m_lblLddSummary->setStyleSheet("color: #e65100; font-weight: bold;");
    m_lblLddSummary->setText(
        QString("Se detectaron %1 dependencia(s) de librería faltante(s). Revise los paquetes sugeridos.")
            .arg(m_tableBrokenLibs->rowCount())
    );
}

// ============================================================================
// AUDITORÍA Y LIMPIEZA SEGURA DE ESPACIO EN DISCO
// ============================================================================

void SystemHealthWidget::scanDiskUsage() {
    m_slackpkgCacheBytes = calculateDirectorySize("/var/cache/packages");
    m_sboTmpBytes = calculateDirectorySize("/tmp/SBo") + calculateDirectorySize("/var/cache/sbopkg");

    m_chkCleanSlackpkgCache->setText(
        QString("Caché de descargas de slackpkg (/var/cache/packages): %1 recuperables")
            .arg(formatBytes(m_slackpkgCacheBytes))
    );
    m_chkCleanSlackpkgCache->setEnabled(m_slackpkgCacheBytes > 0);
    m_chkCleanSlackpkgCache->setChecked(m_slackpkgCacheBytes > 0);

    m_chkCleanSboTmp->setText(
        QString("Temporales de compilación sbopkg (/tmp/SBo y /var/cache/sbopkg): %1 recuperables")
            .arg(formatBytes(m_sboTmpBytes))
    );
    m_chkCleanSboTmp->setEnabled(m_sboTmpBytes > 0);
    m_chkCleanSboTmp->setChecked(m_sboTmpBytes > 0);

    // Detectar kernels antiguos inactivos en /boot (SIEMPRE protege el kernel en ejecución y el más nuevo)
    m_inactiveKernels.clear();
    QString runningKernel = QSysInfo::kernelVersion().trimmed();

    QDir bootDir("/boot");
    QStringList bootFiles = bootDir.entryList(QDir::Files | QDir::NoSymLinks);
    QRegularExpression verRx("(\\d+\\.\\d+\\.\\d+(?:-[a-zA-Z0-9._]+)?)");

    QSet<QString> allVersions;
    for (const QString &f : bootFiles) {
        if (f.startsWith("vmlinuz-")) {
            QRegularExpressionMatch m = verRx.match(f);
            if (m.hasMatch()) allVersions.insert(m.captured(1));
        }
    }

    QStringList sortedVersions = allVersions.values();
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(sortedVersions.begin(), sortedVersions.end(),
              [&](const QString &a, const QString &b) { return collator.compare(a, b) < 0; });

    QString latestVersion = sortedVersions.isEmpty() ? runningKernel : sortedVersions.last();
    qint64 totalOldKernelBytes = 0;
    QStringList oldVerLabels;

    for (const QString &ver : sortedVersions) {
        if (ver == runningKernel || ver == latestVersion) continue;

        InactiveKernelItem item;
        item.version = ver;
        for (const QString &bf : bootFiles) {
            if (bf.contains(ver)) {
                QString fullPath = "/boot/" + bf;
                item.filesInBoot.append(fullPath);
                item.totalBytes += QFileInfo(fullPath).size();
            }
        }
        if (!item.filesInBoot.isEmpty()) {
            m_inactiveKernels.append(item);
            totalOldKernelBytes += item.totalBytes;
            oldVerLabels << QString("%1 (%2)").arg(ver, formatBytes(item.totalBytes));
        }
    }

    m_chkCleanOldKernels->setChecked(false); // Nunca marcado por defecto por seguridad
    m_chkCleanOldKernels->setEnabled(!m_inactiveKernels.isEmpty());
    m_chkCleanOldKernels->setText(
        QString("Kernels antiguos inactivos en /boot (%1 versión(es) obsoleta(s)): %2 recuperables")
            .arg(m_inactiveKernels.size())
            .arg(formatBytes(totalOldKernelBytes))
    );

    if (m_inactiveKernels.isEmpty()) {
        m_lblInactiveKernelsDetail->setText(
            QString("Protegidos: Kernel en ejecución (%1) y última versión instalada (%2). No hay kernels antiguos para limpiar.")
                .arg(runningKernel, latestVersion)
        );
    } else {
        m_lblInactiveKernelsDetail->setText(
            QString("Versiones candidatas a limpiar: %1 | Siempre protegidos: %2 (activo) y %3 (reciente).")
                .arg(oldVerLabels.join(", "), runningKernel, latestVersion)
        );
    }
}

void SystemHealthWidget::runSafeCleanup() {
    if (isBusy()) return;

    bool cleanSlackpkg = m_chkCleanSlackpkgCache->isChecked() && m_slackpkgCacheBytes > 0;
    bool cleanSbo = m_chkCleanSboTmp->isChecked() && m_sboTmpBytes > 0;
    bool cleanKernels = m_chkCleanOldKernels->isChecked() && !m_inactiveKernels.isEmpty();

    if (!cleanSlackpkg && !cleanSbo && !cleanKernels) {
        QMessageBox::information(this, "Nada seleccionado",
                                 "Marque al menos una categoría con espacio recuperable para limpiar.");
        return;
    }

    QStringList summary;
    if (cleanSlackpkg) summary << "• Vaciar caché de descargas de slackpkg (/var/cache/packages)";
    if (cleanSbo) summary << "• Eliminar temporales de compilación SBo (/tmp/SBo y /var/cache/sbopkg)";
    if (cleanKernels) {
        summary << "• Eliminar archivos de kernels antiguos inactivos en /boot (NO afecta al kernel en uso ni al más reciente)";
    }

    QMessageBox::StandardButton confirm = QMessageBox::question(
        this,
        "Confirmar Limpieza Segura (Requiere root)",
        QString("Se ejecutarán las siguientes acciones de limpieza como <b>root</b>:\n\n%1\n\n"
                "¿Desea continuar e ingresar la contraseña de root?")
            .arg(summary.join("\n")),
        QMessageBox::Yes | QMessageBox::No
    );
    if (confirm != QMessageBox::Yes) return;

    QStringList cmds;
    cmds << "set -e";

    if (cleanSlackpkg) {
        cmds << "echo '>>> Limpiando /var/cache/packages...'";
        cmds << "rm -rf /var/cache/packages/*";
    }
    if (cleanSbo) {
        cmds << "echo '>>> Limpiando /tmp/SBo y /var/cache/sbopkg...'";
        cmds << "rm -rf /tmp/SBo/* /var/cache/sbopkg/*";
    }
    if (cleanKernels) {
        cmds << "echo '>>> Eliminando archivos de kernels antiguos inactivos en /boot...'";
        for (const auto &kItem : m_inactiveKernels) {
            for (const QString &filePath : kItem.filesInBoot) {
                QString esc = QString(filePath).replace("'", "'\\''");
                cmds << QString("rm -fv '%1'").arg(esc);
            }
        }
    }
    cmds << "echo '>>> LIMPIEZA SEGURA COMPLETADA'";

    m_consoleLog->clear();
    m_progressBar->setRange(0, 0);
    m_btnExecuteCleanup->setEnabled(false);
    m_lblStatus->setText("Ejecutando limpieza segura como root...");

    m_rootProcess->start("pkexec", {"--user", "root", "/bin/sh", "-c", cmds.join(" && ")});
}

void SystemHealthWidget::readRootProcessOutput() {
    QString text = QString::fromUtf8(m_rootProcess->readAllStandardOutput());
    m_consoleLog->moveCursor(QTextCursor::End);
    m_consoleLog->insertPlainText(text);
    m_consoleLog->verticalScrollBar()->setValue(m_consoleLog->verticalScrollBar()->maximum());
}

void SystemHealthWidget::onRootProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    m_progressBar->setRange(0, 100);
    m_btnExecuteCleanup->setEnabled(true);
    m_consoleLog->append(QString("\n>>> Proceso finalizado (Código: %1)").arg(exitCode));

    if (exitCode == 126 || exitCode == 127) {
        m_progressBar->setValue(0);
        m_lblStatus->setText("Operación cancelada: no se ingresó contraseña de root.");
        return;
    }

    scanDiskUsage();
    emit systemCleaned();

    if (exitCode == 0) {
        m_progressBar->setValue(100);
        m_lblStatus->setText("Limpieza completada correctamente.");
        QMessageBox::information(this, "Limpieza Completada",
                                 "El espacio seleccionado se ha liberado de forma segura.");
    } else {
        m_progressBar->setValue(0);
        m_lblStatus->setText(QString("Error durante la limpieza (Código %1).").arg(exitCode));
    }
}
