#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QTextEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QTableWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTimer>
#include <QLabel>
#include <QCloseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTabWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QMap>
#include <QSet>
#include <QList>

enum TrayState {
    StateUpToDate,
    StateUpdatesAvailable,
    StateWorking
};

struct PendingUpdateItem {
    QString shortName;
    QString installedFull;
    QString candidateFull;
};

struct SboPackageInfo {
    QString name;
    QString category;
    QString version;
    QString shortDesc;
    QStringList requires;
    QString installedFull;
    bool hasUpdate = false;
};

struct AlienPackageInfo {
    QString name;           // Ej: vlc
    QString version;        // Ej: 3.0.21
    QString arch;           // Ej: x86_64
    QString build;          // Ej: 1alien
    QString fileName;       // Ej: vlc-3.0.21-x86_64-1alien.txz
    QString location;       // Ej: vlc/pkg64/15.0
    QString branchTag;      // Ej: pkg64/15.0
    QString compressedSize; // Ej: 48120 K
    QString requiredDeps;   // Dependencias declaradas en PACKAGES.TXT
    QString shortDesc;      // Descripción corta
    QString installedFull;  // Vacío si no está instalado
    bool hasUpdate = false;
};

struct KernelEnvironmentInfo {
    QString runningKernel;
    QStringList installedKernelVersions;
    QString latestInstalledKernel;
    bool newKernelPendingConfig = false;

    bool hasKernelSources = false;
    QString kernelSourceVersion;

    QString firmwareMode;       // "UEFI / EFI" o "Legacy BIOS / MBR"
    QString efiMountPoint;      // Ej. "/boot/efi"
    QString efiSlackwareDir;    // Ej. "/boot/efi/EFI/Slackware"

    QString bootloaderType;     // "ELILO", "LILO", "GRUB", "No detectado"
    QString bootloaderConfigPath;

    bool hasDualBoot = false;
    QStringList detectedOtherOS;

    QString rootDevice;         // Ej. "/dev/nvme0n1p2" o "/dev/sda2"
    QString rootFsType;         // Ej. "ext4", "reiserfs", "xfs", "btrfs"
    QString recommendedModules;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // Slackpkg slots
    void runUpdate();
    void runUpgradeAll();
    void runCleanSystem();
    void runRemovePackage(const QString &shortName, const QString &fullName);
    void showPendingUpdatesDialog();
    void filterOfficialTable();
    void showPackageDetailsDialog(int row, int column);
    void showDotNewManagerDialog();
    void showBlacklistManagerDialog();

    // Sbopkg slots
    void runInstallSbopkg();
    void runSboSync();
    void filterSboTable();
    void onInstallSboClicked(const SboPackageInfo &pkg);

    // AlienBOB slots (Repositorio de binarios de Eric Hameleers)
    void runAlienSync();
    void runAlienUpgradeAll();
    void filterAlienTable();
    void onInstallAlienClicked(const AlienPackageInfo &pkg);
    void onRemoveAlienClicked(const AlienPackageInfo &pkg);

    // Kernel & Bootloader slots
    void scanKernelEnvironment();
    void onKernelSelectionChanged();
    void onToggleUnlockStorageParams(bool checked);
    void updateBootloaderPreview();
    void runGenerateInitrdOnly();
    void runUpdateBootloaderOnly();
    void runFullKernelWorkflow();

    // Proceso y sistema
    void readProcessOutput();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void toggleConsole();
    void checkUpdatesInBackground();
    void onChangeLogDownloaded(QNetworkReply *reply);
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void setupTrayIcon();
    void updateTrayIconState(TrayState state, int count);
    QWidget* createOfficialTab();
    QWidget* createSboTab();
    QWidget* createAlienTab();
    QWidget* createKernelTab();

    void executePrivilegedCommand(const QString &binaryPath, const QStringList &args, const QString &statusMsg);
    void setUiEnabled(bool enabled);

    // Motores de escaneo
    void scanPendingUpgrades();
    QStringList queryRealSlackpkgUpgrades();
    QStringList scanDotNewFiles() const;
    void refreshDotNewBadge();
    void scanSboRepository();
    QString detectSboRepoPath();
    QStringList resolveSboDependencies(const QString &pkgName, const QMap<QString, QString> &installedMap);
    void resolveDepsRecursive(const QString &pkgName,
                              const QMap<QString, QString> &installedMap,
                              QSet<QString> &visited,
                              QStringList &orderedBuildList);

