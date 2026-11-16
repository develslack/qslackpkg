#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QMessageBox>
#include <QDialog>
#include <QListWidget>
#include <QSplitter>
#include <QInputDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include <QRegularExpression>
#include <QNetworkProxy>
#include <QUrl>
#include <QDateTime>
#include <QApplication>
#include <QStyle>
#include <QCollator>
#include <QPixmap>
#include <QIcon>
#include <QPainter>
#include <QSysInfo>
#include <unistd.h>
#include "mainwindow.h"
#include "kernelconfigdialog.h"
#include "usbcreatorwidget.h"
#include "multilibwidget.h"
#include "systemhealthwidget.h"
#include "version.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("QSlackpkg - Package & Kernel Manager for Slackware Linux");
    resize(1340, 810);

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
    // 0. CABECERA VISUAL + INDICADOR DE SESIÓN / PRIVILEGIOS
    // ========================================================================
    QWidget *brandHeader = new QWidget(this);
    QVBoxLayout *brandLayout = new QVBoxLayout(brandHeader);
    brandLayout->setContentsMargins(0, 4, 0, 4);
    brandLayout->setSpacing(4);
    brandLayout->setAlignment(Qt::AlignCenter);


    QLabel *lblLogo = new QLabel(this);
    lblLogo->setAlignment(Qt::AlignCenter);
    if (!logoPixmap.isNull()) {
        lblLogo->setPixmap(logoPixmap.scaledToHeight(62, Qt::SmoothTransformation));
    } else {
        lblLogo->setText("SLACKWARE LINUX");
        lblLogo->setStyleSheet("font-size: 18pt; font-weight: bold; color: #1a237e;");
    }

    QLabel *lblSubtitle = new QLabel("Package & Kernel Manager for Slackware Linux", this);
    lblSubtitle->setAlignment(Qt::AlignCenter);
    lblSubtitle->setStyleSheet(
        "font-size: 11.5pt; font-weight: bold; color: #37474f; "
        "letter-spacing: 0.5px;"
    );

    m_lblPrivilegeBadge = new QLabel(this);
    m_lblPrivilegeBadge->setAlignment(Qt::AlignCenter);
    if (geteuid() == 0) {
        m_lblPrivilegeBadge->setText("Sesión activa como ROOT (UID 0) — Confirmación obligatoria antes de aplicar cambios");
        m_lblPrivilegeBadge->setStyleSheet(
            "background-color: #ffebee; color: #b71c1c; border: 1px solid #ef9a9a; "
            "border-radius: 10px; padding: 3px 14px; font-size: 8.8pt; font-weight: bold;"
        );
    } else {
        m_lblPrivilegeBadge->setText("Modo Informativo (Usuario estándar) — Se exigirá contraseña de ROOT para realizar cualquier operación");
        m_lblPrivilegeBadge->setStyleSheet(
            "background-color: #eceff1; color: #37474f; border: 1px solid #b0bec5; "
            "border-radius: 10px; padding: 3px 14px; font-size: 8.8pt; font-weight: bold;"
        );
    }

    brandLayout->addWidget(lblLogo);
    brandLayout->addWidget(lblSubtitle);
    QHBoxLayout *badgeAndAboutRow = new QHBoxLayout();
    badgeAndAboutRow->setAlignment(Qt::AlignCenter);
    badgeAndAboutRow->setSpacing(10);

    m_btnAbout = new QPushButton("Acerca de...", this);
    m_btnAbout->setCursor(Qt::PointingHandCursor);
    m_btnAbout->setToolTip("Información de versión, desarrollador y tecnologías utilizadas");
    m_btnAbout->setStyleSheet(
        "QPushButton { background-color: #e3f2fd; color: #0d47a1; border: 1px solid #90caf9; "
        "border-radius: 10px; padding: 3px 12px; font-size: 8.8pt; font-weight: bold; }"
        "QPushButton:hover { background-color: #1565c0; color: white; }"
    );
    connect(m_btnAbout, &QPushButton::clicked, this, &MainWindow::showAboutDialog);

    badgeAndAboutRow->addWidget(m_lblPrivilegeBadge);
    badgeAndAboutRow->addWidget(m_btnAbout);
    brandLayout->addLayout(badgeAndAboutRow);

    mainLayout->addWidget(brandHeader);

    // ========================================================================
    // 1. BANNER SUPERIOR DE ESTADO
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

    // Debe ir ANTES de crear m_tabs y m_usbCreatorWidget:
    m_netManager = new QNetworkAccessManager(this);

    // ========================================================================
    // 2. PESTAÑAS PRINCIPALES (Oficial | SlackBuilds | AlienBOB | Kernel)
    // ========================================================================
    m_tabs = new QTabWidget(this);
    m_tabs->addTab(createOfficialTab(), "Sistema Oficial (slackpkg)");
    m_tabs->addTab(createSboTab(), "Repositorio SlackBuilds.org (sbopkg)");
    m_tabs->addTab(createAlienTab(), "Repositorio AlienBOB (Binarios)");
    m_tabs->addTab(createKernelTab(), "Kernel y Arranque (initrd / Bootloader)");
    m_usbCreatorWidget = new UsbCreatorWidget(m_netManager, getMirrorUrl(), this);
    m_tabs->addTab(m_usbCreatorWidget, "Creador USB Booteable");
    mainLayout->addWidget(m_tabs);

    connect(m_usbCreatorWidget, &UsbCreatorWidget::usbCreationCompleted,
            this, [this](const QString &msg) {
        if (m_trayIcon && m_trayIcon->isVisible()) {
            m_trayIcon->showMessage(
                "USB Booteable de Slackware Listo",
                msg,
                QSystemTrayIcon::Information,
                8000
            );
        }
    });

    m_multilibWidget = new MultilibWidget(m_netManager, getMirrorUrl(), this);
    m_tabs->addTab(m_multilibWidget, "Multilib (32-bit)");

    m_systemHealthWidget = new SystemHealthWidget(this);
    m_tabs->addTab(m_systemHealthWidget, "Salud y Limpieza");

    connect(m_multilibWidget, &MultilibWidget::multilibStateChanged,
            this, &MainWindow::scanPendingUpgrades);
    connect(m_systemHealthWidget, &SystemHealthWidget::systemCleaned,
            this, &MainWindow::scanKernelEnvironment);

    // ========================================================================
    // 3. CONSOLA COMPARTIDA Y BARRA DE PROGRESO
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
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("%p% - Procesando...");
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    setCentralWidget(centralWidget);

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

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

    connect(m_btnPendingDetails, &QPushButton::clicked, this, &MainWindow::showPendingUpdatesDialog);
    connect(m_btnToggleConsole, &QPushButton::clicked, this, &MainWindow::toggleConsole);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MainWindow::readProcessOutput);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::processFinished);

    m_checkTimer = new QTimer(this);
    connect(m_checkTimer, &QTimer::timeout, this, &MainWindow::checkUpdatesInBackground);
    m_checkTimer->start(3600000);

    // Escaneo inicial de todo el sistema
    scanKernelEnvironment();
    scanPendingUpgrades();
    refreshDotNewBadge();
    scanSboRepository();
    loadAlienCatalogFromCache();
    QTimer::singleShot(3000, this, &MainWindow::checkUpdatesInBackground);
}

// ============================================================================
// PESTAÑA 1: SISTEMA OFICIAL (SLACKPKG + .NEW + BLACKLIST + BUSCADOR)
// ============================================================================

QWidget* MainWindow::createOfficialTab() {
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);

    // Fila 1: Acciones principales de slackpkg + Herramientas de mantenimiento (.new y blacklist)
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_btnUpdate = new QPushButton("1. Sincronizar listas (slackpkg update)", this);
    m_btnUpgrade = new QPushButton("2. Actualizar sistema (upgrade-all)", this);
    m_btnClean = new QPushButton("3. Limpiar obsoletos (clean-system)", this);

    m_btnDotNewManager = new QPushButton("Revisar archivos .new (0)", this);
    m_btnDotNewManager->setCursor(Qt::PointingHandCursor);
    m_btnDotNewManager->setToolTip(
        "Escanea /etc en busca de archivos de configuración *.new generados tras actualizaciones "
        "y permite comparar diferencias (diff), conservar o aplicar los nuevos."
    );

    m_btnBlacklistManager = new QPushButton("Blacklist", this);
    m_btnBlacklistManager->setCursor(Qt::PointingHandCursor);
    m_btnBlacklistManager->setToolTip("Gestionar paquetes bloqueados en /etc/slackpkg/blacklist");
    m_btnBlacklistManager->setStyleSheet(
        "QPushButton { background-color: #eceff1; color: #37474f; border: 1px solid #b0bec5; "
        "border-radius: 4px; padding: 5px 10px; font-weight: bold; }"
        "QPushButton:hover { background-color: #cfd8dc; }"
    );

    buttonLayout->addWidget(m_btnUpdate);
    buttonLayout->addWidget(m_btnUpgrade);
    buttonLayout->addWidget(m_btnClean);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_btnDotNewManager);
    buttonLayout->addWidget(m_btnBlacklistManager);
    layout->addLayout(buttonLayout);

    // Fila 2: Buscador rápido en tiempo real + Filtro de origen/estado
    QHBoxLayout *filterLayout = new QHBoxLayout();
    QLabel *lblSearch = new QLabel("Buscar paquete instalado:", this);
    m_editOfficialSearch = new QLineEdit(this);
    m_editOfficialSearch->setPlaceholderText("Ej: kernel, glibc, mesa, firefox, sddm, pipewire... (Doble clic en una fila para ver detalles)");
    m_editOfficialSearch->setClearButtonEnabled(true);

    QLabel *lblFilter = new QLabel("Mostrar:", this);
    m_comboOfficialFilter = new QComboBox(this);
    m_comboOfficialFilter->setMinimumWidth(210);
    m_comboOfficialFilter->addItem("Todos los paquetes instalados", "all");
    m_comboOfficialFilter->addItem("Solo con actualización disponible", "updates");
    m_comboOfficialFilter->addItem("Solo paquetes SBo (_SBo)", "sbo");
    m_comboOfficialFilter->addItem("Solo paquetes Externos (AlienBOB / otros)", "external");
    m_comboOfficialFilter->addItem("Solo bloqueados (Blacklist)", "blacklisted");

    m_lblOfficialCount = new QLabel("0 paquetes", this);
    m_lblOfficialCount->setStyleSheet("color: #37474f; font-weight: bold; padding-left: 8px;");

    filterLayout->addWidget(lblSearch);
    filterLayout->addWidget(m_editOfficialSearch, 1);
    filterLayout->addSpacing(10);
    filterLayout->addWidget(lblFilter);
    filterLayout->addWidget(m_comboOfficialFilter);
    filterLayout->addWidget(m_lblOfficialCount);
    layout->addLayout(filterLayout);

    // Tabla principal de 6 columnas
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
    connect(m_btnDotNewManager, &QPushButton::clicked, this, &MainWindow::showDotNewManagerDialog);
    connect(m_btnBlacklistManager, &QPushButton::clicked, this, &MainWindow::showBlacklistManagerDialog);
    connect(m_editOfficialSearch, &QLineEdit::textChanged, this, &MainWindow::filterOfficialTable);
    connect(m_comboOfficialFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::filterOfficialTable);
    connect(m_packageTable, &QTableWidget::cellDoubleClicked,
            this, &MainWindow::showPackageDetailsDialog);

    return tab;
}

void MainWindow::filterOfficialTable() {
    QString query = m_editOfficialSearch->text().trimmed();
    QString filterType = m_comboOfficialFilter->currentData().toString();

    int totalRows = m_packageTable->rowCount();
    int visibleRows = 0;

    for (int row = 0; row < totalRows; ++row) {
        QTableWidgetItem *nameItem = m_packageTable->item(row, 0);
        QTableWidgetItem *baseItem = m_packageTable->item(row, 1);
        QTableWidgetItem *sboItem = m_packageTable->item(row, 2);
        QTableWidgetItem *instItem = m_packageTable->item(row, 3);
        QTableWidgetItem *statusItem = m_packageTable->item(row, 4);
        if (!nameItem || !instItem || !statusItem) continue;

        QString rowCategory = nameItem->data(Qt::UserRole).toString();
        bool matchesType = true;
        if (filterType != "all") {
            matchesType = (rowCategory == filterType);
        }

        bool matchesSearch = true;
        if (!query.isEmpty()) {
            matchesSearch = nameItem->text().contains(query, Qt::CaseInsensitive) ||
                            instItem->text().contains(query, Qt::CaseInsensitive) ||
                            statusItem->text().contains(query, Qt::CaseInsensitive) ||
                            (baseItem && baseItem->text().contains(query, Qt::CaseInsensitive)) ||
                            (sboItem && sboItem->text().contains(query, Qt::CaseInsensitive));
        }

        bool show = matchesType && matchesSearch;
        m_packageTable->setRowHidden(row, !show);
        if (show) visibleRows++;
    }

    m_lblOfficialCount->setText(QString("Mostrando %1 de %2").arg(visibleRows).arg(totalRows));
}

void MainWindow::showPackageDetailsDialog(int row, int column) {
    Q_UNUSED(column);
    QTableWidgetItem *instItem = m_packageTable->item(row, 3);
    if (!instItem) return;

    QString pkgFullName = instItem->text();
    QString pkgLogPath = "/var/log/packages/" + pkgFullName;

    QFile file(pkgLogPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Información no disponible",
                             "No se pudo leer el registro: " + pkgLogPath);
        return;
    }

    QString compressedSize = "—";
    QString uncompressedSize = "—";
    QString location = "—";
    QStringList descriptionLines;
    QStringList fileList;
    bool inDesc = false;
    bool inFiles = false;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("COMPRESSED PACKAGE SIZE:")) {
            compressedSize = line.mid(24).trimmed();
        } else if (line.startsWith("UNCOMPRESSED PACKAGE SIZE:")) {
            uncompressedSize = line.mid(26).trimmed();
        } else if (line.startsWith("PACKAGE LOCATION:")) {
            location = line.mid(17).trimmed();
        } else if (line.startsWith("PACKAGE DESCRIPTION:")) {
            inDesc = true;
            inFiles = false;
        } else if (line.startsWith("FILE LIST:")) {
            inDesc = false;
            inFiles = true;
        } else if (inDesc) {
            descriptionLines << line;
        } else if (inFiles) {
            if (!line.trimmed().isEmpty()) fileList << line;
        }
    }
    file.close();

    QDialog dialog(this);
    dialog.setWindowTitle("Inspección de Paquete - " + pkgFullName);
    dialog.resize(740, 520);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *lblHeader = new QLabel(
        QString("<b>Paquete:</b> %1<br>"
                "<b>Tamaño comprimido / instalado:</b> %2 / %3 &nbsp;|&nbsp; "
                "<b>Archivos instalados:</b> %4")
            .arg(pkgFullName, compressedSize, uncompressedSize)
            .arg(fileList.size()),
        &dialog
    );
    lblHeader->setStyleSheet("background-color: #eceff1; padding: 8px; border-radius: 4px;");
    layout->addWidget(lblHeader);

    layout->addWidget(new QLabel("<b>Descripción oficial (slack-desc):</b>", &dialog));
    QTextEdit *descView = new QTextEdit(&dialog);
    descView->setReadOnly(true);
    descView->setMaximumHeight(165);
    descView->setPlainText(descriptionLines.join("\n"));
    descView->setStyleSheet("font-family: monospace; font-size: 9pt; background-color: #fafafa;");
    layout->addWidget(descView);

    layout->addWidget(new QLabel("<b>Lista de archivos instalados en el sistema:</b>", &dialog));
    QTextEdit *filesView = new QTextEdit(&dialog);
    filesView->setReadOnly(true);
    filesView->setPlainText(fileList.join("\n"));
    filesView->setStyleSheet("font-family: monospace; font-size: 8.5pt; background-color: #1e1e1e; color: #d4d4d4;");
    layout->addWidget(filesView);

    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *btnClose = new QPushButton("Cerrar", &dialog);
    btnRow->addStretch();
    btnRow->addWidget(btnClose);
    layout->addLayout(btnRow);

    connect(btnClose, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.exec();
}

