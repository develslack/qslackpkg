#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QWidget>
#include <QScrollBar>
#include <QMessageBox>
#include <QDialog>
#include <QListWidget>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include <QRegularExpression>
#include <QNetworkProxy>
#include <QUrl>
#include <QApplication>
#include <QStyle>
#include <QCollator>
#include <QPixmap>
#include <QIcon>
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("QSlackpkg - Package Manager for Slackware Linux");
    resize(1300, 740);

    // Cargar logotipo desde recurso embebido o ruta local relativa
    QPixmap logoPixmap(":/img/Linux-Slackware-Logo-Vector.png");
    if (logoPixmap.isNull()) {
        logoPixmap.load(QCoreApplication::applicationDirPath() + "/img/Linux-Slackware-Logo-Vector.png");
    }
    if (!logoPixmap.isNull()) {
        setWindowIcon(QIcon(logoPixmap));
    }

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);

    // ========================================================================
    // 0. CABECERA VISUAL (Logotipo Slackware + Inscripción)
    // ========================================================================
    QWidget *brandHeader = new QWidget(this);
    QVBoxLayout *brandLayout = new QVBoxLayout(brandHeader);
    brandLayout->setContentsMargins(0, 4, 0, 4);
    brandLayout->setSpacing(4);
    brandLayout->setAlignment(Qt::AlignCenter);

    QLabel *lblLogo = new QLabel(this);
    lblLogo->setAlignment(Qt::AlignCenter);
    if (!logoPixmap.isNull()) {
        lblLogo->setPixmap(logoPixmap.scaledToHeight(68, Qt::SmoothTransformation));
    } else {
        lblLogo->setText("SLACKWARE LINUX");
        lblLogo->setStyleSheet("font-size: 18pt; font-weight: bold; color: #1a237e;");
    }

    QLabel *lblSubtitle = new QLabel("Package Manager for Slackware Linux", this);
    lblSubtitle->setAlignment(Qt::AlignCenter);
    lblSubtitle->setStyleSheet(
        "font-size: 11.5pt; font-weight: bold; color: #37474f; "
        "letter-spacing: 0.5px; padding-bottom: 2px;"
    );

    brandLayout->addWidget(lblLogo);
    brandLayout->addWidget(lblSubtitle);
    mainLayout->addWidget(brandHeader);

    // ========================================================================
    // 1. BANNER SUPERIOR DE ESTADO (Panel Verde / Naranja)
    // ========================================================================
    QWidget *bannerContainer = new QWidget(this);
    bannerContainer->setObjectName("bannerContainer");
    QHBoxLayout *bannerLayout = new QHBoxLayout(bannerContainer);
    bannerLayout->setContentsMargins(14, 10, 14, 10);

    m_statusBanner = new QLabel("Verificando estado del sistema...", this);
    m_statusBanner->setStyleSheet("color: white; font-weight: bold; font-size: 11pt; background: transparent;");

    m_btnPendingDetails = new QPushButton("Sin actualizaciones pendientes", this);
    m_btnPendingDetails->setEnabled(false);
    m_btnPendingDetails->setCursor(Qt::PointingHandCursor);
    m_btnPendingDetails->setStyleSheet(
        "QPushButton { background-color: rgba(255, 255, 255, 0.2); color: white; "
        "border: 1px solid rgba(255, 255, 255, 0.5); border-radius: 4px; "
        "padding: 6px 14px; font-weight: bold; }"
        "QPushButton:hover:enabled { background-color: rgba(255, 255, 255, 0.35); }"
        "QPushButton:disabled { color: rgba(255, 255, 255, 0.6); border-color: transparent; }"
    );

    bannerLayout->addWidget(m_statusBanner);
    bannerLayout->addStretch();
    bannerLayout->addWidget(m_btnPendingDetails);
    bannerContainer->setStyleSheet("#bannerContainer { background-color: #2e7d32; border-radius: 6px; }");
    mainLayout->addWidget(bannerContainer);

    // ========================================================================
    // 2. CONTENEDOR DE PESTAÑAS (Sistema Oficial vs SlackBuilds.org)
    // ========================================================================
    m_tabs = new QTabWidget(this);
    m_tabs->addTab(createOfficialTab(), "Sistema Oficial (slackpkg)");
    m_tabs->addTab(createSboTab(), "Repositorio SlackBuilds.org (sbopkg)");
    mainLayout->addWidget(m_tabs);

    // ========================================================================
    // 3. BARRA INFERIOR DE TERMINAL COMPARTIDA
    // ========================================================================
    QHBoxLayout *footerLayout = new QHBoxLayout();
    m_btnToggleConsole = new QPushButton("Mostrar detalles de terminal", this);
    m_btnToggleConsole->setCheckable(true);
    footerLayout->addStretch();
    footerLayout->addWidget(m_btnToggleConsole);
    mainLayout->addLayout(footerLayout);

    m_consoleOutput = new QTextEdit(this);
    m_consoleOutput->setReadOnly(true);
    m_consoleOutput->setVisible(false);
    m_consoleOutput->setMaximumHeight(210);
    m_consoleOutput->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: monospace; font-size: 9pt;");
    mainLayout->addWidget(m_consoleOutput);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    setCentralWidget(centralWidget);

    // Procesos y Red
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    m_netManager = new QNetworkAccessManager(this);

    QMap<QString, QString> proxyVars = loadProxyEnvironment();
    QString proxyStr = proxyVars.value("https_proxy", proxyVars.value("http_proxy"));
    if (!proxyStr.isEmpty()) {
        QUrl proxyUrl(proxyStr);
        if (proxyUrl.isValid() && !proxyUrl.host().isEmpty()) {
            QNetworkProxy proxy(QNetworkProxy::HttpProxy, proxyUrl.host(), proxyUrl.port(8080));
            if (!proxyUrl.userName().isEmpty()) {
                proxy.setUser(proxyUrl.userName());
                proxy.setPassword(proxyUrl.password());
            }
            m_netManager->setProxy(proxy);
        }
    }

    setupTrayIcon();

    // Conexiones generales
    connect(m_btnPendingDetails, &QPushButton::clicked, this, &MainWindow::showPendingUpdatesDialog);
    connect(m_btnToggleConsole, &QPushButton::clicked, this, &MainWindow::toggleConsole);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MainWindow::readProcessOutput);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::processFinished);
    connect(m_netManager, &QNetworkAccessManager::finished, this, &MainWindow::onChangeLogDownloaded);

    // Timer en segundo plano cada 60 min
    m_checkTimer = new QTimer(this);
    connect(m_checkTimer, &QTimer::timeout, this, &MainWindow::checkUpdatesInBackground);
    m_checkTimer->start(3600000);

    // Carga inicial
    scanPendingUpgrades();
    scanSboRepository();
    QTimer::singleShot(3000, this, &MainWindow::checkUpdatesInBackground);
}