    // Helpers de AlienBOB
    void loadAlienCatalogFromCache();
    void parseAlienPackagesIndex(const QString &indexContent);
    QString detectAlienTargetBranch() const;
    QString getAlienCacheFilePath() const;
    QString buildAlienInstallSnippet(const AlienPackageInfo &pkg) const;

    // Helpers de Kernel y Bootloader
    QString detectRecommendedInitrdModules(const QString &kernelVer);
    QString buildBootloaderPreviewText() const;
    QString buildKernelExecutionScript(bool buildBzImage, bool buildInitrd, bool updateBootloader) const;
    bool validateCriticalBootFields();
    bool confirmRootKernelAction(const QString &actionTitle,
                                 const QStringList &configSummaryLines,
                                 const QString &commandPreview);

    QString getMirrorUrl();
    QString stripAnsiCodes(const QString &text);
    QMap<QString, QString> loadProxyEnvironment();

    QProcess *m_process;
    QNetworkAccessManager *m_netManager;
    QTimer *m_checkTimer;
    QTabWidget *m_tabs;

    // Cabecera y privilegios
    QLabel *m_lblPrivilegeBadge;

    // UI Pestaña Oficial (slackpkg)
    QLabel *m_statusBanner;
    QPushButton *m_btnPendingDetails;
    QTableWidget *m_packageTable;
    QPushButton *m_btnUpdate;
    QPushButton *m_btnUpgrade;
    QPushButton *m_btnClean;
    QPushButton *m_btnDotNewManager;
    QPushButton *m_btnBlacklistManager;
    QLineEdit *m_editOfficialSearch;
    QComboBox *m_comboOfficialFilter;
    QLabel *m_lblOfficialCount;

    // UI Pestaña SlackBuilds (sbopkg)
    QLabel *m_sboStatusLabel;
    QPushButton *m_btnInstallSbopkg;
    QPushButton *m_btnSboSync;
    QComboBox *m_comboSboCategory;
    QLineEdit *m_editSboSearch;
    QCheckBox *m_chkSboUpdatesOnly;
    QTableWidget *m_sboTable;

    // UI Pestaña Repositorio AlienBOB
    QLabel *m_alienStatusLabel;
    QPushButton *m_btnAlienSync;
    QPushButton *m_btnAlienUpgradeAll;
    QComboBox *m_comboAlienBranch;
    QLineEdit *m_editAlienSearch;
    QCheckBox *m_chkAlienInstalledOnly;
    QCheckBox *m_chkAlienUpdatesOnly;
    QTableWidget *m_alienTable;

    // UI Pestaña Kernel y Arranque
    QWidget *m_kernelAlertCard;
    QLabel *m_lblKernelAlertText;
    QPushButton *m_btnKernelAlertSelect;

    QLabel *m_lblDiagRunningKernel;
    QLabel *m_lblDiagInstalledKernels;
    QLabel *m_lblDiagKernelSources;
    QLabel *m_lblDiagFirmwareMode;
    QLabel *m_lblDiagBootloader;
    QLabel *m_lblDiagDualBoot;
    QPushButton *m_btnRescanKernel;

    QComboBox *m_comboTargetKernel;
    QComboBox *m_comboKernelVariant;

    QCheckBox *m_chkGenerateInitrd;
    QCheckBox *m_chkUnlockStorageParams;
    QLineEdit *m_editRootDevice;
    QLineEdit *m_editRootFs;
    QLineEdit *m_editInitrdModules;
    QCheckBox *m_chkIncludeUdev;

    QCheckBox *m_chkUpdateBootloader;
    QCheckBox *m_chkKeepFallbackEntry;
    QTextEdit *m_bootloaderPreview;

    QPushButton *m_btnRunInitrdOnly;
    QPushButton *m_btnRunBootloaderOnly;
    QPushButton *m_btnRunFullKernelFlow;

    // Consola compartida y progreso
    QTextEdit *m_consoleOutput;
    QPushButton *m_btnToggleConsole;
    QProgressBar *m_progressBar;

    // Datos en memoria
    QList<PendingUpdateItem> m_confirmedUpdates;
    QMap<QString, SboPackageInfo> m_sboCatalog;
    QList<AlienPackageInfo> m_alienCatalog;
    const QString m_alienBaseUrl = "http://www.slackware.com/~alien/slackbuilds/";
    KernelEnvironmentInfo m_kernelInfo;

    // Tray Icon
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    bool m_forceQuit = false;
    QString m_lastKnownChangeLogEntry;
};

#endif // MAINWINDOW_H