// ============================================================================
// GESTOR VISUAL DE ARCHIVOS .NEW (NEW-CONFIG)
// ============================================================================

QStringList MainWindow::scanDotNewFiles() const {
    QStringList found;
    QDir etcDir("/etc");
    if (!etcDir.exists()) return found;

    QDirIterator it("/etc", {"*.new"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        found.append(it.next());
    }
    found.sort();
    return found;
}

void MainWindow::refreshDotNewBadge() {
    QStringList dotNews = scanDotNewFiles();
    int count = dotNews.size();
    m_btnDotNewManager->setText(QString("Revisar archivos .new (%1)").arg(count));

    if (count > 0) {
        m_btnDotNewManager->setStyleSheet(
            "QPushButton { background-color: #fff3e0; color: #e65100; border: 1px solid #ffb74d; "
            "border-radius: 4px; padding: 5px 12px; font-weight: bold; }"
            "QPushButton:hover { background-color: #ef6c00; color: white; }"
        );
    } else {
        m_btnDotNewManager->setStyleSheet(
            "QPushButton { background-color: #e8f5e9; color: #1b5e20; border: 1px solid #a5d6a7; "
            "border-radius: 4px; padding: 5px 12px; font-weight: bold; }"
            "QPushButton:hover { background-color: #c8e6c9; }"
        );
    }
}

void MainWindow::showDotNewManagerDialog() {
    QStringList dotNews = scanDotNewFiles();
    if (dotNews.isEmpty()) {
        QMessageBox::information(
            this,
            "Archivos de configuración al día",
            "No se encontraron archivos *.new pendientes en /etc.\n\n"
            "Cuando una actualización de paquete incluya configuraciones nuevas, aparecerán aquí "
            "para que pueda compararlas visualmente."
        );
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QString("Gestor Visual de Archivos .new (%1 pendiente(s))").arg(dotNews.size()));
    dialog.resize(900, 560);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *lblIntro = new QLabel(
        "<b>Archivos de configuración nuevos detectados en <code>/etc</code>:</b> En Slackware, las actualizaciones "
        "no sobrescriben tus configuraciones personales; crean un archivo <code>.new</code> para que revises las diferencias.",
        &dialog
    );
    lblIntro->setWordWrap(true);
    lblIntro->setStyleSheet("background-color: #e3f2fd; color: #0d47a1; padding: 8px; border-radius: 4px;");
    layout->addWidget(lblIntro);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, &dialog);

    QListWidget *listFiles = new QListWidget(splitter);
    listFiles->setMinimumWidth(270);
    for (const QString &path : dotNews) {
        listFiles->addItem(path);
    }

    QTextEdit *diffView = new QTextEdit(splitter);
    diffView->setReadOnly(true);
    diffView->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: monospace; font-size: 9pt;");

    splitter->addWidget(listFiles);
    splitter->addWidget(diffView);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);

    auto loadDiffForCurrent = [&]() {
        QListWidgetItem *cur = listFiles->currentItem();
        if (!cur) {
            diffView->clear();
            return;
        }
        QString newPath = cur->text();
        QString origPath = newPath.left(newPath.length() - 4); // quitar ".new"

        if (!QFile::exists(origPath)) {
            QFile nf(newPath);
            QString content;
            if (nf.open(QIODevice::ReadOnly | QIODevice::Text)) {
                content = QString::fromUtf8(nf.readAll());
                nf.close();
            }
            diffView->setPlainText(
                QString("# El archivo original %1 aún no existe.\n# Contenido de %2:\n\n%3")
                    .arg(origPath, newPath, content)
            );
            return;
        }

        QProcess diffProc;
        diffProc.start("diff", {"-u", origPath, newPath});
        if (diffProc.waitForFinished(3000)) {
            QString out = QString::fromUtf8(diffProc.readAllStandardOutput());
            if (out.trimmed().isEmpty()) {
                diffView->setPlainText(
                    QString("# Ambos archivos (%1 y %2) son idénticos.\n# Puede conservar el actual de forma segura.")
                        .arg(origPath, newPath)
                );
            } else {
                diffView->setPlainText(out);
            }
        }
    };

    connect(listFiles, &QListWidget::currentRowChanged, &dialog, [&](int) {
        loadDiffForCurrent();
    });

    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *btnKeepOld = new QPushButton("Conservar actual (Eliminar .new) [root]", &dialog);
    btnKeepOld->setStyleSheet(
        "QPushButton { background-color: #eceff1; color: #263238; border: 1px solid #90a4ae; "
        "font-weight: bold; padding: 6px 12px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #cfd8dc; }"
    );

    QPushButton *btnOverwrite = new QPushButton("Aplicar .new (Respalda .orig) [root]", &dialog);
    btnOverwrite->setStyleSheet(
        "QPushButton { background-color: #ef6c00; color: white; font-weight: bold; "
        "padding: 6px 12px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #e65100; }"
    );

    QPushButton *btnClose = new QPushButton("Cerrar", &dialog);

    btnRow->addWidget(btnKeepOld);
    btnRow->addWidget(btnOverwrite);
    btnRow->addStretch();
    btnRow->addWidget(btnClose);
    layout->addLayout(btnRow);

    connect(btnKeepOld, &QPushButton::clicked, &dialog, [&]() {
        QListWidgetItem *cur = listFiles->currentItem();
        if (!cur) return;
        QString newPath = cur->text();
        QString cmd = QString("rm -f '%1'").arg(QString(newPath).replace("'", "'\\''"));
        dialog.accept();
        executePrivilegedCommand("/bin/sh", {"-c", cmd}, "Eliminando " + newPath + " como root...");
    });

    connect(btnOverwrite, &QPushButton::clicked, &dialog, [&]() {
        QListWidgetItem *cur = listFiles->currentItem();
        if (!cur) return;
        QString newPath = cur->text();
        QString origPath = newPath.left(newPath.length() - 4);
        QString escNew = QString(newPath).replace("'", "'\\''");
        QString escOrig = QString(origPath).replace("'", "'\\''");

        QString cmd = QString(
            "if [ -f '%1' ]; then cp -p '%1' '%1.orig'; fi && mv -f '%2' '%1'"
        ).arg(escOrig, escNew);
        dialog.accept();
        executePrivilegedCommand("/bin/sh", {"-c", cmd},
                                 "Aplicando " + newPath + " (respaldo en .orig) como root...");
    });

    connect(btnClose, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (listFiles->count() > 0) {
        listFiles->setCurrentRow(0);
    }
    dialog.exec();
}

// ============================================================================
// GESTOR VISUAL DE BLACKLIST (/etc/slackpkg/blacklist)
// ============================================================================

void MainWindow::showBlacklistManagerDialog() {
    QFile file("/etc/slackpkg/blacklist");
    QString currentContent;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        currentContent = QString::fromUtf8(file.readAll());
        file.close();
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Gestor de Blacklist (/etc/slackpkg/blacklist)");
    dialog.resize(660, 470);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *lblHint = new QLabel(
        "<b>Paquetes protegidos contra actualizaciones o limpieza:</b> Los patrones activos en este archivo "
        "son ignorados por <code>slackpkg upgrade-all</code> y <code>clean-system</code> (por ejemplo, para proteger "
        "su kernel o paquetes de terceros). Guardar cambios requiere contraseña de <b>root</b>.",
        &dialog
    );
    lblHint->setWordWrap(true);
    lblHint->setStyleSheet("background-color: #fff8e1; color: #5d4037; padding: 8px; border-radius: 4px;");
    layout->addWidget(lblHint);

    QHBoxLayout *quickRow = new QHBoxLayout();
    QPushButton *btnProtectKernel = new QPushButton("+ Bloquear paquetes kernel-*", &dialog);
    QPushButton *btnProtectSbo = new QPushButton("+ Bloquear todos los _SBo", &dialog);
    QPushButton *btnProtectAlien = new QPushButton("+ Bloquear todos los alien", &dialog);
    quickRow->addWidget(btnProtectKernel);
    quickRow->addWidget(btnProtectSbo);
    quickRow->addWidget(btnProtectAlien);
    quickRow->addStretch();
    layout->addLayout(quickRow);

    QTextEdit *editor = new QTextEdit(&dialog);
    editor->setPlainText(currentContent);
    editor->setStyleSheet("font-family: monospace; font-size: 9.5pt;");
    layout->addWidget(editor, 1);

    auto appendRuleIfMissing = [&](const QString &rule) {
        QString txt = editor->toPlainText();
        QStringList lines = txt.split('\n');
        for (const QString &l : lines) {
            if (l.trimmed() == rule) return;
        }
        if (!txt.endsWith('\n') && !txt.isEmpty()) txt += '\n';
        txt += rule + '\n';
        editor->setPlainText(txt);
    };

    connect(btnProtectKernel, &QPushButton::clicked, &dialog, [&]() {
        appendRuleIfMissing("kernel-generic");
        appendRuleIfMissing("kernel-huge");
        appendRuleIfMissing("kernel-modules");
        appendRuleIfMissing("kernel-headers");
        appendRuleIfMissing("kernel-source");
    });
    connect(btnProtectSbo, &QPushButton::clicked, &dialog, [&]() {
        appendRuleIfMissing("[0-9]+_SBo");
    });
    connect(btnProtectAlien, &QPushButton::clicked, &dialog, [&]() {
        appendRuleIfMissing("[0-9]+alien");
    });

    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *btnCancel = new QPushButton("Cancelar", &dialog);
    QPushButton *btnSave = new QPushButton("Guardar Blacklist (requiere root)", &dialog);
    btnSave->setStyleSheet("background-color: #1565c0; color: white; font-weight: bold; padding: 6px 14px;");
    btnRow->addStretch();
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnSave);
    layout->addLayout(btnRow);

    connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(btnSave, &QPushButton::clicked, &dialog, [&]() {
        QString newText = editor->toPlainText();
        QString tmpFile = "/tmp/qslackpkg_blacklist.tmp";
        QFile out(tmpFile);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(&dialog, "Error", "No se pudo crear archivo temporal en /tmp.");
            return;
        }
        out.write(newText.toUtf8());
        out.close();

        dialog.accept();
        QString cmd = QString(
            "cp -p /etc/slackpkg/blacklist /etc/slackpkg/blacklist.bak && "
            "install -m 644 '%1' /etc/slackpkg/blacklist && rm -f '%1'"
        ).arg(tmpFile);
        executePrivilegedCommand("/bin/sh", {"-c", cmd},
                                 "Guardando /etc/slackpkg/blacklist como root...");
    });

    dialog.exec();
}

// ============================================================================
// PESTAÑA 2: REPOSITORIO SLACKBUILDS.ORG (SBOPKG)
// ============================================================================

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

// ============================================================================
// PESTAÑA 3: REPOSITORIO ALIENBOB (BINARIOS PRECOMPILADOS DE ERIC HAMELEERS)
// ============================================================================

QWidget* MainWindow::createAlienTab() {
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);

    QHBoxLayout *topBar = new QHBoxLayout();

    m_btnAlienSync = new QPushButton("Sincronizar catálogo AlienBOB", this);
    m_btnAlienSync->setCursor(Qt::PointingHandCursor);
    m_btnAlienSync->setStyleSheet(
        "QPushButton { background-color: #1565c0; color: white; font-weight: bold; "
        "padding: 6px 14px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #0d47a1; }"
    );

    m_btnAlienUpgradeAll = new QPushButton("Actualizar todos (0)", this);
    m_btnAlienUpgradeAll->setCursor(Qt::PointingHandCursor);
    m_btnAlienUpgradeAll->setEnabled(false);
    m_btnAlienUpgradeAll->setToolTip("Actualiza en lote todos los paquetes AlienBOB instalados que tengan nueva versión (requiere root)");
    m_btnAlienUpgradeAll->setStyleSheet(
        "QPushButton { background-color: #ef6c00; color: white; font-weight: bold; "
        "padding: 6px 12px; border-radius: 4px; }"
        "QPushButton:hover:enabled { background-color: #e65100; }"
        "QPushButton:disabled { background-color: #eceff1; color: #90a4ae; }"
    );

    QLabel *lblBranch = new QLabel("Rama / Arq.:", this);
    m_comboAlienBranch = new QComboBox(this);
    m_comboAlienBranch->setMinimumWidth(160);
    m_comboAlienBranch->setToolTip(
        "Filtro automático según su versión de Slackware y arquitectura.\n"
        "Usuarios avanzados pueden explorar otras ramas si lo desean."
    );
    QString detectedBranch = detectAlienTargetBranch();
    m_comboAlienBranch->addItem(QString("%1 (Detectada)").arg(detectedBranch), detectedBranch);
    if (detectedBranch != "pkg64/15.0") m_comboAlienBranch->addItem("pkg64/15.0", "pkg64/15.0");
    if (detectedBranch != "pkg64/current") m_comboAlienBranch->addItem("pkg64/current", "pkg64/current");
    if (detectedBranch != "pkg/15.0") m_comboAlienBranch->addItem("pkg/15.0", "pkg/15.0");
    if (detectedBranch != "pkg/current") m_comboAlienBranch->addItem("pkg/current", "pkg/current");
    m_comboAlienBranch->addItem("Todas las ramas", "");

    QLabel *lblSearch = new QLabel("Buscar:", this);
    m_editAlienSearch = new QLineEdit(this);
    m_editAlienSearch->setPlaceholderText("Ej: vlc, libreoffice, chromium, wine, obs-studio, calibre...");
    m_editAlienSearch->setClearButtonEnabled(true);

    m_chkAlienInstalledOnly = new QCheckBox("Solo instalados", this);
    m_chkAlienUpdatesOnly = new QCheckBox("Solo con actualización", this);

    topBar->addWidget(m_btnAlienSync);
    topBar->addWidget(m_btnAlienUpgradeAll);
    topBar->addSpacing(8);
    topBar->addWidget(lblBranch);
    topBar->addWidget(m_comboAlienBranch);
    topBar->addSpacing(8);
    topBar->addWidget(lblSearch);
    topBar->addWidget(m_editAlienSearch);
    topBar->addWidget(m_chkAlienInstalledOnly);
    topBar->addWidget(m_chkAlienUpdatesOnly);
    layout->addLayout(topBar);

    QLabel *lblAlienHint = new QLabel(
        "<b>Paquetes binarios precompilados (AlienBOB):</b> Listos para instalar en segundos sin compilar código fuente. "
        "Cada descarga verifica automáticamente su suma <code>.md5</code> y firma <code>GPG-KEY</code> oficial, y solicita contraseña de <b>root</b> antes de instalar o eliminar. "
        "<i>(Pase el cursor sobre un paquete para ver su ruta exacta y dependencias).</i>",
        this
    );
    lblAlienHint->setWordWrap(true);
    lblAlienHint->setStyleSheet(
        "background-color: #e3f2fd; color: #0d47a1; border: 1px solid #90caf9; "
        "border-radius: 4px; padding: 6px 10px; font-size: 9pt;"
    );
    layout->addWidget(lblAlienHint);

    m_alienStatusLabel = new QLabel("Analizando caché local del repositorio AlienBOB...", this);
    m_alienStatusLabel->setStyleSheet("color: #37474f; font-weight: bold; padding: 4px;");
    layout->addWidget(m_alienStatusLabel);

    m_alienTable = new QTableWidget(0, 7, tab);
    m_alienTable->setHorizontalHeaderLabels({
        "Paquete",
        "Rama / Arq.",
        "Versión Instalada",
        "Versión AlienBOB",
        "Tamaño",
        "Descripción",
        "Acciones"
    });
    m_alienTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_alienTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    m_alienTable->setColumnWidth(0, 165);
    m_alienTable->setColumnWidth(1, 105);
    m_alienTable->setColumnWidth(2, 185);
    m_alienTable->setColumnWidth(3, 145);
    m_alienTable->setColumnWidth(4, 85);
    m_alienTable->setColumnWidth(6, 185);
    m_alienTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_alienTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_alienTable->setAlternatingRowColors(true);
    layout->addWidget(m_alienTable);

    connect(m_btnAlienSync, &QPushButton::clicked, this, &MainWindow::runAlienSync);
    connect(m_btnAlienUpgradeAll, &QPushButton::clicked, this, &MainWindow::runAlienUpgradeAll);
    connect(m_comboAlienBranch, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::filterAlienTable);
    connect(m_editAlienSearch, &QLineEdit::textChanged, this, &MainWindow::filterAlienTable);
    connect(m_chkAlienInstalledOnly, &QCheckBox::toggled, this, &MainWindow::filterAlienTable);
    connect(m_chkAlienUpdatesOnly, &QCheckBox::toggled, this, &MainWindow::filterAlienTable);

    return tab;
}