QWidget* MainWindow::createOfficialTab() {
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_btnUpdate = new QPushButton("1. Sincronizar listas (slackpkg update)", this);
    m_btnUpgrade = new QPushButton("2. Actualizar sistema (upgrade-all)", this);
    m_btnClean = new QPushButton("3. Limpiar obsoletos (clean-system)", this);

    buttonLayout->addWidget(m_btnUpdate);
    buttonLayout->addWidget(m_btnUpgrade);
    buttonLayout->addWidget(m_btnClean);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    m_packageTable = new QTableWidget(0, 6, tab);
    m_packageTable->setHorizontalHeaderLabels({
        "Paquete",
        "Paquete Base (Rama Principal)",
        "Paquete SBo (SlackBuilds)",
        "Instalado Actual",
        "Estado / Disponible",
        "Acciones"
    });
    m_packageTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_packageTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    m_packageTable->setColumnWidth(5, 110);
    m_packageTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_packageTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_packageTable->setAlternatingRowColors(true);
    layout->addWidget(m_packageTable);

    connect(m_btnUpdate, &QPushButton::clicked, this, &MainWindow::runUpdate);
    connect(m_btnUpgrade, &QPushButton::clicked, this, &MainWindow::runUpgradeAll);
    connect(m_btnClean, &QPushButton::clicked, this, &MainWindow::runCleanSystem);

    return tab;
}

QWidget* MainWindow::createSboTab() {
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);

    QHBoxLayout *topBar = new QHBoxLayout();

    m_btnInstallSbopkg = new QPushButton("Descargar e Instalar sbopkg (0.38.3)", this);
    m_btnInstallSbopkg->setCursor(Qt::PointingHandCursor);
    m_btnInstallSbopkg->setStyleSheet(
        "QPushButton { background-color: #1565c0; color: white; font-weight: bold; "
        "padding: 6px 14px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #0d47a1; }"
    );
    m_btnInstallSbopkg->setVisible(false);

    m_btnSboSync = new QPushButton("Sincronizar SlackBuilds (sbopkg -r)", this);
    m_btnSboSync->setStyleSheet("font-weight: bold;");

    QLabel *lblCat = new QLabel("Categoría:", this);
    m_comboSboCategory = new QComboBox(this);
    m_comboSboCategory->setMinimumWidth(160);
    m_comboSboCategory->addItem("Todas las categorías", "");

    QLabel *lblSearch = new QLabel("Buscar:", this);
    m_editSboSearch = new QLineEdit(this);
    m_editSboSearch->setPlaceholderText("Ej: obs-studio, audacity, libreoffice, vlc...");
    m_editSboSearch->setClearButtonEnabled(true);

    m_chkSboUpdatesOnly = new QCheckBox("Solo con actualización disponible", this);

    topBar->addWidget(m_btnInstallSbopkg);
    topBar->addWidget(m_btnSboSync);
    topBar->addSpacing(12);
    topBar->addWidget(lblCat);
    topBar->addWidget(m_comboSboCategory);
    topBar->addSpacing(12);
    topBar->addWidget(lblSearch);
    topBar->addWidget(m_editSboSearch);
    topBar->addWidget(m_chkSboUpdatesOnly);
    layout->addLayout(topBar);

    m_sboStatusLabel = new QLabel("Analizando repositorio local de SlackBuilds...", this);
    m_sboStatusLabel->setStyleSheet("color: #37474f; font-weight: bold; padding: 4px;");
    layout->addWidget(m_sboStatusLabel);

    m_sboTable = new QTableWidget(0, 6, tab);
    m_sboTable->setHorizontalHeaderLabels({
        "Paquete SBo",
        "Categoría",
        "Versión Instalada",
        "Versión en Repo SBo",
        "Descripción",
        "Acción"
    });
    m_sboTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_sboTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_sboTable->setColumnWidth(0, 170);
    m_sboTable->setColumnWidth(1, 120);
    m_sboTable->setColumnWidth(2, 180);
    m_sboTable->setColumnWidth(3, 150);
    m_sboTable->setColumnWidth(5, 130);
    m_sboTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_sboTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_sboTable->setAlternatingRowColors(true);
    layout->addWidget(m_sboTable);

    connect(m_btnInstallSbopkg, &QPushButton::clicked, this, &MainWindow::runInstallSbopkg);
    connect(m_btnSboSync, &QPushButton::clicked, this, &MainWindow::runSboSync);
    connect(m_comboSboCategory, &QComboBox::currentTextChanged, this, &MainWindow::filterSboTable);
    connect(m_editSboSearch, &QLineEdit::textChanged, this, &MainWindow::filterSboTable);
    connect(m_chkSboUpdatesOnly, &QCheckBox::toggled, this, &MainWindow::filterSboTable);

    return tab;
}

MainWindow::~MainWindow() {
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished(3000);
    }
}