// ============================================================================
// PESTAÑA 4: KERNEL Y ARRANQUE (DIAGNÓSTICO, INITRD, BZIMAGE Y BOOTLOADER)
// ============================================================================

QWidget* MainWindow::createKernelTab() {
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *container = new QWidget(scrollArea);
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setSpacing(12);

    QString currentSessionNote = (geteuid() == 0)
        ? "Sesión actual detectada: <b>usuario root (UID 0)</b>."
        : "Sesión actual detectada: <b>usuario estándar (Modo Informativo)</b>. Sin ingresar la contraseña de <b>root</b> no se ejecutará ningún cambio.";

    QLabel *lblGlobalRootWarning = new QLabel(
        QString("<b>AVISO DE SEGURIDAD Y PRIVILEGIOS (ROOT):</b> Todas las operaciones de este apartado "
                "(compilación de <code>bzImage</code>, generación de <code>initrd</code> y actualización de "
                "<code>ELILO / LILO / GRUB</code>) modifican archivos críticos de arranque del sistema y "
                "<b>deben ser realizadas por el usuario root</b>. %1<br>"
                "<i>Antes de pulsar Aceptar en cualquier paso, lea atentamente todas las configuraciones seleccionadas.</i>")
            .arg(currentSessionNote),
        container
    );
    lblGlobalRootWarning->setWordWrap(true);
    lblGlobalRootWarning->setStyleSheet(
        "background-color: #fff8e1; color: #5d4037; border: 1px solid #ffb300; "
        "border-left: 5px solid #ff8f00; border-radius: 4px; padding: 10px; font-size: 9.5pt;"
    );
    layout->addWidget(lblGlobalRootWarning);

    m_kernelAlertCard = new QWidget(container);
    m_kernelAlertCard->setObjectName("kernelAlertCard");
    m_kernelAlertCard->setStyleSheet(
        "#kernelAlertCard { background-color: #fbe9e7; border: 1px solid #ff7043; "
        "border-left: 5px solid #d84315; border-radius: 4px; }"
    );
    QHBoxLayout *alertLayout = new QHBoxLayout(m_kernelAlertCard);
    alertLayout->setContentsMargins(12, 8, 12, 8);

    m_lblKernelAlertText = new QLabel(
        "¡Nuevo kernel detectado! Debe generar el initrd y actualizar el bootloader antes de reiniciar.",
        m_kernelAlertCard
    );
    m_lblKernelAlertText->setWordWrap(true);
    m_lblKernelAlertText->setStyleSheet("color: #bf360c; font-weight: bold; background: transparent;");

    m_btnKernelAlertSelect = new QPushButton("Seleccionar nuevo kernel", m_kernelAlertCard);
    m_btnKernelAlertSelect->setCursor(Qt::PointingHandCursor);
    m_btnKernelAlertSelect->setStyleSheet(
        "QPushButton { background-color: #d84315; color: white; font-weight: bold; "
        "padding: 6px 12px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #bf360c; }"
    );

    alertLayout->addWidget(m_lblKernelAlertText, 1);
    alertLayout->addWidget(m_btnKernelAlertSelect);
    m_kernelAlertCard->setVisible(false);
    layout->addWidget(m_kernelAlertCard);

    QGroupBox *grpDiag = new QGroupBox("1. Diagnóstico del Kernel, Firmware y Gestor de Arranque", container);
    QGridLayout *diagGrid = new QGridLayout(grpDiag);
    diagGrid->setHorizontalSpacing(18);
    diagGrid->setVerticalSpacing(6);

    m_lblDiagRunningKernel = new QLabel("Detectando...", grpDiag);
    m_lblDiagInstalledKernels = new QLabel("Detectando...", grpDiag);
    m_lblDiagKernelSources = new QLabel("Detectando...", grpDiag);
    m_lblDiagFirmwareMode = new QLabel("Detectando...", grpDiag);
    m_lblDiagBootloader = new QLabel("Detectando...", grpDiag);
    m_lblDiagDualBoot = new QLabel("Detectando...", grpDiag);

    m_lblDiagRunningKernel->setStyleSheet("font-weight: bold; color: #1b5e20;");
    m_lblDiagInstalledKernels->setWordWrap(true);
    m_lblDiagKernelSources->setWordWrap(true);
    m_lblDiagFirmwareMode->setWordWrap(true);
    m_lblDiagBootloader->setWordWrap(true);
    m_lblDiagDualBoot->setWordWrap(true);

    diagGrid->addWidget(new QLabel("<b>Kernel en ejecución (uname -r):</b>", grpDiag), 0, 0);
    diagGrid->addWidget(m_lblDiagRunningKernel, 0, 1);

    diagGrid->addWidget(new QLabel("<b>Kernels instalados (/boot):</b>", grpDiag), 1, 0);
    diagGrid->addWidget(m_lblDiagInstalledKernels, 1, 1);

    diagGrid->addWidget(new QLabel("<b>Fuentes del Kernel (/usr/src/linux):</b>", grpDiag), 2, 0);
    diagGrid->addWidget(m_lblDiagKernelSources, 2, 1);

    diagGrid->addWidget(new QLabel("<b>Firmware y Particionado:</b>", grpDiag), 0, 2);
    diagGrid->addWidget(m_lblDiagFirmwareMode, 0, 3);

    diagGrid->addWidget(new QLabel("<b>Bootloader activo:</b>", grpDiag), 1, 2);
    diagGrid->addWidget(m_lblDiagBootloader, 1, 3);

    diagGrid->addWidget(new QLabel("<b>Otros Sistemas (Dual-Boot):</b>", grpDiag), 2, 2);
    diagGrid->addWidget(m_lblDiagDualBoot, 2, 3);

    diagGrid->setColumnStretch(1, 1);
    diagGrid->setColumnStretch(3, 1);

    m_btnRescanKernel = new QPushButton("Re-escanear entorno", grpDiag);
    m_btnRescanKernel->setCursor(Qt::PointingHandCursor);
    diagGrid->addWidget(m_btnRescanKernel, 3, 3, Qt::AlignRight);

    layout->addWidget(grpDiag);

    QGroupBox *grpKernelImg = new QGroupBox("2. Versión Objetivo, Imagen del Kernel y Compilación a Medida", container);
    QVBoxLayout *imgBoxLayout = new QVBoxLayout(grpKernelImg);

    QLabel *lblStep2RootNote = new QLabel(
        "<i>Aviso de permisos: La instalación o compilación de imágenes de kernel (<code>bzImage</code>) y módulos en "
        "<code>/boot</code> y <code>/lib/modules</code> requiere ejecutarse como <b>root</b>. Verifique la versión antes de continuar.</i>",
        grpKernelImg
    );
    lblStep2RootNote->setWordWrap(true);
    lblStep2RootNote->setStyleSheet("color: #6d4c41; font-size: 8.8pt;");
    imgBoxLayout->addWidget(lblStep2RootNote);

    QFormLayout *imgForm = new QFormLayout();
    m_comboTargetKernel = new QComboBox(grpKernelImg);
    m_comboKernelVariant = new QComboBox(grpKernelImg);

    imgForm->addRow("<b>Versión de Kernel a configurar:</b>", m_comboTargetKernel);
    imgForm->addRow("<b>Imagen de Kernel a utilizar:</b>", m_comboKernelVariant);
    imgBoxLayout->addLayout(imgForm);

    QHBoxLayout *customKernelRow = new QHBoxLayout();
    m_btnOpenKernelConfigurator = new QPushButton(
        "Descargar, Configurar (Kconfig Visual) y Compilar Kernel a Medida...",
        grpKernelImg
    );
    m_btnOpenKernelConfigurator->setCursor(Qt::PointingHandCursor);
    m_btnOpenKernelConfigurator->setToolTip(
        "Abre el asistente visual para descargar fuentes desde Kernel.org, heredar los módulos activos de tu sistema "
        "(olddefconfig / localmodconfig), personalizar opciones Kconfig sin ncurses y compilar bzImage + initrd."
    );
    m_btnOpenKernelConfigurator->setStyleSheet(
        "QPushButton { background-color: #1565c0; color: white; font-weight: bold; "
        "padding: 7px 14px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #0d47a1; }"
    );
    customKernelRow->addStretch();
    customKernelRow->addWidget(m_btnOpenKernelConfigurator);
    imgBoxLayout->addLayout(customKernelRow);

    layout->addWidget(grpKernelImg);

    QGroupBox *grpInitrd = new QGroupBox("3. Generación de Imagen Inicial RAM (initrd)", container);
    QVBoxLayout *initrdBoxLayout = new QVBoxLayout(grpInitrd);

    QLabel *lblStep3RootNote = new QLabel(
        "<i>Aviso de permisos y seguridad: La creación de <code>/boot/initrd-&lt;versión&gt;.gz</code> escribe directamente en <code>/boot</code> "
        "como usuario <b>root</b>. Los parámetros de dispositivo raíz y sistema de archivos están <b>bloqueados por protección</b> "
        "con los valores autodetectados para evitar dejar el sistema sin arranque.</i>",
        grpInitrd
    );
    lblStep3RootNote->setWordWrap(true);
    lblStep3RootNote->setStyleSheet("color: #6d4c41; font-size: 8.8pt;");
    initrdBoxLayout->addWidget(lblStep3RootNote);

    QHBoxLayout *initrdHeaderRow = new QHBoxLayout();
    m_chkGenerateInitrd = new QCheckBox("Generar archivo /boot/initrd-<versión>.gz automáticamente (Recomendado para kernel-generic)", grpInitrd);
    m_chkGenerateInitrd->setChecked(true);
    m_chkGenerateInitrd->setStyleSheet("font-weight: bold;");

    m_chkUnlockStorageParams = new QCheckBox("Modo Avanzado: Desbloquear edición de dispositivo raíz, FS y módulos", grpInitrd);
    m_chkUnlockStorageParams->setChecked(false);
    m_chkUnlockStorageParams->setStyleSheet("color: #c62828; font-weight: bold;");

    initrdHeaderRow->addWidget(m_chkGenerateInitrd);
    initrdHeaderRow->addStretch();
    initrdHeaderRow->addWidget(m_chkUnlockStorageParams);
    initrdBoxLayout->addLayout(initrdHeaderRow);

    QString lockedFieldStyle =
        "QLineEdit:disabled { background-color: #eceff1; color: #37474f; font-weight: bold; "
        "border: 1px solid #b0bec5; border-radius: 3px; padding: 4px; }"
        "QLineEdit:enabled { background-color: #fffde7; color: #bf360c; font-weight: bold; "
        "border: 1px solid #e53935; border-radius: 3px; padding: 4px; }";

    QGridLayout *initrdGrid = new QGridLayout();
    m_editRootDevice = new QLineEdit(grpInitrd);
    m_editRootDevice->setEnabled(false);
    m_editRootDevice->setStyleSheet(lockedFieldStyle);
    m_editRootDevice->setToolTip("Bloqueado por seguridad. Muestra la partición raíz real detectada en el sistema.");

    m_editRootFs = new QLineEdit(grpInitrd);
    m_editRootFs->setEnabled(false);
    m_editRootFs->setStyleSheet(lockedFieldStyle);
    m_editRootFs->setToolTip("Bloqueado por seguridad. Muestra el sistema de archivos raíz real detectado.");

    m_editInitrdModules = new QLineEdit(grpInitrd);
    m_editInitrdModules->setPlaceholderText("(Autodetección automática de módulos de almacenamiento/FS)");
    m_editInitrdModules->setEnabled(false);
    m_editInitrdModules->setStyleSheet(lockedFieldStyle);
    m_editInitrdModules->setToolTip("Bloqueado por seguridad. Solo editable en Modo Avanzado.");

    m_chkIncludeUdev = new QCheckBox("Incluir soporte udev (-u) [Obligatorio/Estándar en Slackware]", grpInitrd);
    m_chkIncludeUdev->setChecked(true);
    m_chkIncludeUdev->setEnabled(false);

    initrdGrid->addWidget(new QLabel("<b>Partición Raíz (-r) [Protegido]:</b>", grpInitrd), 0, 0);
    initrdGrid->addWidget(m_editRootDevice, 0, 1);
    initrdGrid->addWidget(new QLabel("<b>Sistema de Archivos (-f) [Protegido]:</b>", grpInitrd), 0, 2);
    initrdGrid->addWidget(m_editRootFs, 0, 3);
    initrdGrid->addWidget(new QLabel("<b>Módulos del Kernel (-m) [Protegido]:</b>", grpInitrd), 1, 0);
    initrdGrid->addWidget(m_editInitrdModules, 1, 1);
    initrdGrid->addWidget(m_chkIncludeUdev, 1, 2, 1, 2);

    initrdBoxLayout->addLayout(initrdGrid);

    QHBoxLayout *initrdActionRow = new QHBoxLayout();
    m_btnRunInitrdOnly = new QPushButton("Ejecutar solo generación de initrd (requiere root)", grpInitrd);
    m_btnRunInitrdOnly->setCursor(Qt::PointingHandCursor);
    initrdActionRow->addStretch();
    initrdActionRow->addWidget(m_btnRunInitrdOnly);
    initrdBoxLayout->addLayout(initrdActionRow);

    layout->addWidget(grpInitrd);

    QGroupBox *grpBootloader = new QGroupBox("4. Actualización Visual del Gestor de Arranque (ELILO / LILO / GRUB)", container);
    QVBoxLayout *bootBoxLayout = new QVBoxLayout(grpBootloader);

    QLabel *lblStep4RootNote = new QLabel(
        "<i>Aviso de permisos: Modificar <code>elilo.conf</code>, ejecutar <code>/sbin/lilo</code> o regenerar "
        "<code>/boot/grub/grub.cfg</code> requiere ingresar contraseña de <b>root</b>. Se creará automáticamente un respaldo "
        "<code>.bak</code> antes de aplicar cambios. Lea la vista previa antes de aceptar.</i>",
        grpBootloader
    );
    lblStep4RootNote->setWordWrap(true);
    lblStep4RootNote->setStyleSheet("color: #6d4c41; font-size: 8.8pt;");
    bootBoxLayout->addWidget(lblStep4RootNote);

    QHBoxLayout *bootOptsRow = new QHBoxLayout();
    m_chkUpdateBootloader = new QCheckBox("Actualizar configuración del Bootloader detectado automáticamente", grpBootloader);
    m_chkUpdateBootloader->setChecked(true);
    m_chkUpdateBootloader->setStyleSheet("font-weight: bold;");

    m_chkKeepFallbackEntry = new QCheckBox("Conservar entrada anterior como respaldo arrancable (Slackware-Prev)", grpBootloader);
    m_chkKeepFallbackEntry->setChecked(true);

    bootOptsRow->addWidget(m_chkUpdateBootloader);
    bootOptsRow->addSpacing(16);
    bootOptsRow->addWidget(m_chkKeepFallbackEntry);
    bootOptsRow->addStretch();
    bootBoxLayout->addLayout(bootOptsRow);

    QLabel *lblPreviewTitle = new QLabel("Vista previa de la configuración / comandos que se aplicarán (solo lectura):", grpBootloader);
    lblPreviewTitle->setStyleSheet("color: #37474f; font-size: 9pt;");
    bootBoxLayout->addWidget(lblPreviewTitle);

    m_bootloaderPreview = new QTextEdit(grpBootloader);
    m_bootloaderPreview->setReadOnly(true);
    m_bootloaderPreview->setMaximumHeight(115);
    m_bootloaderPreview->setStyleSheet(
        "background-color: #263238; color: #eceff1; font-family: monospace; font-size: 9pt; border-radius: 4px;"
    );
    bootBoxLayout->addWidget(m_bootloaderPreview);

    QHBoxLayout *bootActionRow = new QHBoxLayout();
    m_btnRunBootloaderOnly = new QPushButton("Ejecutar solo actualización de Bootloader (requiere root)", grpBootloader);
    m_btnRunBootloaderOnly->setCursor(Qt::PointingHandCursor);

    m_btnRunFullKernelFlow = new QPushButton("Aplicar Configuración Completa (Kernel / initrd + Bootloader) como root", grpBootloader);
    m_btnRunFullKernelFlow->setCursor(Qt::PointingHandCursor);
    m_btnRunFullKernelFlow->setStyleSheet(
        "QPushButton { background-color: #2e7d32; color: white; font-weight: bold; "
        "padding: 8px 18px; border-radius: 4px; font-size: 10pt; }"
        "QPushButton:hover { background-color: #1b5e20; }"
        "QPushButton:disabled { background-color: #a5d6a7; }"
    );

    bootActionRow->addWidget(m_btnRunBootloaderOnly);
    bootActionRow->addStretch();
    bootActionRow->addWidget(m_btnRunFullKernelFlow);
    bootBoxLayout->addLayout(bootActionRow);

    layout->addWidget(grpBootloader);
    layout->addStretch();

    scrollArea->setWidget(container);

    connect(m_btnRescanKernel, &QPushButton::clicked, this, &MainWindow::scanKernelEnvironment);
    connect(m_btnKernelAlertSelect, &QPushButton::clicked, this, [this]() {
        int idx = m_comboTargetKernel->findData(m_kernelInfo.latestInstalledKernel);
        if (idx >= 0) m_comboTargetKernel->setCurrentIndex(idx);
    });
    connect(m_comboTargetKernel, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onKernelSelectionChanged);
    connect(m_comboKernelVariant, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onKernelSelectionChanged);
    connect(m_btnOpenKernelConfigurator, &QPushButton::clicked,
            this, &MainWindow::openKernelConfiguratorDialog);

    connect(m_chkUnlockStorageParams, &QCheckBox::toggled, this, &MainWindow::onToggleUnlockStorageParams);

    connect(m_chkGenerateInitrd, &QCheckBox::toggled, this, &MainWindow::updateBootloaderPreview);
    connect(m_editRootDevice, &QLineEdit::textChanged, this, &MainWindow::updateBootloaderPreview);
    connect(m_editRootFs, &QLineEdit::textChanged, this, &MainWindow::updateBootloaderPreview);
    connect(m_editInitrdModules, &QLineEdit::textChanged, this, &MainWindow::updateBootloaderPreview);
    connect(m_chkIncludeUdev, &QCheckBox::toggled, this, &MainWindow::updateBootloaderPreview);
    connect(m_chkUpdateBootloader, &QCheckBox::toggled, this, &MainWindow::updateBootloaderPreview);
    connect(m_chkKeepFallbackEntry, &QCheckBox::toggled, this, &MainWindow::updateBootloaderPreview);

    connect(m_btnRunInitrdOnly, &QPushButton::clicked, this, &MainWindow::runGenerateInitrdOnly);
    connect(m_btnRunBootloaderOnly, &QPushButton::clicked, this, &MainWindow::runUpdateBootloaderOnly);
    connect(m_btnRunFullKernelFlow, &QPushButton::clicked, this, &MainWindow::runFullKernelWorkflow);

    return scrollArea;
}

void MainWindow::openKernelConfiguratorDialog() {
    KernelConfigDialog dialog(
        m_netManager,
        m_editRootDevice->text().trimmed(),
        m_editRootFs->text().trimmed(),
        m_kernelInfo.bootloaderType,
        m_kernelInfo.bootloaderConfigPath,
        m_kernelInfo.efiSlackwareDir,
        m_chkKeepFallbackEntry->isChecked(),
        this
    );

    connect(&dialog, &KernelConfigDialog::environmentChanged,
            this, &MainWindow::scanKernelEnvironment);

    connect(&dialog, &KernelConfigDialog::kernelInstalledNotification,
            this, [this](const QString &newVer) {
        QWidget *bannerContainer = findChild<QWidget*>("bannerContainer");
        if (bannerContainer) {
            bannerContainer->setStyleSheet("#bannerContainer { background-color: #1565c0; border-radius: 6px; }");
        }
        m_statusBanner->setText(
            QString("¡Nuevo Kernel (%1) compilado e instalado! Reinicie el equipo cuando desee utilizarlo.")
                .arg(newVer)
        );
        updateTrayIconState(StateUpdatesAvailable, 1);
        if (m_trayIcon && m_trayIcon->isVisible()) {
            m_trayIcon->showMessage(
                "Nuevo Kernel listo para usar",
                QString("El kernel %1 se ha compilado e instalado en %2. Reinicie su equipo para iniciar con él.")
                    .arg(newVer, m_kernelInfo.bootloaderType),
                QSystemTrayIcon::Information,
                10000
            );
        }
    });

    dialog.exec();
}