void MainWindow::setupTrayIcon() {
    m_trayIcon = new QSystemTrayIcon(this);
    if (!windowIcon().isNull()) {
        m_trayIcon->setIcon(windowIcon());
    } else {
        m_trayIcon->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    }
    m_trayIcon->setToolTip("QSlackpkg - Package Manager for Slackware Linux");

    m_trayMenu = new QMenu(this);
    QAction *showAction = m_trayMenu->addAction("Abrir Centro de Software");
    QAction *checkAction = m_trayMenu->addAction("Buscar actualizaciones ahora");
    m_trayMenu->addSeparator();
    QAction *quitAction = m_trayMenu->addAction("Salir completamente");

    connect(showAction, &QAction::triggered, this, &MainWindow::showNormal);
    connect(checkAction, &QAction::triggered, this, &MainWindow::checkUpdatesInBackground);
    connect(quitAction, &QAction::triggered, this, [this]() {
        m_forceQuit = true;
        qApp->quit();
    });
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (!m_forceQuit && m_trayIcon->isVisible()) {
        hide();
        m_trayIcon->showMessage(
            "QSlackpkg sigue activo",
            "El monitor continuará buscando actualizaciones en segundo plano.",
            QSystemTrayIcon::Information, 3000
        );
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        if (isVisible()) {
            hide();
        } else {
            showNormal();
            raise();
            activateWindow();
        }
    }
}

void MainWindow::toggleConsole() {
    bool show = m_btnToggleConsole->isChecked();
    m_consoleOutput->setVisible(show);
    m_btnToggleConsole->setText(show ? "Ocultar detalles de terminal" : "Mostrar detalles de terminal");
}

QString MainWindow::getMirrorUrl() {
    QFile file("/etc/slackpkg/mirrors");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty() && !line.startsWith('#') &&
            (line.startsWith("http://") || line.startsWith("https://"))) {
            if (!line.endsWith('/')) line += '/';
            return line;
        }
    }
    return QString();
}

void MainWindow::checkUpdatesInBackground() {
    QString mirror = getMirrorUrl();
    if (mirror.isEmpty()) return;

    QUrl url(mirror + "ChangeLog.txt");
    QNetworkRequest request(url);
    request.setRawHeader("Range", "bytes=0-2048");
    m_netManager->get(request);
}

void MainWindow::onChangeLogDownloaded(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QString content = QString::fromUtf8(reply->readAll());
        QString firstLine = content.section('\n', 0, 0).trimmed();

        QString localFirstLine;
        QFile localChangeLog("/var/lib/slackpkg/ChangeLog.txt");
        if (localChangeLog.open(QIODevice::ReadOnly | QIODevice::Text)) {
            localFirstLine = QString::fromUtf8(localChangeLog.readLine()).trimmed();
            localChangeLog.close();
        }

        if (!firstLine.isEmpty() && firstLine != localFirstLine && firstLine != m_lastKnownChangeLogEntry) {
            m_lastKnownChangeLogEntry = firstLine;
            QWidget *bannerContainer = findChild<QWidget*>("bannerContainer");
            if (bannerContainer) {
                bannerContainer->setStyleSheet("#bannerContainer { background-color: #ef6c00; border-radius: 6px; }");
            }
            m_statusBanner->setText("¡Nuevas actualizaciones en el mirror! (" + firstLine + ")");
            m_trayIcon->setIcon(style()->standardIcon(QStyle::SP_MessageBoxWarning));
            m_trayIcon->showMessage(
                "Actualizaciones de Slackware disponibles",
                "Se detectaron cambios en el repositorio (" + firstLine + "). Pulsa 'Sincronizar listas'.",
                QSystemTrayIcon::Information, 6000
            );
        }
    }
    reply->deleteLater();
}

// ============================================================================
// MOTOR OFICIAL (SLACKPKG)
// ============================================================================

QStringList MainWindow::queryRealSlackpkgUpgrades() {
    QStringList realCandidates;

    QProcess simProc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("DIALOG", "off");
    env.insert("BATCH", "on");
    env.insert("DEFAULT_ANSWER", "n");
    simProc.setProcessEnvironment(env);

    simProc.start("/bin/sh", {"-c",
        "TMPDIR=$(mktemp -d); "
        "sed 's|/var/lock|/tmp|g' /usr/sbin/slackpkg > \"$TMPDIR/slackpkg_sim\"; "
        "chmod +x \"$TMPDIR/slackpkg_sim\"; "
        "\"$TMPDIR/slackpkg_sim\" -dialog=off -batch=on -default_answer=n upgrade-all 2>/dev/null; "
        "rm -rf \"$TMPDIR\""
    });

    if (simProc.waitForFinished(8000)) {
        QString output = stripAnsiCodes(QString::fromUtf8(simProc.readAllStandardOutput()));
        QRegularExpression pkgRx("\\b([a-zA-Z0-9._+-]+-[a-zA-Z0-9._+]+-(?:x86_64|i[3456]86|noarch|fw)-[a-zA-Z0-9._+]+)\\.t[gx]z\\b");
        QRegularExpressionMatchIterator it = pkgRx.globalMatch(output);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString fullName = match.captured(1);
            if (!realCandidates.contains(fullName)) {
                realCandidates.append(fullName);
            }
        }
    }

    return realCandidates;
}