void MainWindow::onToggleUnlockStorageParams(bool checked) {
    if (checked) {
        QMessageBox::StandardButton resp = QMessageBox::warning(
            this,
            "¡ADVERTENCIA CRÍTICA DE SEGURIDAD!",
            "Está a punto de desbloquear la edición manual de:\n"
            "  • Partición Raíz (ej. /dev/sda2)\n"
            "  • Sistema de Archivos (ej. ext4, reiserfs, xfs)\n"
            "  • Módulos del Kernel y soporte udev\n\n"
            "Si introduce un dispositivo o sistema de archivos incorrecto, Slackware NO podrá arrancar "
            "y producirá un Kernel Panic.\n\n"
            "¿Es usted un usuario avanzado y está seguro de querer habilitar estos campos?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

        if (resp != QMessageBox::Yes) {
            m_chkUnlockStorageParams->blockSignals(true);
            m_chkUnlockStorageParams->setChecked(false);
            m_chkUnlockStorageParams->blockSignals(false);
            return;
        }
    } else {
        m_editRootDevice->setText(m_kernelInfo.rootDevice);
        m_editRootFs->setText(m_kernelInfo.rootFsType);
        m_editInitrdModules->setText(m_kernelInfo.recommendedModules);
        m_chkIncludeUdev->setChecked(true);
    }

    m_editRootDevice->setEnabled(checked);
    m_editRootFs->setEnabled(checked);
    m_editInitrdModules->setEnabled(checked);
    m_chkIncludeUdev->setEnabled(checked);
    updateBootloaderPreview();
}

bool MainWindow::validateCriticalBootFields() {
    QString rootDev = m_editRootDevice->text().trimmed();
    QString rootFs = m_editRootFs->text().trimmed();

    if (rootDev.isEmpty() || rootFs.isEmpty()) {
        QMessageBox::critical(
            this, "Configuración de Arranque Inválida",
            "La partición raíz (-r) y el sistema de archivos (-f) no pueden estar vacíos.\n"
            "Operación cancelada para proteger el arranque del sistema."
        );
        return false;
    }

    if (rootDev.startsWith("/dev/") && !QFile::exists(rootDev)) {
        QMessageBox::critical(
            this, "Dispositivo Inexistente Detectado",
            QString("El dispositivo especificado '%1' NO existe en el sistema.\n\n"
                    "Aplicar esta configuración dejaría el sistema sin arranque. "
                    "Operación bloqueada.").arg(rootDev)
        );
        return false;
    }

    return true;
}

// ============================================================================
// MOTOR DE DIAGNÓSTICO DE KERNEL, FIRMWARE, DUAL-BOOT Y BOOTLOADER
// ============================================================================

QString MainWindow::detectRecommendedInitrdModules(const QString &kernelVer) {
    if (kernelVer.isEmpty() || !QFile::exists("/usr/share/mkinitrd/mkinitrd_command_generator.sh")) {
        return QString();
    }

    QProcess proc;
    proc.start("/bin/sh", {"/usr/share/mkinitrd/mkinitrd_command_generator.sh", "-k", kernelVer});
    if (proc.waitForFinished(4000)) {
        QString out = QString::fromUtf8(proc.readAllStandardOutput());
        QRegularExpression modRx("-m\\s+([^\\s\"']+)");
        QRegularExpressionMatch match = modRx.match(out);
        if (match.hasMatch()) {
            return match.captured(1).trimmed();
        }
    }
    return QString();
}

void MainWindow::scanKernelEnvironment() {
    KernelEnvironmentInfo info;

    info.runningKernel = QSysInfo::kernelVersion().trimmed();
    if (info.runningKernel.isEmpty()) {
        QProcess unameProc;
        unameProc.start("uname", {"-r"});
        if (unameProc.waitForFinished(2000)) {
            info.runningKernel = QString::fromUtf8(unameProc.readAllStandardOutput()).trimmed();
        }
    }

    QSet<QString> versionsSet;
    QDir bootDir("/boot");
    QStringList vmlinuzFiles = bootDir.entryList({"vmlinuz-*"}, QDir::Files | QDir::NoSymLinks);
    QRegularExpression verRx("(\\d+\\.\\d+\\.\\d+(?:-[a-zA-Z0-9._]+)?)");

    for (const QString &file : vmlinuzFiles) {
        QRegularExpressionMatch m = verRx.match(file);
        if (m.hasMatch()) {
            versionsSet.insert(m.captured(1));
        }
    }

    QDir pkgDir("/var/log/packages");
    for (const QString &pkg : pkgDir.entryList({"kernel-generic-*", "kernel-huge-*", "kernel-modules-*"}, QDir::Files)) {
        QStringList parts = pkg.split('-');
        if (parts.size() >= 4) {
            versionsSet.insert(parts[parts.size() - 3]);
        }
    }

    if (!info.runningKernel.isEmpty()) {
        versionsSet.insert(info.runningKernel);
    }

    info.installedKernelVersions = versionsSet.values();
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(info.installedKernelVersions.begin(), info.installedKernelVersions.end(),
              [&](const QString &a, const QString &b) { return collator.compare(a, b) < 0; });

    info.latestInstalledKernel = info.installedKernelVersions.isEmpty()
        ? info.runningKernel
        : info.installedKernelVersions.last();

    QFile srcMakefile("/usr/src/linux/Makefile");
    if (srcMakefile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        info.hasKernelSources = true;
        QString ver, patch, sub, extra;
        QTextStream in(&srcMakefile);
        int linesRead = 0;
        while (!in.atEnd() && linesRead < 15) {
            QString line = in.readLine().trimmed();
            linesRead++;
            if (line.startsWith("VERSION =")) ver = line.mid(9).trimmed();
            else if (line.startsWith("PATCHLEVEL =")) patch = line.mid(12).trimmed();
            else if (line.startsWith("SUBLEVEL =")) sub = line.mid(10).trimmed();
            else if (line.startsWith("EXTRAVERSION =")) extra = line.mid(14).trimmed();
        }
        srcMakefile.close();
        if (!ver.isEmpty() && !patch.isEmpty()) {
            info.kernelSourceVersion = QString("%1.%2.%3%4").arg(ver, patch, sub, extra);
        }
    }

    QProcess findmntProc;
    findmntProc.start("findmnt", {"-n", "-o", "SOURCE,FSTYPE", "/"});
    if (findmntProc.waitForFinished(2500)) {
        QStringList fields = QString::fromUtf8(findmntProc.readAllStandardOutput())
                                 .trimmed()
                                 .split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (fields.size() >= 2) {
            info.rootDevice = fields[0];
            info.rootFsType = fields[1];
        }
    }
    if (info.rootDevice.isEmpty()) info.rootDevice = "/dev/sda2";
    if (info.rootFsType.isEmpty()) info.rootFsType = "ext4";

    if (QDir("/sys/firmware/efi").exists()) {
        info.firmwareMode = "UEFI / EFI";
        if (QDir("/boot/efi/EFI").exists()) {
            info.efiMountPoint = "/boot/efi";
        } else if (QDir("/efi/EFI").exists()) {
            info.efiMountPoint = "/efi";
        }
        if (!info.efiMountPoint.isEmpty()) {
            if (QDir(info.efiMountPoint + "/EFI/Slackware").exists()) {
                info.efiSlackwareDir = info.efiMountPoint + "/EFI/Slackware";
            } else if (QDir(info.efiMountPoint + "/EFI/slackware").exists()) {
                info.efiSlackwareDir = info.efiMountPoint + "/EFI/slackware";
            } else {
                info.efiSlackwareDir = info.efiMountPoint + "/EFI/Slackware";
            }
        }
    } else {
        info.firmwareMode = "Legacy BIOS / MBR";
    }

    if (!info.efiSlackwareDir.isEmpty() && QFile::exists(info.efiSlackwareDir + "/elilo.conf")) {
        info.bootloaderType = "ELILO";
        info.bootloaderConfigPath = info.efiSlackwareDir + "/elilo.conf";
    } else if (QFile::exists("/boot/grub/grub.cfg") &&
               (QFile::exists("/usr/sbin/grub-mkconfig") || QFile::exists("/sbin/grub-mkconfig"))) {
        info.bootloaderType = "GRUB";
        info.bootloaderConfigPath = "/boot/grub/grub.cfg";
    } else if (QFile::exists("/etc/lilo.conf") && QFile::exists("/sbin/lilo")) {
        info.bootloaderType = "LILO";
        info.bootloaderConfigPath = "/etc/lilo.conf";
    } else if (QFile::exists("/boot/grub/grub.cfg")) {
        info.bootloaderType = "GRUB";
        info.bootloaderConfigPath = "/boot/grub/grub.cfg";
    } else if (QFile::exists("/etc/lilo.conf")) {
        info.bootloaderType = "LILO";
        info.bootloaderConfigPath = "/etc/lilo.conf";
    } else {
        info.bootloaderType = "No detectado";
        info.bootloaderConfigPath = "—";
    }

    QSet<QString> otherOsSet;
    if (!info.efiMountPoint.isEmpty()) {
        QDir efiRoot(info.efiMountPoint + "/EFI");
        for (const QString &entry : efiRoot.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString lower = entry.toLower();
            if (lower == "slackware" || lower == "boot") continue;
            if (lower == "microsoft") otherOsSet.insert("Windows Boot Manager (EFI)");
            else otherOsSet.insert(entry + " (EFI)");
        }
    }
    if (QFile::exists("/usr/bin/os-prober") || QFile::exists("/usr/sbin/os-prober")) {
        QProcess osProber;
        osProber.start("os-prober");
        if (osProber.waitForFinished(3000)) {
            QString out = QString::fromUtf8(osProber.readAllStandardOutput()).trimmed();
            for (const QString &line : out.split('\n', Qt::SkipEmptyParts)) {
                QStringList parts = line.split(':');
                if (parts.size() >= 2) {
                    otherOsSet.insert(QString("%1 (%2)").arg(parts[1], parts[0]));
                }
            }
        }
    }
    info.detectedOtherOS = otherOsSet.values();
    info.hasDualBoot = !info.detectedOtherOS.isEmpty();

    if (!info.latestInstalledKernel.isEmpty() && info.latestInstalledKernel != info.runningKernel) {
        bool hasInitrd = QFile::exists(QString("/boot/initrd-%1.gz").arg(info.latestInstalledKernel));
        bool bootloaderConfigured = false;
        if (!info.bootloaderConfigPath.isEmpty() && info.bootloaderConfigPath != "—") {
            QFile cfg(info.bootloaderConfigPath);
            if (cfg.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString cfgContent = QString::fromUtf8(cfg.readAll());
                bootloaderConfigured = cfgContent.contains(info.latestInstalledKernel);
                cfg.close();
            }
        }
        info.newKernelPendingConfig = (!hasInitrd || !bootloaderConfigured);
    }

    info.recommendedModules = detectRecommendedInitrdModules(info.latestInstalledKernel);
    m_kernelInfo = info;

    m_lblDiagRunningKernel->setText(info.runningKernel);
    m_lblDiagInstalledKernels->setText(info.installedKernelVersions.join(", "));

    if (info.hasKernelSources) {
        m_lblDiagKernelSources->setStyleSheet("color: #1b5e20; font-weight: bold;");
        m_lblDiagKernelSources->setText(QString("Disponibles (%1)").arg(info.kernelSourceVersion));
    } else {
        m_lblDiagKernelSources->setStyleSheet("color: #616161;");
        m_lblDiagKernelSources->setText("No instalados (/usr/src/linux ausente)");
    }

    QString fwText = info.firmwareMode;
    if (!info.efiMountPoint.isEmpty()) {
        fwText += QString(" [ESP: %1]").arg(info.efiMountPoint);
    }
    fwText += QString(" | Raíz: %1 (%2)").arg(info.rootDevice, info.rootFsType);
    m_lblDiagFirmwareMode->setText(fwText);

    m_lblDiagBootloader->setStyleSheet(
        info.bootloaderType == "No detectado" ? "color: #c62828; font-weight: bold;" : "color: #0d47a1; font-weight: bold;"
    );
    m_lblDiagBootloader->setText(QString("%1 (%2)").arg(info.bootloaderType, info.bootloaderConfigPath));

    if (info.hasDualBoot) {
        m_lblDiagDualBoot->setStyleSheet("color: #ef6c00; font-weight: bold;");
        m_lblDiagDualBoot->setText("Sí: " + info.detectedOtherOS.join(", "));
    } else {
        m_lblDiagDualBoot->setStyleSheet("color: #424242;");
        m_lblDiagDualBoot->setText("No detectado (Solo Slackware)");
    }

    m_comboTargetKernel->blockSignals(true);
    m_comboTargetKernel->clear();
    for (int i = info.installedKernelVersions.size() - 1; i >= 0; --i) {
        const QString &ver = info.installedKernelVersions[i];
        QString label = ver;
        if (ver == info.runningKernel) label += "  [En ejecución actual]";
        if (ver != info.runningKernel && ver == info.latestInstalledKernel) label += "  [NUEVO - Pendiente de configurar]";
        m_comboTargetKernel->addItem(label, ver);
    }
    if (info.hasKernelSources && !info.kernelSourceVersion.isEmpty() &&
        !info.installedKernelVersions.contains(info.kernelSourceVersion)) {
        m_comboTargetKernel->addItem(info.kernelSourceVersion + "  [Desde /usr/src/linux]", info.kernelSourceVersion);
    }
    m_comboTargetKernel->blockSignals(false);

    if (!m_chkUnlockStorageParams->isChecked()) {
        m_editRootDevice->setText(info.rootDevice);
        m_editRootFs->setText(info.rootFsType);
        m_editInitrdModules->setText(info.recommendedModules);
    }

    if (info.newKernelPendingConfig) {
        m_lblKernelAlertText->setText(
            QString("¡Atención! Está ejecutando el kernel %1, pero se detectó la nueva versión instalada %2 "
                    "pendiente de configurar (initrd / %3). Configúrelo antes de reiniciar.")
                .arg(info.runningKernel, info.latestInstalledKernel, info.bootloaderType)
        );
        m_kernelAlertCard->setVisible(true);
    } else {
        m_kernelAlertCard->setVisible(false);
    }

    onKernelSelectionChanged();
}

void MainWindow::onKernelSelectionChanged() {
    QString targetVer = m_comboTargetKernel->currentData().toString();
    if (targetVer.isEmpty()) return;

    QString prevVariant = m_comboKernelVariant->currentData().toString();
    m_comboKernelVariant->blockSignals(true);
    m_comboKernelVariant->clear();

    QString genericFile = QString("vmlinuz-generic-%1").arg(targetVer);
    QString hugeFile = QString("vmlinuz-huge-%1").arg(targetVer);
    QString customFile = QString("vmlinuz-custom-%1").arg(targetVer);

    if (QFile::exists("/boot/" + genericFile)) {
        m_comboKernelVariant->addItem(genericFile + " (Recomendado - Requiere initrd)", genericFile);
    }
    if (QFile::exists("/boot/" + hugeFile)) {
        m_comboKernelVariant->addItem(hugeFile + " (Monolítico)", hugeFile);
    }
    if (QFile::exists("/boot/" + customFile)) {
        m_comboKernelVariant->addItem(customFile + " (Imagen bzImage compilada)", customFile);
    }
    if (m_comboKernelVariant->count() == 0) {
        m_comboKernelVariant->addItem(genericFile + " (Estándar)", genericFile);
    }
    if (m_kernelInfo.hasKernelSources) {
        m_comboKernelVariant->addItem(
            QString("Compilar nueva imagen bzImage desde /usr/src/linux -> %1").arg(customFile),
            "BUILD_BZIMAGE:" + customFile
        );
    }

    int idx = m_comboKernelVariant->findData(prevVariant);
    if (idx >= 0) m_comboKernelVariant->setCurrentIndex(idx);
    m_comboKernelVariant->blockSignals(false);

    if (!m_chkUnlockStorageParams->isChecked()) {
        QString mods = detectRecommendedInitrdModules(targetVer);
        m_editInitrdModules->setText(mods);
    }

    updateBootloaderPreview();
}

QString MainWindow::buildBootloaderPreviewText() const {
    QString targetVer = m_comboTargetKernel->currentData().toString();
    QString variantData = m_comboKernelVariant->currentData().toString();
    QString kernelFileName = variantData.startsWith("BUILD_BZIMAGE:")
        ? variantData.mid(14)
        : variantData;

    QString rootDev = m_editRootDevice->text().trimmed();
    QString initrdFileName = QString("initrd-%1.gz").arg(targetVer);
    bool useInitrd = m_chkGenerateInitrd->isChecked() || QFile::exists("/boot/" + initrdFileName);

    QString preview;
    if (m_kernelInfo.bootloaderType == "ELILO") {
        preview += QString("# Destino: %1 (Respaldo automático .bak)\n").arg(m_kernelInfo.bootloaderConfigPath);
        preview += QString("# Copia a ESP: /boot/%1 y /boot/%2 -> %3/\n\n")
                       .arg(kernelFileName, useInitrd ? initrdFileName : "(sin initrd)", m_kernelInfo.efiSlackwareDir);
        preview += "chooser=simple\ndelay=1\ntimeout=1\n#\ndefault=Slackware\n#\n";
        preview += QString("image=%1\n").arg(kernelFileName);
        preview += "\tlabel=Slackware\n";
        if (useInitrd) {
            preview += QString("\tinitrd=%1\n").arg(initrdFileName);
        }
        preview += "\tread-only\n";
        preview += QString("\tappend=\"root=%1 vga=normal ro\"\n").arg(rootDev);
        if (m_chkKeepFallbackEntry->isChecked()) {
            preview += "\n# + Entrada de respaldo anterior conservada como label=Slackware-Prev";
        }
    } else if (m_kernelInfo.bootloaderType == "LILO") {
        preview += "# Destino: /etc/lilo.conf (Respaldo .bak) + ejecución obligatoria de /sbin/lilo\n\n";
        preview += QString("image = /boot/%1\n").arg(kernelFileName);
        preview += QString("  root = %1\n").arg(rootDev);
        preview += "  label = Slackware\n";
        if (useInitrd) {
            preview += QString("  initrd = /boot/%1\n").arg(initrdFileName);
        }
        preview += "  read-only\n";
        if (m_chkKeepFallbackEntry->isChecked()) {
            preview += "\n# + Entrada anterior conservada como label = Slackware-Prev\n";
        }
        preview += "# Comando final: /sbin/lilo -v";
    } else if (m_kernelInfo.bootloaderType == "GRUB") {
        preview += "# Destino: /boot/grub/grub.cfg (Respaldo automático .bak)\n";
        preview += "# GRUB detectará automáticamente los kernels en /boot y sus initrd correspondientes:\n";
        preview += QString("#   - /boot/%1\n").arg(kernelFileName);
        if (useInitrd) {
            preview += QString("#   - /boot/%1\n").arg(initrdFileName);
        }
        preview += "\n/usr/sbin/grub-mkconfig -o /boot/grub/grub.cfg";
    } else {
        preview = "# No se detectó ELILO, LILO ni GRUB en rutas estándar.\n"
                  "# Puede generar únicamente el archivo /boot/" + initrdFileName + " en el paso 3.";
    }

    return preview;
}

void MainWindow::updateBootloaderPreview() {
    m_bootloaderPreview->setPlainText(buildBootloaderPreviewText());
}

QString MainWindow::buildKernelExecutionScript(bool buildBzImage, bool buildInitrd, bool updateBootloader) const {
    QString targetVer = m_comboTargetKernel->currentData().toString();
    QString variantData = m_comboKernelVariant->currentData().toString();
    QString kernelFileName = variantData.startsWith("BUILD_BZIMAGE:")
        ? variantData.mid(14)
        : variantData;

    QString rootDev = m_editRootDevice->text().trimmed();
    QString rootFs = m_editRootFs->text().trimmed();
    QString modules = m_editInitrdModules->text().trimmed();
    bool includeUdev = m_chkIncludeUdev->isChecked();
    bool keepFallback = m_chkKeepFallbackEntry->isChecked();

    QString initrdFileName = QString("initrd-%1.gz").arg(targetVer);
    QStringList cmds;
    cmds << "set -e";
    cmds << "echo '===================================================================='";
    cmds << "echo 'INICIANDO PROCESO DE KERNEL Y ARRANQUE COMO USUARIO ROOT'";
    cmds << "echo '===================================================================='";

    if (buildBzImage) {
        cmds << "echo '>>> [1/3] Compilando imagen bzImage y módulos desde /usr/src/linux...'";
        cmds << "cd /usr/src/linux";
        cmds << "make -j$(nproc) bzImage";
        cmds << "make -j$(nproc) modules";
        cmds << "make modules_install";
        cmds << QString("cp -v arch/x86/boot/bzImage /boot/%1").arg(kernelFileName);
        cmds << QString("cp -v System.map /boot/System.map-custom-%1").arg(targetVer);
    }

    if (buildInitrd) {
        cmds << QString("echo '>>> Generando /boot/%1 para kernel %2...'").arg(initrdFileName, targetVer);
        QString mkinitrdCmd = QString("/sbin/mkinitrd -c -k '%1' -f '%2' -r '%3'")
                                  .arg(targetVer, rootFs, rootDev);
        if (!modules.isEmpty()) {
            mkinitrdCmd += QString(" -m '%1'").arg(modules);
        }
        if (includeUdev) {
            mkinitrdCmd += " -u";
        }
        mkinitrdCmd += QString(" -o '/boot/%1'").arg(initrdFileName);
        cmds << mkinitrdCmd;

        cmds << QString("if [ ! -f '/boot/%1' ]; then echo 'ERROR: No se generó /boot/%1'; exit 1; fi")
                    .arg(initrdFileName);
        cmds << QString("echo '>>> initrd generado correctamente: /boot/%1'").arg(initrdFileName);
    }

    if (updateBootloader) {
        bool attachInitrd = buildInitrd || QFile::exists("/boot/" + initrdFileName);

        cmds << QString("if [ ! -f '/boot/%1' ]; then echo 'ERROR: No existe /boot/%1'; exit 1; fi")
                    .arg(kernelFileName);

        if (m_kernelInfo.bootloaderType == "ELILO") {
            QString espDir = m_kernelInfo.efiSlackwareDir;
            QString eliloConf = m_kernelInfo.bootloaderConfigPath;

            cmds << QString("echo '>>> Actualizando ELILO en %1...'").arg(espDir);
            cmds << QString("mkdir -p '%1'").arg(espDir);
            cmds << QString("cp -v '/boot/%1' '%2/'").arg(kernelFileName, espDir);
            if (attachInitrd) {
                cmds << QString("cp -v '/boot/%1' '%2/'").arg(initrdFileName, espDir);
            }
            cmds << QString("if [ -f '%1' ]; then cp -v '%1' '%1.bak.$(date +%%Y%%m%%d%%H%%M%%S)'; fi").arg(eliloConf);

            QString newStanza = QString("chooser=simple\\ndelay=1\\ntimeout=1\\n#\\ndefault=Slackware\\n#\\n"
                                        "image=%1\\n\\tlabel=Slackware\\n").arg(kernelFileName);
            if (attachInitrd) {
                newStanza += QString("\\tinitrd=%1\\n").arg(initrdFileName);
            }
            newStanza += QString("\\tread-only\\n\\tappend=\"root=%1 vga=normal ro\"\\n").arg(rootDev);

            if (keepFallback) {
                cmds << QString(
                    "PREV_STANZA=''; "
                    "if [ -f '%1' ]; then "
                    "  PREV_STANZA=$(awk '/^image=/{flag=1} flag{print}' '%1' | sed '0,/label=Slackware/s//label=Slackware-Prev/'); "
                    "fi; "
                    "printf '%2' > '%1'; "
                    "if [ -n \"$PREV_STANZA\" ]; then printf '\\n# Entrada anterior de respaldo\\n%%s\\n' \"$PREV_STANZA\" >> '%1'; fi"
                ).arg(eliloConf, newStanza);
            } else {
                cmds << QString("printf '%2' > '%1'").arg(eliloConf, newStanza);
            }

            cmds << QString("echo '>>> Contenido actualizado de %1:' && cat '%1'").arg(eliloConf);
        } else if (m_kernelInfo.bootloaderType == "LILO") {
            cmds << "echo '>>> Actualizando /etc/lilo.conf (preservando entradas anteriores) y ejecutando /sbin/lilo...'";
            QString newEntryPath = "/tmp/qslackpkg_lilo_std_entry.txt";
            QFile f(newEntryPath);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream ts(&f);
                ts << "image = /boot/" << kernelFileName << "\n";
                ts << "  root = " << rootDev << "\n";
                ts << "  label = Slackware\n";
                if (attachInitrd) {
                    ts << "  initrd = /boot/" << initrdFileName << "\n";
                }
                ts << "  read-only\n";
                f.close();
            }

            QString keepFlag = keepFallback ? "1" : "0";
            cmds << QString(
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

        } else if (m_kernelInfo.bootloaderType == "GRUB") {
            cmds << "echo '>>> Regenerando configuración de GRUB (/boot/grub/grub.cfg)...'";
            cmds << "if [ -f /boot/grub/grub.cfg ]; then cp -v /boot/grub/grub.cfg /boot/grub/grub.cfg.bak.$(date +%Y%m%d%H%M%S); fi";
            cmds << "grub-mkconfig -o /boot/grub/grub.cfg";
        } else {
            cmds << "echo 'AVISO: No se detectó ELILO, LILO ni GRUB. Actualice su gestor de arranque manualmente.'";
        }
    }

    cmds << "echo '===================================================================='";
    cmds << "echo 'PROCESO COMPLETADO EXITOSAMENTE'";
    cmds << "echo '===================================================================='";

    return cmds.join(" && ");
}

bool MainWindow::confirmRootKernelAction(const QString &actionTitle,
                                         const QStringList &configSummaryLines,
                                         const QString &commandPreview) {
    QDialog dialog(this);
    dialog.setWindowTitle("Confirmación Obligatoria de Usuario ROOT - " + actionTitle);
    dialog.resize(740, 520);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(10);

    bool isAlreadyRoot = (geteuid() == 0);
    QString rootBannerText = isAlreadyRoot
        ? "<b>ATENCIÓN (SESIÓN ROOT ACTIVA - UID 0):</b> Aunque ya ha iniciado sesión como usuario <b>root</b>, "
          "por seguridad debe revisar cuidadosamente cada configuración seleccionada antes de dar <b>Aceptar</b>."
        : "<b>ATENCIÓN (SE EXIGE CONTRASEÑA DE ROOT):</b> Los cambios seleccionados modifican el kernel y/o "
          "el arranque del sistema y <b>solo pueden ser ejecutados por el usuario root</b>. A continuación se le solicitará "
          "la contraseña de <b>root</b> (si no la ingresa, no se realizará ninguna operación). Lea atentamente antes de dar <b>Aceptar</b>.";

    QLabel *lblTopWarn = new QLabel(rootBannerText, &dialog);
    lblTopWarn->setWordWrap(true);
    lblTopWarn->setStyleSheet(
        "background-color: #ffebee; color: #b71c1c; border: 1px solid #ef5350; "
        "border-left: 5px solid #c62828; border-radius: 4px; padding: 10px; font-size: 9.5pt;"
    );
    layout->addWidget(lblTopWarn);

    QLabel *lblSummaryHeader = new QLabel("<b>Resumen de las configuraciones seleccionadas:</b>", &dialog);
    layout->addWidget(lblSummaryHeader);

    QTextEdit *summaryView = new QTextEdit(&dialog);
    summaryView->setReadOnly(true);
    summaryView->setMaximumHeight(165);
    summaryView->setPlainText(configSummaryLines.join("\n"));
    summaryView->setStyleSheet("background-color: #f5f5f5; color: #212121; font-size: 9.5pt; border-radius: 4px;");
    layout->addWidget(summaryView);

    QLabel *lblCmdHeader = new QLabel("<b>Comandos exactos que ejecutará el usuario root:</b>", &dialog);
    layout->addWidget(lblCmdHeader);

    QTextEdit *cmdView = new QTextEdit(&dialog);
    cmdView->setReadOnly(true);
    cmdView->setMaximumHeight(120);
    cmdView->setPlainText(commandPreview);
    cmdView->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: monospace; font-size: 8.5pt;");
    layout->addWidget(cmdView);

    QCheckBox *chkReadAck = new QCheckBox(
        "He leído atentamente todas las configuraciones seleccionadas y autorizo realizar estos cambios como usuario root.",
        &dialog
    );
    chkReadAck->setStyleSheet("font-weight: bold; color: #1a237e; padding: 4px;");
    layout->addWidget(chkReadAck);

    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *btnCancel = new QPushButton("Cancelar y revisar", &dialog);
    QPushButton *btnAccept = new QPushButton("Aceptar e Ingresar Contraseña de root", &dialog);
    if (isAlreadyRoot) btnAccept->setText("Aceptar y Ejecutar como root");
    btnAccept->setEnabled(false);
    btnAccept->setStyleSheet(
        "QPushButton { background-color: #c62828; color: white; font-weight: bold; padding: 7px 18px; border-radius: 4px; }"
        "QPushButton:hover:enabled { background-color: #b71c1c; }"
        "QPushButton:disabled { background-color: #ef9a9a; color: #fafafa; }"
    );

    btnRow->addStretch();
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnAccept);
    layout->addLayout(btnRow);

    connect(chkReadAck, &QCheckBox::toggled, btnAccept, &QPushButton::setEnabled);
    connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(btnAccept, &QPushButton::clicked, &dialog, &QDialog::accept);

    return dialog.exec() == QDialog::Accepted;
}

void MainWindow::runGenerateInitrdOnly() {
    if (!validateCriticalBootFields()) return;

    QString targetVer = m_comboTargetKernel->currentData().toString();
    QString rootDev = m_editRootDevice->text().trimmed();
    QString rootFs = m_editRootFs->text().trimmed();
    QString modules = m_editInitrdModules->text().trimmed();

    QStringList summary = {
        "• Operación: Generación exclusiva de imagen initrd",
        "• Usuario ejecutor: root (obligatorio)",
        "• Kernel objetivo: " + targetVer,
        "• Partición raíz (-r): " + rootDev + (m_chkUnlockStorageParams->isChecked() ? " [Editado manualmente]" : " [Protegido/Autodetectado]"),
        "• Sistema de archivos (-f): " + rootFs,
        "• Módulos (-m): " + (modules.isEmpty() ? "(Autodetección)" : modules),
        "• Soporte udev (-u): " + QString(m_chkIncludeUdev->isChecked() ? "Sí" : "No"),
        "• Archivo de salida: /boot/initrd-" + targetVer + ".gz"
    };

    QString script = buildKernelExecutionScript(false, true, false);
    if (!confirmRootKernelAction("Generar initrd", summary, script)) return;

    if (!m_btnToggleConsole->isChecked()) {
        m_btnToggleConsole->setChecked(true);
        toggleConsole();
    }

    executePrivilegedCommand("/bin/sh", {"-c", script},
                             QString("Generando /boot/initrd-%1.gz como root...").arg(targetVer));
}

void MainWindow::runUpdateBootloaderOnly() {
    if (!validateCriticalBootFields()) return;

    QString targetVer = m_comboTargetKernel->currentData().toString();
    QString variantData = m_comboKernelVariant->currentData().toString();
    QString kernelFileName = variantData.startsWith("BUILD_BZIMAGE:") ? variantData.mid(14) : variantData;

    QStringList summary = {
        "• Operación: Actualización exclusiva del Bootloader",
        "• Usuario ejecutor: root (obligatorio)",
        "• Bootloader detectado: " + m_kernelInfo.bootloaderType + " (" + m_kernelInfo.bootloaderConfigPath + ")",
        "• Kernel objetivo: " + targetVer + " (/boot/" + kernelFileName + ")",
        "• Partición raíz configurada: " + m_editRootDevice->text().trimmed(),
        "• Respaldo automático (.bak): Activado",
        "• Conservar entrada anterior (Slackware-Prev): " + QString(m_chkKeepFallbackEntry->isChecked() ? "Sí" : "No")
    };

    QString script = buildKernelExecutionScript(false, false, true);
    if (!confirmRootKernelAction("Actualizar Bootloader", summary, script)) return;

    if (!m_btnToggleConsole->isChecked()) {
        m_btnToggleConsole->setChecked(true);
        toggleConsole();
    }

    executePrivilegedCommand("/bin/sh", {"-c", script},
                             QString("Actualizando %1 para kernel %2 como root...")
                                 .arg(m_kernelInfo.bootloaderType, targetVer));
}

void MainWindow::runFullKernelWorkflow() {
    if (!validateCriticalBootFields()) return;

    QString targetVer = m_comboTargetKernel->currentData().toString();
    QString variantData = m_comboKernelVariant->currentData().toString();
    bool buildBzImage = variantData.startsWith("BUILD_BZIMAGE:");
    QString kernelFileName = buildBzImage ? variantData.mid(14) : variantData;
    bool buildInitrd = m_chkGenerateInitrd->isChecked();
    bool updateBoot = m_chkUpdateBootloader->isChecked();

    if (!buildBzImage && !buildInitrd && !updateBoot) {
        QMessageBox::information(this, "Sin acciones marcadas",
                                 "Marque al menos la generación de initrd o la actualización del bootloader.");
        return;
    }

    QStringList summary = {
        "• Operación: Configuración completa de Kernel y Arranque",
        "• Usuario ejecutor: root (obligatorio)",
        "• Kernel objetivo: " + targetVer,
        "• Imagen de Kernel: " + kernelFileName + (buildBzImage ? " (Se compilará bzImage desde /usr/src/linux)" : " (Existente en /boot)"),
        "• Generar initrd: " + QString(buildInitrd ? "Sí (/boot/initrd-" + targetVer + ".gz)" : "No"),
        "• Partición raíz / FS: " + m_editRootDevice->text().trimmed() + " (" + m_editRootFs->text().trimmed() + ")",
        "• Actualizar Bootloader: " + QString(updateBoot ? m_kernelInfo.bootloaderType + " (" + m_kernelInfo.bootloaderConfigPath + ")" : "No"),
        "• Conservar entrada anterior (Slackware-Prev): " + QString(m_chkKeepFallbackEntry->isChecked() ? "Sí" : "No")
    };

    QString script = buildKernelExecutionScript(buildBzImage, buildInitrd, updateBoot);
    if (!confirmRootKernelAction("Configuración Completa de Kernel", summary, script)) return;

    if (!m_btnToggleConsole->isChecked()) {
        m_btnToggleConsole->setChecked(true);
        toggleConsole();
    }

    executePrivilegedCommand("/bin/sh", {"-c", script},
                             QString("Configurando kernel %1 y %2 como root...")
                                 .arg(targetVer, m_kernelInfo.bootloaderType));
}

// ============================================================================
// BANDEJA DEL SISTEMA Y RED
// ============================================================================

void MainWindow::setupTrayIcon() {
    m_trayIcon = new QSystemTrayIcon(this);
    updateTrayIconState(StateUpToDate, 0);
    m_trayIcon->setToolTip("QSlackpkg - Package & Kernel Manager for Slackware Linux");

    m_trayMenu = new QMenu(this);
    QAction *showAction = m_trayMenu->addAction("Abrir Centro de Software");
    QAction *checkAction = m_trayMenu->addAction("Buscar actualizaciones ahora");
    QAction *aboutAction = m_trayMenu->addAction("Acerca de QSlackpkg");
    m_trayMenu->addSeparator();
    QAction *quitAction = m_trayMenu->addAction("Salir completamente");

    connect(showAction, &QAction::triggered, this, &QMainWindow::showNormal);
    connect(checkAction, &QAction::triggered, this, &MainWindow::checkUpdatesInBackground);
    connect(quitAction, &QAction::triggered, this, [this]() {
        m_forceQuit = true;
        qApp->quit();
    });
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();
}

void MainWindow::updateTrayIconState(TrayState state, int count) {
    QIcon baseIcon;
    if (state == StateUpToDate) {
        baseIcon = style()->standardIcon(QStyle::SP_DialogApplyButton);
    } else if (state == StateUpdatesAvailable) {
        baseIcon = style()->standardIcon(QStyle::SP_MessageBoxWarning);
    } else {
        baseIcon = style()->standardIcon(QStyle::SP_BrowserReload);
    }

    if (state == StateUpdatesAvailable && count > 0) {
        QPixmap pixmap = baseIcon.pixmap(24, 24);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        QRect badgeRect(12, 0, 12, 12);
        if (count > 9) badgeRect.setRight(16);
        painter.setBrush(QColor("#e53935"));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(badgeRect);

        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPointSize(7);
        font.setBold(true);
        painter.setFont(font);
        painter.drawText(badgeRect, Qt::AlignCenter, QString::number(count));
        painter.end();

        m_trayIcon->setIcon(QIcon(pixmap));
    } else {
        m_trayIcon->setIcon(baseIcon);
    }
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
    QNetworkReply *reply = m_netManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onChangeLogDownloaded(reply);
    });
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
            updateTrayIconState(StateUpdatesAvailable, 1);
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
        QString categoryTag; // "updates", "sbo", "external", "blacklisted", "official"
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
            QString tag = bl ? "blacklisted" : "sbo";
            rowsUpToDate.append({shortName, "—", instFullName, instFullName, statusText, false, true, tag});
        } else if (confirmedUpgradeMap.contains(shortName)) {
            QString baseName = basePkgMap.value(shortName, "— (Rama Extra)");
            QString repoFullName = confirmedUpgradeMap.value(shortName);

            rowsWithUpdates.append({shortName, baseName, "—", instFullName, repoFullName, true, false, "updates"});
            m_confirmedUpdates.append({shortName, instFullName, repoFullName});
        } else {
            QString baseName = basePkgMap.value(shortName, "— (Externo)");
            QString statusText = bl ? "Bloqueado (blacklist)" : "Sin paquete nuevo";
            QString tag = bl ? "blacklisted" : (basePkgMap.contains(shortName) ? "official" : "external");
            rowsUpToDate.append({shortName, baseName, "—", instFullName, statusText, false, false, tag});
        }
    }

    m_packageTable->setUpdatesEnabled(false);
    m_packageTable->setRowCount(0);
    QList<PkgRow> allRows = rowsWithUpdates + rowsUpToDate;

    for (const PkgRow &item : allRows) {
        int row = m_packageTable->rowCount();
        m_packageTable->insertRow(row);

        QTableWidgetItem *nameItem = new QTableWidgetItem(item.name);
        nameItem->setData(Qt::UserRole, item.categoryTag);

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

        QWidget *cellWidget = new QWidget(m_packageTable);
        QHBoxLayout *cellLayout = new QHBoxLayout(cellWidget);
        cellLayout->setContentsMargins(6, 2, 6, 2);
        cellLayout->setAlignment(Qt::AlignCenter);

        QPushButton *btnRemove = new QPushButton("Remove", cellWidget);
        btnRemove->setCursor(Qt::PointingHandCursor);
        btnRemove->setToolTip(QString("Desinstalar %1 (requiere contraseña de root)").arg(item.installed));
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

    m_packageTable->setUpdatesEnabled(true);
    filterOfficialTable();

    int pendingCount = m_confirmedUpdates.size();
    m_btnUpgrade->setEnabled(pendingCount > 0);
    m_btnPendingDetails->setEnabled(pendingCount > 0);

    QWidget *bannerContainer = findChild<QWidget*>("bannerContainer");
    if (pendingCount > 0) {
        if (bannerContainer) bannerContainer->setStyleSheet("#bannerContainer { background-color: #ef6c00; border-radius: 6px; }");
        m_statusBanner->setText(QString("Atención: Hay %1 paquete(s) oficial(es) listos para actualizar.").arg(pendingCount));
        m_btnPendingDetails->setText(QString("Ver %1 paquete(s) a actualizar...").arg(pendingCount));
        updateTrayIconState(StateUpdatesAvailable, pendingCount);
    } else if (m_kernelInfo.newKernelPendingConfig) {
        if (bannerContainer) bannerContainer->setStyleSheet("#bannerContainer { background-color: #d84315; border-radius: 6px; }");
        m_statusBanner->setText(
            QString("¡Nuevo Kernel instalado (%1)! Configure initrd y %2 en la pestaña 'Kernel y Arranque' antes de reiniciar.")
                .arg(m_kernelInfo.latestInstalledKernel, m_kernelInfo.bootloaderType)
        );
        m_btnPendingDetails->setText("0 paquetes oficiales pendientes");
        updateTrayIconState(StateUpdatesAvailable, 1);
    } else {
        if (bannerContainer) bannerContainer->setStyleSheet("#bannerContainer { background-color: #2e7d32; border-radius: 6px; }");
        m_statusBanner->setText("Sistema base al día. Coincide con slackpkg: 0 paquetes pendientes.");
        m_btnPendingDetails->setText("0 paquetes oficiales para actualizar");
        updateTrayIconState(StateUpToDate, 0);
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
    QPushButton *btnApplyNow = new QPushButton("Actualizar sistema ahora (requiere root)", &dialog);
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
// MOTOR SLACKBUILDS.ORG (SBOPKG)
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
            "(requiere contraseña de root) para habilitar el repositorio de SlackBuilds.org."
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
        this, "Instalar sbopkg 0.38.3 (Requiere root)",
        "Se descargará el paquete oficial desde GitHub y se instalará en tu sistema como root:\n\n"
        "URL: https://github.com/sbopkg/sbopkg/releases/download/0.38.3/sbopkg-0.38.3-noarch-1_wsr.tgz\n"
        "Comando: installpkg sbopkg-0.38.3-noarch-1_wsr.tgz\n\n"
        "¿Deseas continuar e ingresar la contraseña de root?",
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
                             "Descargando e instalando sbopkg 0.38.3 como root...");
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
                              "Se compilará e instalará como <b>root</b> mediante <code>sbopkg</code>:")
                          .arg(pkg.name, pkg.version);
    } else {
        summaryText = QString("Para instalar <b>%1 (%2)</b> se detectaron <b>%3 dependencia(s) faltante(s)</b>.<br>"
                              "<code>sbopkg</code> (como <b>root</b>) compilará e instalará la cola en este orden estricto:")
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
    QPushButton *btnBuild = new QPushButton("Compilar e Instalar Cola (requiere root)", &dialog);
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
                             QString("Compilando e instalando %1 como root (%2 paquete(s) en cola)...").arg(pkg.name).arg(buildQueue.size()));
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

    executePrivilegedCommand("/usr/sbin/sbopkg", {"-r"}, "Sincronizando repositorio local de SlackBuilds.org como root...");
}