void MainWindow::scanPendingUpgrades() {
    QStringList slackpkgOfficialUpgrades = queryRealSlackpkgUpgrades();
    QMap<QString, QString> confirmedUpgradeMap;

    for (const QString &candFull : slackpkgOfficialUpgrades) {
        QStringList parts = candFull.split('-');
        if (parts.size() >= 4) {
            QString shortName = parts.mid(0, parts.size() - 3).join('-');
            confirmedUpgradeMap.insert(shortName, candFull);
        }
    }

    QList<QRegularExpression> blacklistPatterns;
    QFile blacklistFile("/etc/slackpkg/blacklist");
    if (blacklistFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream blIn(&blacklistFile);
        while (!blIn.atEnd()) {
            QString line = blIn.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#')) continue;
            blacklistPatterns.append(QRegularExpression(line));
        }
        blacklistFile.close();
    }

    auto isBlacklisted = [&](const QString &pkgFullName, const QString &pkgShortName) -> bool {
        for (const auto &rx : blacklistPatterns) {
            if (rx.match(pkgFullName).hasMatch() || rx.match(pkgShortName).hasMatch()) {
                return true;
            }
        }
        return false;
    };

    QMap<QString, QString> basePkgMap;
    QFile pkgListFile("/var/lib/slackpkg/pkglist");
    if (pkgListFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&pkgListFile);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;
            QStringList cols = line.split(QRegularExpression("\\s+"));
            if (cols.size() >= 6) {
                QString repo = cols[0];
                QString shortName = cols[1];
                QString fullName = cols[5];
                if (repo == "slackware64" || repo == "slackware") {
                    basePkgMap.insert(shortName, fullName);
                }
            }
        }
        pkgListFile.close();
    }

    struct PkgRow {
        QString name;
        QString basePkg;
        QString sboPkg;
        QString installed;
        QString available;
        bool hasUpdate;
        bool isSbo;
    };

    QList<PkgRow> rowsWithUpdates;
    QList<PkgRow> rowsUpToDate;
    m_confirmedUpdates.clear();

    QDir pkgDir("/var/log/packages");
    QStringList installedFiles = pkgDir.entryList(QDir::Files, QDir::Name);

    for (const QString &instFullName : installedFiles) {
        QStringList parts = instFullName.split('-');
        if (parts.size() < 4) continue;

        QString shortName = parts.mid(0, parts.size() - 3).join('-');
        QString buildTag = parts.last();
        bool bl = isBlacklisted(instFullName, shortName);
        bool isSboPkg = buildTag.endsWith("_SBo", Qt::CaseInsensitive);

        if (isSboPkg) {
            QString statusText = bl ? "Bloqueado (blacklist)" : "Gestionado por SBo";
            rowsUpToDate.append({shortName, "—", instFullName, instFullName, statusText, false, true});
        } else if (confirmedUpgradeMap.contains(shortName)) {
            QString baseName = basePkgMap.value(shortName, "— (Rama Extra)");
            QString repoFullName = confirmedUpgradeMap.value(shortName);

            rowsWithUpdates.append({shortName, baseName, "—", instFullName, repoFullName, true, false});
            m_confirmedUpdates.append({shortName, instFullName, repoFullName});
        } else {
            QString baseName = basePkgMap.value(shortName, "— (Externo)");
            QString statusText = bl ? "Bloqueado (blacklist)" : "Sin paquete nuevo";
            rowsUpToDate.append({shortName, baseName, "—", instFullName, statusText, false, false});
        }
    }

    m_packageTable->setRowCount(0);
    QList<PkgRow> allRows = rowsWithUpdates + rowsUpToDate;

    for (const PkgRow &item : allRows) {
        int row = m_packageTable->rowCount();
        m_packageTable->insertRow(row);

        QTableWidgetItem *nameItem = new QTableWidgetItem(item.name);
        QTableWidgetItem *baseItem = new QTableWidgetItem(item.basePkg);
        QTableWidgetItem *sboItem = new QTableWidgetItem(item.sboPkg);
        QTableWidgetItem *instItem = new QTableWidgetItem(item.installed);
        QTableWidgetItem *availItem = new QTableWidgetItem(item.available);

        baseItem->setForeground(QColor("#616161"));

        if (item.isSbo) {
            sboItem->setBackground(QColor("#e3f2fd"));
            sboItem->setForeground(QColor("#0d47a1"));
        } else {
            sboItem->setForeground(QColor("#9e9e9e"));
        }

        if (item.hasUpdate) {
            availItem->setBackground(QColor("#ffe0b2"));
            availItem->setForeground(QColor("#e65100"));
            QFont boldFont = availItem->font();
            boldFont.setBold(true);
            availItem->setFont(boldFont);
        } else {
            instItem->setBackground(QColor("#e8f5e9"));
            instItem->setForeground(QColor("#1b5e20"));
            availItem->setForeground(QColor("#757575"));
        }

        m_packageTable->setItem(row, 0, nameItem);
        m_packageTable->setItem(row, 1, baseItem);
        m_packageTable->setItem(row, 2, sboItem);
        m_packageTable->setItem(row, 3, instItem);
        m_packageTable->setItem(row, 4, availItem);

        // Botón Remove en columna 5
        QWidget *cellWidget = new QWidget(m_packageTable);
        QHBoxLayout *cellLayout = new QHBoxLayout(cellWidget);
        cellLayout->setContentsMargins(6, 2, 6, 2);
        cellLayout->setAlignment(Qt::AlignCenter);

        QPushButton *btnRemove = new QPushButton("Remove", cellWidget);
        btnRemove->setCursor(Qt::PointingHandCursor);
        btnRemove->setToolTip(QString("Desinstalar %1").arg(item.installed));
        btnRemove->setStyleSheet(
            "QPushButton { background-color: #ffebee; color: #c62828; border: 1px solid #ef9a9a; "
            "border-radius: 3px; padding: 2px 10px; font-weight: bold; font-size: 8.5pt; }"
            "QPushButton:hover { background-color: #c62828; color: white; }"
        );

        QString pkgShort = item.name;
        QString pkgFull = item.installed;
        connect(btnRemove, &QPushButton::clicked, this, [this, pkgShort, pkgFull]() {
            runRemovePackage(pkgShort, pkgFull);
        });

        cellLayout->addWidget(btnRemove);
        m_packageTable->setCellWidget(row, 5, cellWidget);
    }

    int pendingCount = m_confirmedUpdates.size();
    m_btnUpgrade->setEnabled(pendingCount > 0);
    m_btnPendingDetails->setEnabled(pendingCount > 0);

    QWidget *bannerContainer = findChild<QWidget*>("bannerContainer");
    if (pendingCount > 0) {
        if (bannerContainer) bannerContainer->setStyleSheet("#bannerContainer { background-color: #ef6c00; border-radius: 6px; }");
        m_statusBanner->setText(QString("Atención: Hay %1 paquete(s) oficial(es) listos para actualizar.").arg(pendingCount));
        m_btnPendingDetails->setText(QString("Ver %1 paquete(s) a actualizar...").arg(pendingCount));
        m_trayIcon->setIcon(style()->standardIcon(QStyle::SP_MessageBoxWarning));
    } else {
        if (bannerContainer) bannerContainer->setStyleSheet("#bannerContainer { background-color: #2e7d32; border-radius: 6px; }");
        m_statusBanner->setText("Sistema base al día. Coincide con slackpkg: 0 paquetes pendientes.");
        m_btnPendingDetails->setText("0 paquetes oficiales para actualizar");
        if (!windowIcon().isNull()) {
            m_trayIcon->setIcon(windowIcon());
        } else {
            m_trayIcon->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
        }
    }
}