// ============================================================================
// MOTOR REPOSITORIO ALIENBOB (DESCARGA, VERIFICACIÓN MD5+GPG, INSTALACIÓN, REMOVE)
// ============================================================================

QString MainWindow::detectAlienTargetBranch() const {
    QString arch = QSysInfo::currentCpuArchitecture();
    bool is64 = (arch.contains("64") || arch == "x86_64");
    QString pkgPrefix = is64 ? "pkg64" : "pkg";

    QString verTag = "15.0";
    QFile verFile("/etc/slackware-version");
    if (verFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(verFile.readAll()).trimmed();
        verFile.close();
        if (content.contains('+') || content.contains("current", Qt::CaseInsensitive)) {
            verTag = "current";
        } else {
            QRegularExpression rx("(\\d+\\.\\d+)");
            QRegularExpressionMatch m = rx.match(content);
            if (m.hasMatch()) {
                verTag = m.captured(1);
            }
        }
    }
    return QString("%1/%2").arg(pkgPrefix, verTag);
}

QString MainWindow::getAlienCacheFilePath() const {
    QString cacheDir = QDir::homePath() + "/.cache/qslackpkg";
    QDir().mkpath(cacheDir);
    return cacheDir + "/alien_PACKAGES.TXT";
}

QString MainWindow::buildAlienInstallSnippet(const AlienPackageInfo &pkg) const {
    QString pkgUrl = m_alienBaseUrl + pkg.location + "/" + pkg.fileName;
    QString md5Url = pkgUrl + ".md5";
    QString ascUrl = pkgUrl + ".asc";

    return QString(
        "echo '>>> Descargando %1 desde AlienBOB...'; "
        "wget -O '%1' '%2'; "
        "echo '>>> Verificando suma MD5...'; "
        "if wget -q -O '%1.md5' '%3'; then "
        "  md5sum -c '%1.md5'; "
        "else "
        "  echo 'AVISO: Archivo .md5 no disponible en servidor.'; "
        "fi; "
        "if command -v gpg >/dev/null 2>&1 && [ -s GPG-KEY ]; then "
        "  if wget -q -O '%1.asc' '%4'; then "
        "    echo '>>> Verificando firma GPG de Eric Hameleers...'; "
        "    gpg --homedir \"$TMPD/gnupg\" --verify '%1.asc' '%1'; "
        "  fi; "
        "fi; "
        "echo '>>> Instalando / actualizando %1 en el sistema...'; "
        "/sbin/upgradepkg --install-new '%1'"
    ).arg(pkg.fileName, pkgUrl, md5Url, ascUrl);
}

void MainWindow::loadAlienCatalogFromCache() {
    QString cachePath = getAlienCacheFilePath();
    QFile file(cachePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_alienCatalog.clear();
        m_alienTable->setRowCount(0);
        m_btnAlienUpgradeAll->setEnabled(false);
        m_btnAlienUpgradeAll->setText("Actualizar todos (0)");
        m_alienStatusLabel->setStyleSheet("color: #ef6c00; font-weight: bold; padding: 4px;");
        m_alienStatusLabel->setText(
            "Catálogo AlienBOB aún no sincronizado. Pulsa 'Sincronizar catálogo AlienBOB' "
            "(no requiere contraseña de root) para descargar la lista de paquetes binarios."
        );
        return;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();
    parseAlienPackagesIndex(content);
}

void MainWindow::runAlienSync() {
    m_btnAlienSync->setEnabled(false);
    m_alienStatusLabel->setStyleSheet("color: #1565c0; font-weight: bold; padding: 4px;");
    m_alienStatusLabel->setText("Descargando índice PACKAGES.TXT desde slackware.com/~alien/slackbuilds/...");

    QUrl url(m_alienBaseUrl + "PACKAGES.TXT");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "QSlackpkg/1.0 (Slackware Linux)");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_netManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        m_btnAlienSync->setEnabled(true);

        if (reply->error() != QNetworkReply::NoError) {
            m_alienStatusLabel->setStyleSheet("color: #c62828; font-weight: bold; padding: 4px;");
            m_alienStatusLabel->setText(
                QString("Error al sincronizar catálogo AlienBOB: %1").arg(reply->errorString())
            );
            reply->deleteLater();
            return;
        }

        QByteArray rawData = reply->readAll();
        QString content = QString::fromUtf8(rawData);
        reply->deleteLater();

        if (!content.contains("PACKAGE NAME:")) {
            m_alienStatusLabel->setStyleSheet("color: #c62828; font-weight: bold; padding: 4px;");
            m_alienStatusLabel->setText("La respuesta recibida de AlienBOB no contiene un índice PACKAGES.TXT válido.");
            return;
        }

        QFile cacheFile(getAlienCacheFilePath());
        if (cacheFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            cacheFile.write(rawData);
            cacheFile.close();
        }

        parseAlienPackagesIndex(content);
    });
}

void MainWindow::parseAlienPackagesIndex(const QString &indexContent) {
    m_alienCatalog.clear();

    QMap<QString, QString> installedFullMap;
    QMap<QString, QString> installedVerMap;
    QMap<QString, QString> installedBuildMap;
    QDir pkgDir("/var/log/packages");
    for (const QString &fileName : pkgDir.entryList(QDir::Files)) {
        QStringList parts = fileName.split('-');
        if (parts.size() >= 4) {
            QString shortName = parts.mid(0, parts.size() - 3).join('-');
            QString version = parts[parts.size() - 3];
            QString build = parts.last();
            installedFullMap.insert(shortName, fileName);
            installedVerMap.insert(shortName, version);
            installedBuildMap.insert(shortName, build);
        }
    }

    auto finalizeEntry = [&](AlienPackageInfo &entry) {
        if (entry.name.isEmpty() || entry.fileName.isEmpty()) return;
        if (installedFullMap.contains(entry.name)) {
            entry.installedFull = installedFullMap.value(entry.name);
            QString instVer = installedVerMap.value(entry.name);
            QString instBuild = installedBuildMap.value(entry.name);
            if (instBuild.endsWith("alien", Qt::CaseInsensitive) &&
                (instVer != entry.version || instBuild != entry.build)) {
                entry.hasUpdate = true;
            }
        }
        m_alienCatalog.append(entry);
    };

    QStringList lines = indexContent.split('\n');
    AlienPackageInfo current;
    bool inDescription = false;

    for (const QString &rawLine : lines) {
        QString line = rawLine;
        if (line.endsWith('\r')) line.chop(1);

        if (line.startsWith("PACKAGE NAME:")) {
            if (!current.name.isEmpty()) {
                finalizeEntry(current);
            }
            current = AlienPackageInfo();
            inDescription = false;

            current.fileName = line.mid(13).trimmed();
            QString baseName = current.fileName;
            if (baseName.endsWith(".txz") || baseName.endsWith(".tgz") ||
                baseName.endsWith(".tlz") || baseName.endsWith(".tbz")) {
                baseName.chop(4);
            }
            QStringList parts = baseName.split('-');
            if (parts.size() >= 4) {
                current.build = parts.last();
                current.arch = parts[parts.size() - 2];
                current.version = parts[parts.size() - 3];
                current.name = parts.mid(0, parts.size() - 3).join('-');
            }
        } else if (line.startsWith("PACKAGE LOCATION:")) {
            QString loc = line.mid(17).trimmed();
            if (loc.startsWith("./")) loc = loc.mid(2);
            while (loc.startsWith('/')) loc = loc.mid(1);
            while (loc.endsWith('/')) loc.chop(1);
            current.location = loc;

            QStringList locParts = loc.split('/', Qt::SkipEmptyParts);
            if (locParts.size() >= 2) {
                current.branchTag = locParts[locParts.size() - 2] + "/" + locParts[locParts.size() - 1];
            }
        } else if (line.startsWith("PACKAGE SIZE (compressed):")) {
            current.compressedSize = line.mid(26).trimmed();
        } else if (line.startsWith("PACKAGE REQUIRED:")) {
            current.requiredDeps = line.mid(17).trimmed();
        } else if (line.startsWith("PACKAGE DESCRIPTION:")) {
            inDescription = true;
        } else if (line.trimmed().isEmpty()) {
            if (!current.name.isEmpty()) {
                finalizeEntry(current);
                current = AlienPackageInfo();
            }
            inDescription = false;
        } else if (inDescription && !current.name.isEmpty() && current.shortDesc.isEmpty()) {
            QString prefix = current.name + ":";
            if (line.startsWith(prefix)) {
                QString descText = line.mid(prefix.length()).trimmed();
                if (!descText.isEmpty()) {
                    current.shortDesc = descText;
                }
            }
        }
    }

    if (!current.name.isEmpty()) {
        finalizeEntry(current);
    }

    QFileInfo cacheInfo(getAlienCacheFilePath());
    QString syncDateStr = cacheInfo.exists()
        ? cacheInfo.lastModified().toString("dd/MM/yyyy HH:mm")
        : "Recién descargado";

    QString targetBranch = m_comboAlienBranch->currentData().toString();
    int branchPkgCount = 0;
    int branchUpdatesCount = 0;
    for (const AlienPackageInfo &pkg : m_alienCatalog) {
        if (targetBranch.isEmpty() || pkg.branchTag == targetBranch) {
            branchPkgCount++;
            if (pkg.hasUpdate) branchUpdatesCount++;
        }
    }

    m_btnAlienUpgradeAll->setEnabled(branchUpdatesCount > 0);
    m_btnAlienUpgradeAll->setText(QString("Actualizar todos (%1)").arg(branchUpdatesCount));

    m_alienStatusLabel->setStyleSheet("color: #0d47a1; font-weight: bold; padding: 4px;");
    m_alienStatusLabel->setText(
        QString("Catálogo AlienBOB listo (%1): %2 binarios disponibles en rama seleccionada (%3 total) | Actualizaciones AlienBOB: %4")
            .arg(syncDateStr)
            .arg(branchPkgCount)
            .arg(m_alienCatalog.size())
            .arg(branchUpdatesCount)
    );

    filterAlienTable();
}

void MainWindow::filterAlienTable() {
    QString selectedBranch = m_comboAlienBranch->currentData().toString();
    QString searchQuery = m_editAlienSearch->text().trimmed();
    bool onlyInstalled = m_chkAlienInstalledOnly->isChecked();
    bool onlyUpdates = m_chkAlienUpdatesOnly->isChecked();

    m_alienTable->setUpdatesEnabled(false);
    m_alienTable->setRowCount(0);

    int maxVisible = (selectedBranch.isEmpty() && searchQuery.isEmpty() && !onlyInstalled && !onlyUpdates) ? 500 : 2500;
    int shown = 0;
    int branchUpdatesCount = 0;

    for (const AlienPackageInfo &pkg : m_alienCatalog) {
        if (!selectedBranch.isEmpty() && pkg.branchTag != selectedBranch) continue;
        if (pkg.hasUpdate) branchUpdatesCount++;

        if (onlyInstalled && pkg.installedFull.isEmpty()) continue;
        if (onlyUpdates && !pkg.hasUpdate) continue;
        if (!searchQuery.isEmpty() &&
            !pkg.name.contains(searchQuery, Qt::CaseInsensitive) &&
            !pkg.shortDesc.contains(searchQuery, Qt::CaseInsensitive)) {
            continue;
        }

        if (shown >= maxVisible) break;
        shown++;

        int row = m_alienTable->rowCount();
        m_alienTable->insertRow(row);

        QString pkgUrl = m_alienBaseUrl + pkg.location + "/" + pkg.fileName;
        QString detailTooltip = QString(
            "Archivo: %1\n"
            "Ubicación: %2\n"
            "URL: %3\n"
            "Dependencias declaradas: %4"
        ).arg(pkg.fileName,
              pkg.location,
              pkgUrl,
              pkg.requiredDeps.isEmpty() ? "Ninguna especificada" : pkg.requiredDeps);

        QTableWidgetItem *nameItem = new QTableWidgetItem(pkg.name);
        QFont boldName = nameItem->font();
        boldName.setBold(true);
        nameItem->setFont(boldName);
        nameItem->setToolTip(detailTooltip);

        QTableWidgetItem *branchItem = new QTableWidgetItem(pkg.branchTag);
        branchItem->setForeground(QColor("#546e7a"));

        QTableWidgetItem *instItem = new QTableWidgetItem(
            pkg.installedFull.isEmpty() ? "No instalado" : pkg.installedFull
        );
        QTableWidgetItem *verItem = new QTableWidgetItem(
            QString("%1 (%2)").arg(pkg.version, pkg.build)
        );
        QTableWidgetItem *sizeItem = new QTableWidgetItem(pkg.compressedSize);
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QTableWidgetItem *descItem = new QTableWidgetItem(pkg.shortDesc);
        descItem->setToolTip(detailTooltip);

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

        m_alienTable->setItem(row, 0, nameItem);
        m_alienTable->setItem(row, 1, branchItem);
        m_alienTable->setItem(row, 2, instItem);
        m_alienTable->setItem(row, 3, verItem);
        m_alienTable->setItem(row, 4, sizeItem);
        m_alienTable->setItem(row, 5, descItem);

        QWidget *cellWidget = new QWidget(m_alienTable);
        QHBoxLayout *cellLayout = new QHBoxLayout(cellWidget);
        cellLayout->setContentsMargins(4, 2, 4, 2);
        cellLayout->setSpacing(4);

        QString btnText = "Instalar";
        QString btnStyle = "QPushButton { background-color: #e3f2fd; color: #0d47a1; border: 1px solid #90caf9; "
                           "border-radius: 3px; padding: 2px 8px; font-weight: bold; font-size: 8.5pt; }"
                           "QPushButton:hover { background-color: #1565c0; color: white; }";

        if (pkg.hasUpdate) {
            btnText = "Actualizar";
            btnStyle = "QPushButton { background-color: #fff3e0; color: #e65100; border: 1px solid #ffb74d; "
                       "border-radius: 3px; padding: 2px 8px; font-weight: bold; font-size: 8.5pt; }"
                       "QPushButton:hover { background-color: #ef6c00; color: white; }";
        } else if (!pkg.installedFull.isEmpty()) {
            btnText = "Reinstalar";
            btnStyle = "QPushButton { background-color: #f5f5f5; color: #424242; border: 1px solid #bdbdbd; "
                       "border-radius: 3px; padding: 2px 8px; font-size: 8.5pt; }"
                       "QPushButton:hover { background-color: #616161; color: white; }";
        }

        QPushButton *btnInstall = new QPushButton(btnText, cellWidget);
        btnInstall->setCursor(Qt::PointingHandCursor);
        btnInstall->setStyleSheet(btnStyle);
        connect(btnInstall, &QPushButton::clicked, this, [this, pkg]() {
            onInstallAlienClicked(pkg);
        });
        cellLayout->addWidget(btnInstall);

        if (!pkg.installedFull.isEmpty()) {
            QPushButton *btnRemove = new QPushButton("Remove", cellWidget);
            btnRemove->setCursor(Qt::PointingHandCursor);
            btnRemove->setToolTip(QString("Desinstalar %1 (requiere root)").arg(pkg.installedFull));
            btnRemove->setStyleSheet(
                "QPushButton { background-color: #ffebee; color: #c62828; border: 1px solid #ef9a9a; "
                "border-radius: 3px; padding: 2px 8px; font-weight: bold; font-size: 8.5pt; }"
                "QPushButton:hover { background-color: #c62828; color: white; }"
            );
            connect(btnRemove, &QPushButton::clicked, this, [this, pkg]() {
                onRemoveAlienClicked(pkg);
            });
            cellLayout->addWidget(btnRemove);
        }

        m_alienTable->setCellWidget(row, 6, cellWidget);
    }

    m_btnAlienUpgradeAll->setEnabled(branchUpdatesCount > 0);
    m_btnAlienUpgradeAll->setText(QString("Actualizar todos (%1)").arg(branchUpdatesCount));
    m_alienTable->setUpdatesEnabled(true);
}