void MainWindow::showPendingUpdatesDialog() {
    if (m_confirmedUpdates.isEmpty()) return;

    QDialog dialog(this);
    dialog.setWindowTitle(QString("Paquetes disponibles para actualizar (%1)").arg(m_confirmedUpdates.size()));
    dialog.resize(780, 420);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *headerLabel = new QLabel(
        QString("Los siguientes <b>%1 paquete(s)</b> han sido verificados por <code>slackpkg</code> y están listos para instalarse:")
            .arg(m_confirmedUpdates.size()),
        &dialog
    );
    layout->addWidget(headerLabel);

    QTableWidget *summaryTable = new QTableWidget(m_confirmedUpdates.size(), 3, &dialog);
    summaryTable->setHorizontalHeaderLabels({"Paquete", "Versión Actual", "Nueva Versión"});
    summaryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    summaryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    summaryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    summaryTable->setAlternatingRowColors(true);

    for (int i = 0; i < m_confirmedUpdates.size(); ++i) {
        const auto &item = m_confirmedUpdates[i];
        summaryTable->setItem(i, 0, new QTableWidgetItem(item.shortName));
        summaryTable->setItem(i, 1, new QTableWidgetItem(item.installedFull));

        QTableWidgetItem *newItem = new QTableWidgetItem(item.candidateFull);
        newItem->setForeground(QColor("#e65100"));
        QFont f = newItem->font();
        f.setBold(true);
        newItem->setFont(f);
        summaryTable->setItem(i, 2, newItem);
    }
    layout->addWidget(summaryTable);

    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *btnClose = new QPushButton("Cerrar", &dialog);
    QPushButton *btnApplyNow = new QPushButton("Actualizar sistema ahora", &dialog);
    btnApplyNow->setStyleSheet("background-color: #2e7d32; color: white; font-weight: bold; padding: 6px 16px;");

    btnRow->addStretch();
    btnRow->addWidget(btnClose);
    btnRow->addWidget(btnApplyNow);
    layout->addLayout(btnRow);

    connect(btnClose, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(btnApplyNow, &QPushButton::clicked, &dialog, [&]() {
        dialog.accept();
        runUpgradeAll();
    });

    dialog.exec();
}

// ============================================================================
// MOTOR SLACKBUILDS.ORG (SBOPKG + INSTALACIÓN ASISTIDA + RESOLUCIÓN DE DEPS)
// ============================================================================

QString MainWindow::detectSboRepoPath() {
    QDir baseDir("/var/lib/sbopkg/SBo");
    if (!baseDir.exists()) return QString();

    QStringList subDirs = baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);
    for (const QString &dirName : subDirs) {
        QString candidate = baseDir.filePath(dirName);
        if (QFile::exists(candidate + "/SLACKBUILDS.TXT") || QDir(candidate + "/system").exists()) {
            return candidate;
        }
    }
    return QString();
}

void MainWindow::scanSboRepository() {
    m_sboCatalog.clear();

    bool isSbopkgInstalled = QFile::exists("/usr/sbin/sbopkg");

    m_btnInstallSbopkg->setVisible(!isSbopkgInstalled);
    m_btnInstallSbopkg->setEnabled(!isSbopkgInstalled);
    m_btnSboSync->setEnabled(isSbopkgInstalled);
    m_comboSboCategory->setEnabled(isSbopkgInstalled);
    m_editSboSearch->setEnabled(isSbopkgInstalled);
    m_chkSboUpdatesOnly->setEnabled(isSbopkgInstalled);

    if (!isSbopkgInstalled) {
        m_sboStatusLabel->setStyleSheet("color: #c62828; font-weight: bold; padding: 6px;");
        m_sboStatusLabel->setText(
            "sbopkg no está instalado en tu sistema. Haz clic en 'Descargar e Instalar sbopkg (0.38.3)' "
            "para instalarlo automáticamente y habilitar el repositorio de SlackBuilds.org."
        );
        m_sboTable->setRowCount(0);
        return;
    }

    QString repoPath = detectSboRepoPath();
    if (repoPath.isEmpty()) {
        m_sboStatusLabel->setStyleSheet("color: #ef6c00; font-weight: bold; padding: 6px;");
        m_sboStatusLabel->setText(
            "sbopkg está instalado, pero aún no has descargado el catálogo local. "
            "Pulsa 'Sincronizar SlackBuilds (sbopkg -r)' para obtener la lista completa de software."
        );
        m_sboTable->setRowCount(0);
        return;
    }

    QMap<QString, QString> installedFullMap;
    QMap<QString, QString> installedVerMap;
    QDir pkgDir("/var/log/packages");
    for (const QString &fileName : pkgDir.entryList(QDir::Files)) {
        QStringList parts = fileName.split('-');
        if (parts.size() >= 4) {
            QString shortName = parts.mid(0, parts.size() - 3).join('-');
            QString version = parts[parts.size() - 3];
            installedFullMap.insert(shortName, fileName);
            installedVerMap.insert(shortName, version);
        }
    }

    QFile sboIndex(repoPath + "/SLACKBUILDS.TXT");
    QSet<QString> categoriesSet;
    int sboUpdatesCount = 0;

    if (sboIndex.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&sboIndex);
        SboPackageInfo current;

        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("SLACKBUILD NAME:")) {
                current = SboPackageInfo();
                current.name = line.mid(16).trimmed();
            } else if (line.startsWith("SLACKBUILD LOCATION:")) {
                QString loc = line.mid(20).trimmed();
                QStringList locParts = loc.split('/', Qt::SkipEmptyParts);
                if (locParts.size() >= 2) {
                    current.category = locParts[locParts.size() - 2];
                    categoriesSet.insert(current.category);
                }
            } else if (line.startsWith("SLACKBUILD VERSION:")) {
                current.version = line.mid(19).trimmed();
            } else if (line.startsWith("SLACKBUILD REQUIRES:")) {
                QString reqStr = line.mid(20).trimmed();
                if (!reqStr.isEmpty()) {
                    for (const QString &r : reqStr.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts)) {
                        if (r != "%README%") current.requires.append(r);
                    }
                }
            } else if (line.startsWith("SLACKBUILD SHORT DESCRIPTION:")) {
                current.shortDesc = line.mid(29).trimmed();
            } else if (line.trimmed().isEmpty() && !current.name.isEmpty()) {
                if (installedFullMap.contains(current.name)) {
                    current.installedFull = installedFullMap.value(current.name);
                    QString instVer = installedVerMap.value(current.name);
                    if (instVer != current.version) {
                        current.hasUpdate = true;
                        sboUpdatesCount++;
                    }
                }
                m_sboCatalog.insert(current.name, current);
                current = SboPackageInfo();
            }
        }
        sboIndex.close();
    }

    QString prevCat = m_comboSboCategory->currentData().toString();
    m_comboSboCategory->blockSignals(true);
    m_comboSboCategory->clear();
    m_comboSboCategory->addItem(QString("Todas las categorías (%1)").arg(categoriesSet.size()), "");

    QStringList sortedCats = categoriesSet.values();
    sortedCats.sort();
    for (const QString &cat : sortedCats) {
        m_comboSboCategory->addItem(cat, cat);
    }
    int idx = m_comboSboCategory->findData(prevCat);
    if (idx >= 0) m_comboSboCategory->setCurrentIndex(idx);
    m_comboSboCategory->blockSignals(false);

    m_sboStatusLabel->setStyleSheet("color: #0d47a1; font-weight: bold; padding: 6px;");
    m_sboStatusLabel->setText(
        QString("Catálogo SlackBuilds.org listo: %1 paquetes en %2 categorías | Actualizaciones SBo disponibles: %3")
            .arg(m_sboCatalog.size())
            .arg(sortedCats.size())
            .arg(sboUpdatesCount)
    );

    filterSboTable();
}

void MainWindow::runInstallSbopkg() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Instalar sbopkg 0.38.3",
        "Se descargará el paquete oficial desde GitHub y se instalará en tu sistema:\n\n"
        "URL: https://github.com/sbopkg/sbopkg/releases/download/0.38.3/sbopkg-0.38.3-noarch-1_wsr.tgz\n"
        "Comando: installpkg sbopkg-0.38.3-noarch-1_wsr.tgz\n\n"
        "¿Deseas continuar?",
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply != QMessageBox::Yes) return;

    if (!m_btnToggleConsole->isChecked()) {
        m_btnToggleConsole->setChecked(true);
        toggleConsole();
    }

    const QString url = "https://github.com/sbopkg/sbopkg/releases/download/0.38.3/sbopkg-0.38.3-noarch-1_wsr.tgz";
    const QString tmpPkg = "/tmp/sbopkg-0.38.3-noarch-1_wsr.tgz";

    QString cmd = QString(
        "wget -O %1 '%2' && /sbin/installpkg %1 && rm -f %1"
    ).arg(tmpPkg, url);

    executePrivilegedCommand("/bin/sh", {"-c", cmd},
                             "Descargando e instalando sbopkg 0.38.3 en segundo plano...");
}

void MainWindow::filterSboTable() {
    QString selectedCat = m_comboSboCategory->currentData().toString();
    QString searchQuery = m_editSboSearch->text().trimmed();
    bool onlyUpdates = m_chkSboUpdatesOnly->isChecked();

    m_sboTable->setUpdatesEnabled(false);
    m_sboTable->setRowCount(0);

    int maxVisible = (selectedCat.isEmpty() && searchQuery.isEmpty() && !onlyUpdates) ? 400 : 2000;
    int shown = 0;

    for (auto it = m_sboCatalog.constBegin(); it != m_sboCatalog.constEnd(); ++it) {
        const SboPackageInfo &pkg = it.value();

        if (!selectedCat.isEmpty() && pkg.category != selectedCat) continue;
        if (onlyUpdates && !pkg.hasUpdate) continue;
        if (!searchQuery.isEmpty() &&
            !pkg.name.contains(searchQuery, Qt::CaseInsensitive) &&
            !pkg.shortDesc.contains(searchQuery, Qt::CaseInsensitive)) {
            continue;
        }

        if (shown >= maxVisible) break;
        shown++;

        int row = m_sboTable->rowCount();
        m_sboTable->insertRow(row);

        QTableWidgetItem *nameItem = new QTableWidgetItem(pkg.name);
        QFont boldName = nameItem->font();
        boldName.setBold(true);
        nameItem->setFont(boldName);

        QTableWidgetItem *catItem = new QTableWidgetItem(pkg.category);
        QTableWidgetItem *instItem = new QTableWidgetItem(pkg.installedFull.isEmpty() ? "No instalado" : pkg.installedFull);
        QTableWidgetItem *verItem = new QTableWidgetItem(pkg.version);
        QTableWidgetItem *descItem = new QTableWidgetItem(pkg.shortDesc);

        if (!pkg.installedFull.isEmpty()) {
            instItem->setForeground(QColor("#1b5e20"));
            instItem->setBackground(QColor("#e8f5e9"));
        } else {
            instItem->setForeground(QColor("#757575"));
        }

        if (pkg.hasUpdate) {
            verItem->setBackground(QColor("#ffe0b2"));
            verItem->setForeground(QColor("#e65100"));
            verItem->setFont(boldName);
        }

        m_sboTable->setItem(row, 0, nameItem);
        m_sboTable->setItem(row, 1, catItem);
        m_sboTable->setItem(row, 2, instItem);
        m_sboTable->setItem(row, 3, verItem);
        m_sboTable->setItem(row, 4, descItem);

        QWidget *cellWidget = new QWidget(m_sboTable);
        QHBoxLayout *cellLayout = new QHBoxLayout(cellWidget);
        cellLayout->setContentsMargins(6, 2, 6, 2);

        QString btnText = "Instalar";
        QString btnStyle = "QPushButton { background-color: #e3f2fd; color: #0d47a1; border: 1px solid #90caf9; "
                           "border-radius: 3px; padding: 2px 10px; font-weight: bold; }"
                           "QPushButton:hover { background-color: #1565c0; color: white; }";

        if (pkg.hasUpdate) {
            btnText = "Actualizar";
            btnStyle = "QPushButton { background-color: #fff3e0; color: #e65100; border: 1px solid #ffb74d; "
                       "border-radius: 3px; padding: 2px 10px; font-weight: bold; }"
                       "QPushButton:hover { background-color: #ef6c00; color: white; }";
        } else if (!pkg.installedFull.isEmpty()) {
            btnText = "Reinstalar";
            btnStyle = "QPushButton { background-color: #f5f5f5; color: #424242; border: 1px solid #bdbdbd; "
                       "border-radius: 3px; padding: 2px 10px; }"
                       "QPushButton:hover { background-color: #616161; color: white; }";
        }

        QPushButton *btnAction = new QPushButton(btnText, cellWidget);
        btnAction->setCursor(Qt::PointingHandCursor);
        btnAction->setStyleSheet(btnStyle);
        connect(btnAction, &QPushButton::clicked, this, [this, pkg]() {
            onInstallSboClicked(pkg);
        });

        cellLayout->addWidget(btnAction);
        m_sboTable->setCellWidget(row, 5, cellWidget);
    }

    m_sboTable->setUpdatesEnabled(true);
}