void MainWindow::runAlienUpgradeAll() {
    QString selectedBranch = m_comboAlienBranch->currentData().toString();
    QList<AlienPackageInfo> pendingList;

    for (const AlienPackageInfo &pkg : m_alienCatalog) {
        if (!selectedBranch.isEmpty() && pkg.branchTag != selectedBranch) continue;
        if (pkg.hasUpdate) {
            pendingList.append(pkg);
        }
    }

    if (pendingList.isEmpty()) return;

    QStringList namesPreview;
    for (const AlienPackageInfo &pkg : pendingList) {
        namesPreview << QString("  • %1  (%2  →  %3)").arg(pkg.name, pkg.installedFull, pkg.fileName);
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QString("Actualizar %1 paquete(s) de AlienBOB (Requiere root)").arg(pendingList.size()),
        QString("Se descargarán, verificarán (MD5 + GPG-KEY) y actualizarán los siguientes paquetes como <b>root</b>:\n\n"
                "%1\n\n"
                "¿Deseas continuar e ingresar la contraseña de root?").arg(namesPreview.join("\n")),
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply != QMessageBox::Yes) return;

    if (!m_btnToggleConsole->isChecked()) {
        m_btnToggleConsole->setChecked(true);
        toggleConsole();
    }

    QStringList scriptSteps;
    scriptSteps << "set -e";
    scriptSteps << "TMPD=$(mktemp -d /tmp/qslackpkg_alien_batch.XXXXXX)";
    scriptSteps << "trap 'rm -rf \"$TMPD\"' EXIT";
    scriptSteps << "cd \"$TMPD\"";
    scriptSteps << "mkdir -m 700 -p \"$TMPD/gnupg\"";
    scriptSteps << QString("wget -q -O GPG-KEY '%1GPG-KEY' && gpg --homedir \"$TMPD/gnupg\" --import GPG-KEY 2>/dev/null || true")
                       .arg(m_alienBaseUrl);

    int total = pendingList.size();
    for (int i = 0; i < total; ++i) {
        scriptSteps << QString("echo '>>> (%1/%2) Procesando actualización de %3...'")
                           .arg(i + 1)
                           .arg(total)
                           .arg(pendingList[i].name);
        scriptSteps << buildAlienInstallSnippet(pendingList[i]);
    }

    executePrivilegedCommand("/bin/sh", {"-c", scriptSteps.join("; ")},
                             QString("Actualizando %1 paquete(s) de AlienBOB como root...").arg(total));
}

void MainWindow::onInstallAlienClicked(const AlienPackageInfo &pkg) {
    QString pkgUrl = m_alienBaseUrl + pkg.location + "/" + pkg.fileName;

    QString depsNote = pkg.requiredDeps.isEmpty()
        ? "Ninguna adicional especificada"
        : pkg.requiredDeps;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QString("Instalar / Actualizar desde AlienBOB - %1").arg(pkg.name),
        QString("Se descargará e instalará el siguiente paquete binario como <b>root</b>:\n\n"
                "• Paquete: %1\n"
                "• Rama / Arquitectura: %2\n"
                "• Tamaño comprimido: %3\n"
                "• Dependencias declaradas: %4\n"
                "• Origen: %5\n\n"
                "Antes de instalar, se verificará automáticamente su suma MD5 y firma GPG oficial.\n"
                "¿Deseas continuar e ingresar la contraseña de root?")
            .arg(pkg.fileName, pkg.branchTag, pkg.compressedSize, depsNote, pkgUrl),
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply != QMessageBox::Yes) return;

    if (!m_btnToggleConsole->isChecked()) {
        m_btnToggleConsole->setChecked(true);
        toggleConsole();
    }

    QString cmd = QString(
        "set -e; "
        "TMPD=$(mktemp -d /tmp/qslackpkg_alien.XXXXXX); "
        "trap 'rm -rf \"$TMPD\"' EXIT; "
        "cd \"$TMPD\"; "
        "mkdir -m 700 -p \"$TMPD/gnupg\"; "
        "wget -q -O GPG-KEY '%1GPG-KEY' && gpg --homedir \"$TMPD/gnupg\" --import GPG-KEY 2>/dev/null || true; "
        "%2"
    ).arg(m_alienBaseUrl, buildAlienInstallSnippet(pkg));

    executePrivilegedCommand("/bin/sh", {"-c", cmd},
                             QString("Descargando e instalando %1 desde AlienBOB como root...").arg(pkg.name));
}

void MainWindow::onRemoveAlienClicked(const AlienPackageInfo &pkg) {
    if (pkg.installedFull.isEmpty()) return;

    QMessageBox::StandardButton reply = QMessageBox::warning(
        this,
        "Eliminar Paquete (Requiere root)",
        QString("¿Estás seguro de que deseas desinstalar el siguiente paquete?\n\n"
                "Paquete instalado: %1\n"
                "Comando: /sbin/removepkg %1\n\n"
                "Se exigirá la contraseña de root para proceder.").arg(pkg.installedFull),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    if (reply != QMessageBox::Yes) return;

    if (!m_btnToggleConsole->isChecked()) {
        m_btnToggleConsole->setChecked(true);
        toggleConsole();
    }

    executePrivilegedCommand("/sbin/removepkg", {pkg.installedFull},
                             QString("Eliminando paquete %1 como root...").arg(pkg.installedFull));
}

// ============================================================================
// EJECUCIÓN PRIVILEGIADA ESTRICTA (EXIGE CONTRASEÑA DE ROOT VIA PKEXEC)
// ============================================================================

void MainWindow::executePrivilegedCommand(const QString &binaryPath, const QStringList &args, const QString &statusMsg) {
    if (m_process->state() != QProcess::NotRunning) return;

    m_consoleOutput->clear();
    setUiEnabled(false);
    updateTrayIconState(StateWorking, 0);

    QMap<QString, QString> proxyVars = loadProxyEnvironment();
    QStringList fullArgs;

    fullArgs << "--user" << "root";

    if (!proxyVars.isEmpty()) {
        fullArgs << "/usr/bin/env";
        for (auto it = proxyVars.constBegin(); it != proxyVars.constEnd(); ++it) {
            fullArgs << QString("%1=%2").arg(it.key(), it.value());
        }
    }

    fullArgs << binaryPath << args;
    m_statusBanner->setText(statusMsg);
    m_consoleOutput->append(">>> Solicitando autenticación obligatoria de usuario ROOT...\n");
    m_process->start("pkexec", fullArgs);
}

void MainWindow::runUpdate() {
    executePrivilegedCommand("/usr/sbin/slackpkg", {"-batch=on", "-default_answer=y", "update"},
                             "Sincronizando listas oficiales con slackpkg (root)...");
}

void MainWindow::runUpgradeAll() {
    if (m_confirmedUpdates.isEmpty()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirmar Actualización (Requiere root)",
        QString("¿Deseas instalar los %1 paquete(s) confirmados por slackpkg?\n\n"
                "Se solicitará la contraseña de root para realizar esta operación.").arg(m_confirmedUpdates.size()),
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        executePrivilegedCommand("/usr/sbin/slackpkg", {"-batch=on", "-default_answer=y", "upgrade-all"},
                                 "Aplicando actualizaciones del sistema como root...");
    }
}

void MainWindow::runCleanSystem() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirmar Limpieza (Requiere root)",
        "¡Atención! Esto eliminará paquetes que no estén en el árbol oficial (incluyendo paquetes SBo).\n"
        "Se solicitará la contraseña de root.\n\n¿Estás seguro?",
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        executePrivilegedCommand("/usr/sbin/slackpkg", {"-batch=on", "-default_answer=y", "clean-system"},
                                 "Limpiando paquetes obsoletos como root...");
    }
}

void MainWindow::runRemovePackage(const QString &shortName, const QString &fullName) {
    QMessageBox::StandardButton reply = QMessageBox::warning(
        this, "Eliminar Paquete (Requiere root)",
        QString("¿Estás seguro de que deseas desinstalar el siguiente paquete?\n\n"
                "Paquete: %1\n"
                "Comando: slackpkg remove %2\n\n"
                "Se exigirá la contraseña de root para proceder.").arg(fullName, shortName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        executePrivilegedCommand("/usr/sbin/slackpkg", {"-batch=on", "-default_answer=y", "remove", shortName},
                                 QString("Eliminando paquete %1 como root...").arg(shortName));
    }
}

void MainWindow::readProcessOutput() {
    QByteArray data = m_process->readAllStandardOutput();
    QString text = stripAnsiCodes(QString::fromUtf8(data));

    static QRegularExpression percentRx("(\\d{1,3})%");
    QRegularExpressionMatch match = percentRx.match(text);
    if (match.hasMatch()) {
        int pct = match.captured(1).toInt();
        if (pct >= 0 && pct <= 100) {
            m_progressBar->setValue(pct);
        }
    } else {
        static QRegularExpression stepRx("\\(\\s*(\\d+)/(\\d+)\\s*\\)");
        QRegularExpressionMatch stepMatch = stepRx.match(text);
        if (stepMatch.hasMatch()) {
            int current = stepMatch.captured(1).toInt();
            int total = stepMatch.captured(2).toInt();
            if (total > 0) {
                int pct = (current * 100) / total;
                m_progressBar->setValue(pct);
            }
        }
    }

    m_consoleOutput->moveCursor(QTextCursor::End);
    m_consoleOutput->insertPlainText(text);
    m_consoleOutput->verticalScrollBar()->setValue(m_consoleOutput->verticalScrollBar()->maximum());
}

void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    setUiEnabled(true);
    m_consoleOutput->append(QString("\n>>> Proceso finalizado (Código: %1)").arg(exitCode));

    if (exitCode == 126 || exitCode == 127) {
        QMessageBox::warning(
            this,
            "Autenticación de ROOT Requerida",
            "No se ingresó una contraseña de root válida o la solicitud fue cancelada.\n\n"
            "Sin permisos de root, QSlackpkg opera únicamente en modo informativo y no realizará "
            "ningún cambio en el sistema."
        );
    }

    bool hadPendingKernelBefore = m_kernelInfo.newKernelPendingConfig;
    int dotNewBefore = m_btnDotNewManager->text().contains("(0)") ? 0 : 1;

    scanKernelEnvironment();
    scanPendingUpgrades();
    refreshDotNewBadge();
    scanSboRepository();
    loadAlienCatalogFromCache();

    if (!hadPendingKernelBefore && m_kernelInfo.newKernelPendingConfig) {
        m_tabs->setCurrentIndex(3);
        m_trayIcon->showMessage(
            "¡Nuevo Kernel instalado!",
            QString("Se detectó el kernel %1. Genere initrd y actualice %2 antes de reiniciar.")
                .arg(m_kernelInfo.latestInstalledKernel, m_kernelInfo.bootloaderType),
            QSystemTrayIcon::Warning, 8000
        );
    } else if (exitCode == 0 && dotNewBefore == 0 && !scanDotNewFiles().isEmpty()) {
        QMessageBox::StandardButton resp = QMessageBox::information(
            this,
            "Nuevos archivos de configuración (.new) detectados",
            QString("Se detectaron %1 archivo(s) *.new en /etc tras la actualización.\n"
                    "¿Deseas abrir ahora el Gestor Visual de .new para revisarlos?")
                .arg(scanDotNewFiles().size()),
            QMessageBox::Yes | QMessageBox::No
        );
        if (resp == QMessageBox::Yes) {
            showDotNewManagerDialog();
        }
    }
}

void MainWindow::setUiEnabled(bool enabled) {
    m_btnUpdate->setEnabled(enabled);
    m_btnClean->setEnabled(enabled);
    m_btnDotNewManager->setEnabled(enabled);
    m_btnBlacklistManager->setEnabled(enabled);
    m_btnInstallSbopkg->setEnabled(enabled && !QFile::exists("/usr/sbin/sbopkg"));
    m_btnSboSync->setEnabled(enabled && QFile::exists("/usr/sbin/sbopkg"));
    m_btnAlienSync->setEnabled(enabled);
    m_packageTable->setEnabled(enabled);
    m_sboTable->setEnabled(enabled);
    m_alienTable->setEnabled(enabled);

    m_btnRescanKernel->setEnabled(enabled);
    m_btnOpenKernelConfigurator->setEnabled(enabled);
    m_btnRunInitrdOnly->setEnabled(enabled);
    m_btnRunBootloaderOnly->setEnabled(enabled);
    m_btnRunFullKernelFlow->setEnabled(enabled);

    m_progressBar->setVisible(!enabled);
    if (!enabled) {
        m_btnUpgrade->setEnabled(false);
        m_btnPendingDetails->setEnabled(false);
        m_btnAlienUpgradeAll->setEnabled(false);
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

MainWindow::~MainWindow() {
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished(3000);
    }
}

void MainWindow::showAboutDialog() {
#ifndef APP_VERSION
#define APP_VERSION "v-1.0.0-compilación-dev"
#endif
#ifndef APP_BUILD_DATETIME
#define APP_BUILD_DATETIME __DATE__ " " __TIME__
#endif

    QString qtVer = QString::fromLatin1(qVersion());

    QString aboutHtml = QString(
        "<div style='font-family: sans-serif;'>"
        "<h2 style='color: #0d47a1; margin-bottom: 4px;'>QSlackpkg — Package &amp; Kernel Manager</h2>"
        "<p style='color: #37474f; font-size: 10pt; margin-top: 0;'>"
        "<b>Versión:</b> <code>%1</code><br>"
        "<b>Marca de compilación:</b> %2 &nbsp;|&nbsp; <b>Runtime Qt:</b> %3"
        "</p>"
        "<hr>"
        "<p>• <b>Slackware Linux</b> es una marca registrada de <b>Patrick Volkerding</b></p>"
        "<p>• <b>Linux</b> es una marca registrada de <b>Linus Torvals</b></p>"
        "<hr>"
        "<p>Centro de administración integral para <b>Slackware Linux</b>: gestión de paquetes "
        "oficiales (<code>slackpkg</code>), repositorio comunitario (<code>sbopkg</code> / SlackBuilds.org), "
        "binarios precompilados (AlienBOB), compilación y arranque del Kernel Linux "
        "(<code>mkinitrd</code>, ELILO, LILO, GRUB2) y creación segura de unidades USB booteables.</p>"
        "<p><b>Créditos y Desarrollo:</b><br>"
        "• <b>Desarrollador:</b> <i>[Slackzone Development]</i><br>"
        "• <b>Sistema Objetivo:</b> Slackware Linux (32-bit &amp; 64-bit)</p>"
        "<p><b>Tecnologías Utilizadas:</b><br>"
        "• <b>Lenguaje y Framework:</b> C++11/17 &amp; Qt Widgets / Qt Network (%3)<br>"
        "• <b>Herramientas de Sistema:</b> slackpkg, sbopkg, pkgtools (installpkg / upgradepkg / removepkg)<br>"
        "• <b>Kernel &amp; Bootloaders:</b> Kconfig, mkinitrd, ELILO, GRUB2, LILO<br>"
        "• <b>USB &amp; Seguridad:</b> lsblk (JSON), dd (isohybrid), usbimg2disk.sh, PolicyKit (pkexec)</p>"
        "</div>"
    ).arg(QString::fromUtf8(APP_VERSION), QString::fromUtf8(APP_BUILD_DATETIME), qtVer);

    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("Acerca de QSlackpkg — " + QString::fromUtf8(APP_VERSION));
    aboutBox.setTextFormat(Qt::RichText);
    aboutBox.setText(aboutHtml);
    if (!windowIcon().isNull()) {
        aboutBox.setIconPixmap(windowIcon().pixmap(64, 64));
    }
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.exec();
}