void MainWindow::resolveDepsRecursive(const QString &pkgName,
                                      const QMap<QString, QString> &installedMap,
                                      QSet<QString> &visited,
                                      QStringList &orderedBuildList) {
    if (visited.contains(pkgName)) return;
    visited.insert(pkgName);

    if (!m_sboCatalog.contains(pkgName)) return;

    const SboPackageInfo &info = m_sboCatalog[pkgName];
    for (const QString &dep : info.requires) {
        if (!installedMap.contains(dep)) {
            resolveDepsRecursive(dep, installedMap, visited, orderedBuildList);
        }
    }

    orderedBuildList.append(pkgName);
}

QStringList MainWindow::resolveSboDependencies(const QString &pkgName, const QMap<QString, QString> &installedMap) {
    QSet<QString> visited;
    QStringList orderedBuildList;
    resolveDepsRecursive(pkgName, installedMap, visited, orderedBuildList);
    return orderedBuildList;
}

void MainWindow::onInstallSboClicked(const SboPackageInfo &pkg) {
    QMap<QString, QString> installedMap;
    QDir pkgDir("/var/log/packages");
    for (const QString &fileName : pkgDir.entryList(QDir::Files)) {
        QStringList parts = fileName.split('-');
        if (parts.size() >= 4) {
            QString shortName = parts.mid(0, parts.size() - 3).join('-');
            installedMap.insert(shortName, fileName);
        }
    }

    QStringList buildQueue = resolveSboDependencies(pkg.name, installedMap);

    QDialog dialog(this);
    dialog.setWindowTitle(QString("Resolución de Dependencias SBo - %1").arg(pkg.name));
    dialog.resize(620, 400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QString summaryText;
    if (buildQueue.size() == 1) {
        summaryText = QString("El paquete <b>%1 (%2)</b> tiene todas sus dependencias satisfechas.<br>"
                              "Se compilará e instalará directamente mediante <code>sbopkg</code>:")
                          .arg(pkg.name, pkg.version);
    } else {
        summaryText = QString("Para instalar <b>%1 (%2)</b> se detectaron <b>%3 dependencia(s) faltante(s)</b>.<br>"
                              "<code>sbopkg</code> compilará e instalará automáticamente la cola en este orden estricto:")
                          .arg(pkg.name, pkg.version)
                          .arg(buildQueue.size() - 1);
    }

    QLabel *lblInfo = new QLabel(summaryText, &dialog);
    lblInfo->setWordWrap(true);
    layout->addWidget(lblInfo);

    QListWidget *listWidget = new QListWidget(&dialog);
    for (int i = 0; i < buildQueue.size(); ++i) {
        const QString &itemName = buildQueue[i];
        QString ver = m_sboCatalog.contains(itemName) ? m_sboCatalog[itemName].version : "";
        QString tag = (i == buildQueue.size() - 1) ? "[Paquete Principal]" : "[Dependencia]";
        QListWidgetItem *li = new QListWidgetItem(
            QString("%1. %2 (%3)  %4").arg(i + 1).arg(itemName, ver, tag),
            listWidget
        );
        if (i == buildQueue.size() - 1) {
            QFont f = li->font();
            f.setBold(true);
            li->setFont(f);
        }
    }
    layout->addWidget(listWidget);

    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *btnCancel = new QPushButton("Cancelar", &dialog);
    QPushButton *btnBuild = new QPushButton("Compilar e Instalar Cola (sbopkg)", &dialog);
    btnBuild->setStyleSheet("background-color: #1565c0; color: white; font-weight: bold; padding: 6px 16px;");

    btnRow->addStretch();
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnBuild);
    layout->addLayout(btnRow);

    connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(btnBuild, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted) return;

    QString sqfContent = buildQueue.join("\n") + "\n";
    QString sqfName = QString("qslackpkg_%1.sqf").arg(pkg.name);

    if (!m_btnToggleConsole->isChecked()) {
        m_btnToggleConsole->setChecked(true);
        toggleConsole();
    }

    QString shellCmd = QString(
        "mkdir -p /var/lib/sbopkg/queues && "
        "printf '%1' > /var/lib/sbopkg/queues/%2 && "
        "/usr/sbin/sbopkg -B -i %2"
    ).arg(sqfContent.replace("'", "'\\''"), sqfName);

    executePrivilegedCommand("/bin/sh", {"-c", shellCmd},
                             QString("Compilando e instalando %1 (%2 paquete(s) en cola)...").arg(pkg.name).arg(buildQueue.size()));
}

void MainWindow::runSboSync() {
    if (!QFile::exists("/usr/sbin/sbopkg")) {
        scanSboRepository();
        return;
    }

    if (!m_btnToggleConsole->isChecked()) {
        m_btnToggleConsole->setChecked(true);
        toggleConsole();
    }

    executePrivilegedCommand("/usr/sbin/sbopkg", {"-r"}, "Sincronizando repositorio local de SlackBuilds.org...");
}

// ============================================================================
// EJECUCIÓN PRIVILEGIADA UNIFICADA (PROXY + PKEXEC)
// ============================================================================

void MainWindow::executePrivilegedCommand(const QString &binaryPath, const QStringList &args, const QString &statusMsg) {
    if (m_process->state() != QProcess::NotRunning) return;

    m_consoleOutput->clear();
    setUiEnabled(false);

    QMap<QString, QString> proxyVars = loadProxyEnvironment();
    QStringList fullArgs;

    if (!proxyVars.isEmpty()) {
        fullArgs << "/usr/bin/env";
        for (auto it = proxyVars.constBegin(); it != proxyVars.constEnd(); ++it) {
            fullArgs << QString("%1=%2").arg(it.key(), it.value());
        }
    }

    fullArgs << binaryPath << args;
    m_statusBanner->setText(statusMsg);
    m_process->start("pkexec", fullArgs);
}

void MainWindow::runUpdate() {
    executePrivilegedCommand("/usr/sbin/slackpkg", {"-batch=on", "-default_answer=y", "update"},
                             "Sincronizando listas oficiales con slackpkg...");
}

void MainWindow::runUpgradeAll() {
    if (m_confirmedUpdates.isEmpty()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirmar Actualización",
        QString("¿Deseas instalar los %1 paquete(s) confirmados por slackpkg?").arg(m_confirmedUpdates.size()),
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        executePrivilegedCommand("/usr/sbin/slackpkg", {"-batch=on", "-default_answer=y", "upgrade-all"},
                                 "Aplicando actualizaciones del sistema...");
    }
}

void MainWindow::runCleanSystem() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirmar Limpieza",
        "¡Atención! Esto eliminará paquetes que no estén en el árbol oficial (incluyendo paquetes SBo).\n¿Estás seguro?",
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        executePrivilegedCommand("/usr/sbin/slackpkg", {"-batch=on", "-default_answer=y", "clean-system"},
                                 "Limpiando paquetes obsoletos...");
    }
}

void MainWindow::runRemovePackage(const QString &shortName, const QString &fullName) {
    QMessageBox::StandardButton reply = QMessageBox::warning(
        this, "Eliminar Paquete",
        QString("¿Estás seguro de que deseas desinstalar el siguiente paquete?\n\n"
                "Paquete: %1\n"
                "Comando: slackpkg remove %2").arg(fullName, shortName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        executePrivilegedCommand("/usr/sbin/slackpkg", {"-batch=on", "-default_answer=y", "remove", shortName},
                                 QString("Eliminando paquete %1...").arg(shortName));
    }
}

void MainWindow::readProcessOutput() {
    QByteArray data = m_process->readAllStandardOutput();
    QString text = stripAnsiCodes(QString::fromUtf8(data));

    m_consoleOutput->moveCursor(QTextCursor::End);
    m_consoleOutput->insertPlainText(text);
    m_consoleOutput->verticalScrollBar()->setValue(m_consoleOutput->verticalScrollBar()->maximum());
}

void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    setUiEnabled(true);
    m_consoleOutput->append(QString("\n>>> Proceso finalizado (Código: %1)").arg(exitCode));

    scanPendingUpgrades();
    scanSboRepository();
}

void MainWindow::setUiEnabled(bool enabled) {
    m_btnUpdate->setEnabled(enabled);
    m_btnClean->setEnabled(enabled);
    m_btnInstallSbopkg->setEnabled(enabled && !QFile::exists("/usr/sbin/sbopkg"));
    m_btnSboSync->setEnabled(enabled && QFile::exists("/usr/sbin/sbopkg"));
    m_packageTable->setEnabled(enabled);
    m_sboTable->setEnabled(enabled);
    m_progressBar->setVisible(!enabled);
    if (!enabled) {
        m_btnUpgrade->setEnabled(false);
        m_btnPendingDetails->setEnabled(false);
    }
}

QString MainWindow::stripAnsiCodes(const QString &text) {
    static QRegularExpression ansiRegex("\x1b\\[[0-9;]*[a-zA-Z]");
    QString clean = text;
    return clean.remove(ansiRegex);
}

QMap<QString, QString> MainWindow::loadProxyEnvironment() {
    QMap<QString, QString> proxyVars;
    const QStringList targetKeys = {"http_proxy", "https_proxy", "ftp_proxy", "no_proxy",
                                    "HTTP_PROXY", "HTTPS_PROXY", "FTP_PROXY", "NO_PROXY"};

    for (const QString &key : targetKeys) {
        QByteArray val = qgetenv(key.toUtf8().constData());
        if (!val.isEmpty()) proxyVars.insert(key, QString::fromUtf8(val));
    }

    QStringList candidateFiles = {"/etc/environment", "/etc/enviroment"};
    for (const QString &filePath : candidateFiles) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.isEmpty() || line.startsWith('#')) continue;
                if (line.startsWith("export ")) line = line.mid(7).trimmed();

                int eqIndex = line.indexOf('=');
                if (eqIndex > 0) {
                    QString key = line.left(eqIndex).trimmed();
                    QString value = line.mid(eqIndex + 1).trimmed();
                    if ((value.startsWith('"') && value.endsWith('"')) ||
                        (value.startsWith('\'') && value.endsWith('\''))) {
                        value = value.mid(1, value.length() - 2);
                    }
                    if (targetKeys.contains(key)) proxyVars.insert(key, value);
                }
            }
            file.close();
            break;
        }
    }
    return proxyVars;
}
