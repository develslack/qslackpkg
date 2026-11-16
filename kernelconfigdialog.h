#ifndef KERNELCONFIGDIALOG_H
#define KERNELCONFIGDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QProgressBar>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMap>
#include <QSet>
#include <QList>

struct KernelReleaseInfo {
    QString version;
    QString moniker;    // mainline, stable, longterm
    QString released;
    QString sourceUrl;
};

struct KconfigOption {
    QString symbol;         // Ej: "NVME_CORE" (sin prefijo CONFIG_)
    QString prompt;         // Ej: "NVM Express block device"
    QString type;           // "bool", "tristate", "string", "int", "hex", "menu"
    QString defaultVal;     // "y", "m", "n" o literal
    QString dependsOn;
    QString selects;
    QString helpText;
    QString sourceFile;
    QString value;          // Estado actual: "y", "m", "n" o valor texto/numérico
    bool isMenuOnly = false;
};

class KernelConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit KernelConfigDialog(QNetworkAccessManager *netManager,
                                const QString &rootDevice,
                                const QString &rootFs,
                                const QString &bootloaderType,
                                const QString &bootloaderConfigPath,
                                const QString &efiSlackwareDir,
                                bool keepFallbackEntry,
                                QWidget *parent = nullptr);
    ~KernelConfigDialog();

signals:
    void kernelInstalledNotification(const QString &kernelVersion);
    void environmentChanged();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // Paso 1: Fuentes y Kernel.org
    void fetchKernelOrgReleases();
    void onKernelOrgReleasesDownloaded(QNetworkReply *reply);
    void onSourceSelectionChanged();
    void downloadAndExtractSelectedKernel();

    // Modo Seguro vs Modo Avanzado
    void toggleAdvancedKernelMode();

    // Paso 2: Herencia de configuración
    void inheritCurrentKernelConfig(bool runLocalModConfig);

    // Paso 3: Árbol Kconfig visual
    void loadKconfigTree();
    void onTreeItemSelected(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void onOptionStateChanged();
    void filterKconfigTree(const QString &query);

    // Paso 4: Guardar y compilar
    void saveDotConfigOnly();
    void compileAndInstallCustomKernel();

    // Ejecución interna persistente (sin cerrar el diálogo)
    void readInternalProcessOutput();
    void onInternalProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    enum TaskType {
        TaskNone,
        TaskDownloadSource,
        TaskInheritConfig,
        TaskSaveConfig,
        TaskCompileKernel,
        TaskRebootSystem
    };

    void applyDialogModeTheme();
    void setDialogBusy(bool busy, const QString &statusText = QString());
    void runInternalPrivilegedTask(TaskType type, const QString &script, const QString &statusMsg);

    void scanLocalKernelSources();
    void loadCurrentDotConfigValues(const QString &dotConfigPath);
    void parseKconfigRecursive(const QString &kconfigRelPath,
                               QTreeWidgetItem *parentItem,
                               QSet<QString> &visitedFiles,
                               int depth = 0);
    void updateTreeItemVisual(QTreeWidgetItem *item, const KconfigOption &opt);
    bool filterTreeRecursive(QTreeWidgetItem *item, const QString &query);
    QString generateDotConfigContent() const;
    QString readKernelVersionFromMakefile(const QString &srcDir) const;
    QString buildLiloUpdateShellScript(const QString &kernelFileName,
                                       const QString &initrdFileName,
                                       bool doInitrd) const;
    QString buildBootloaderPreviewSnippet(const QString &kernelFileName,
                                          const QString &initrdFileName,
                                          bool doInitrd) const;
    bool showPreCompileReportDialog(const QString &targetVer,
                                    int jobs,
                                    bool doInitrd,
                                    bool doBoot);

    QNetworkAccessManager *m_netManager;
    QString m_rootDevice;
    QString m_rootFs;
    QString m_bootloaderType;
    QString m_bootloaderConfigPath;
    QString m_efiSlackwareDir;
    bool m_keepFallbackEntry;

    // Estado del modo (Seguro por defecto vs Avanzado en rojo)
    bool m_advancedMode = false;

    // UI Cabecera y Botón de Modo Avanzado
    QLabel *m_lblTopBanner;
    QPushButton *m_btnToggleAdvancedMode;

    // UI Paso 1: Selección / Descarga de Kernel
    QGroupBox *m_grpStep1;
    QComboBox *m_comboSourceSelect;
    QPushButton *m_btnFetchKernelOrg;
    QPushButton *m_btnDownloadSource;
    QLabel *m_lblSourceStatus;

    // UI Paso 2: Herencia de módulos actuales
    QGroupBox *m_grpStep2;
    QPushButton *m_btnInheritFullConfig;
    QPushButton *m_btnInheritLocalModConfig;
    QLabel *m_lblConfigStatus;
#ifndef KERNELCONFIGDIALOG_H
#define KERNELCONFIGDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QProgressBar>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMap>
#include <QSet>
#include <QList>
#include <QStandardItemModel>

struct KernelReleaseInfo {
    QString version;
    QString moniker;    // mainline, stable, longterm
    QString released;
    QString sourceUrl;
};

struct KconfigOption {
    QString symbol;         // Ej: "NVME_CORE" (sin prefijo CONFIG_)
    QString prompt;         // Ej: "NVM Express block device"
    QString type;           // "bool", "tristate", "string", "int", "hex", "menu"
    QString defaultVal;     // "y", "m", "n" o literal
    QString dependsOn;
    QString selects;
    QString helpText;
    QString sourceFile;
    QString value;          // Estado actual: "y", "m", "n" o valor texto/numérico
    bool isMenuOnly = false;
};

class KernelConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit KernelConfigDialog(QNetworkAccessManager *netManager,
                                const QString &rootDevice,
                                const QString &rootFs,
                                const QString &bootloaderType,
                                const QString &bootloaderConfigPath,
                                const QString &efiSlackwareDir,
                                bool keepFallbackEntry,
                                QWidget *parent = nullptr);
    ~KernelConfigDialog();

signals:
    void kernelInstalledNotification(const QString &kernelVersion);
    void environmentChanged();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // Paso 1: Fuentes y Kernel.org
    void fetchKernelOrgReleases();
    void onKernelOrgReleasesDownloaded(QNetworkReply *reply);
    void onSourceSelectionChanged();
    void downloadAndExtractSelectedKernel();

    // Modo Seguro vs Modo Avanzado
    void toggleAdvancedKernelMode();

    // Paso 2: Herencia de configuración
    void inheritCurrentKernelConfig(bool runLocalModConfig);

    // Paso 3: Árbol Kconfig visual
    void loadKconfigTree();
    void onTreeItemSelected(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void onOptionStateChanged();
    void filterKconfigTree(const QString &query);

    // Paso 4: Guardar y compilar
    void saveDotConfigOnly();
    void compileAndInstallCustomKernel();

    // Ejecución interna persistente (sin cerrar el diálogo)
    void readInternalProcessOutput();
    void onInternalProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    enum TaskType {
        TaskNone,
        TaskDownloadSource,
        TaskInheritConfig,
        TaskSaveConfig,
        TaskCompileKernel,
        TaskRebootSystem
    };

    void applyDialogModeTheme();
    void setDialogBusy(bool busy, const QString &statusText = QString());
    void runInternalPrivilegedTask(TaskType type, const QString &script, const QString &statusMsg);

    void scanLocalKernelSources();
    void loadCurrentDotConfigValues(const QString &dotConfigPath);
    void parseKconfigRecursive(const QString &kconfigRelPath,
                               QTreeWidgetItem *parentItem,
                               QSet<QString> &visitedFiles,
                               int depth = 0);
    void updateTreeItemVisual(QTreeWidgetItem *item, const KconfigOption &opt);
    bool filterTreeRecursive(QTreeWidgetItem *item, const QString &query);
    QString generateDotConfigContent() const;
    QString readKernelVersionFromMakefile(const QString &srcDir) const;
    QString buildLiloUpdateShellScript(const QString &kernelFileName,
                                       const QString &initrdFileName,
                                       bool doInitrd) const;
    QString buildBootloaderPreviewSnippet(const QString &kernelFileName,
                                          const QString &initrdFileName,
                                          bool doInitrd) const;
    bool showPreCompileReportDialog(const QString &targetVer,
                                    int jobs,
                                    bool doInitrd,
                                    bool doBoot);

    QNetworkAccessManager *m_netManager;
    QString m_rootDevice;
    QString m_rootFs;
    QString m_bootloaderType;
    QString m_bootloaderConfigPath;
    QString m_efiSlackwareDir;
    bool m_keepFallbackEntry;

    // Estado del modo (Seguro por defecto vs Avanzado en rojo)
    bool m_advancedMode = false;

    // UI Cabecera y Botón de Modo Avanzado
    QLabel *m_lblTopBanner;
    QPushButton *m_btnToggleAdvancedMode;

    // UI Paso 1: Selección / Descarga de Kernel
    QGroupBox *m_grpStep1;
    QComboBox *m_comboSourceSelect;
    QPushButton *m_btnFetchKernelOrg;
    QPushButton *m_btnDownloadSource;
    QLabel *m_lblSourceStatus;

    // UI Paso 2: Herencia de módulos actuales
    QGroupBox *m_grpStep2;
    QPushButton *m_btnInheritFullConfig;
    QPushButton *m_btnInheritLocalModConfig;
    QLabel *m_lblConfigStatus;

    // UI Paso 3: Configurador visual Kconfig
    QGroupBox *m_grpStep3;
    QLineEdit *m_editKconfigSearch;
    QCheckBox *m_chkShowEnabledOnly;
    QLabel *m_lblOptionsCounter;
    QTreeWidget *m_kconfigTree;

    // Panel derecho de inspección y edición de símbolo
    QLabel *m_lblOptTitle;
    QLabel *m_lblOptSymbol;
    QLabel *m_lblOptDepends;
    QComboBox *m_comboOptTriState;
    QLineEdit *m_editOptValue;
    QTextEdit *m_txtOptHelp;

    // UI Paso 4: Compilación e instalación
    QGroupBox *m_grpStep4;
    QSpinBox *m_spinJobs;
    QCheckBox *m_chkBuildInitrdAfter;
    QCheckBox *m_chkUpdateBootAfter;
    QPushButton *m_btnSaveConfigOnly;
    QPushButton *m_btnCompileAndInstall;
    QPushButton *m_btnCloseDialog;

    // Consola interna y barra de progreso persistente
    QLabel *m_lblTaskProgressStatus;
    QProgressBar *m_internalProgress;
    QTextEdit *m_internalConsole;
    QProcess *m_internalProcess;
    TaskType m_currentTask = TaskNone;
    QString m_pendingCompiledVersion;

    // Datos internos
    QString m_activeSourceDir;
    QList<KernelReleaseInfo> m_remoteReleases;
    QMap<QString, QString> m_dotConfigValues;        // SYMBOL -> "y" | "m" | "n" | "\"str\""
    QMap<QString, KconfigOption> m_kconfigOptions;   // SYMBOL -> KconfigOption
    QMap<QString, QTreeWidgetItem*> m_symbolToItem;
    bool m_updatingUi = false;
};

#endif // KERNELCONFIGDIALOG_H
    // UI Paso 3: Configurador visual Kconfig
    QGroupBox *m_grpStep3;
    QLineEdit *m_editKconfigSearch;
    QCheckBox *m_chkShowEnabledOnly;
    QLabel *m_lblOptionsCounter;
    QTreeWidget *m_kconfigTree;

    // Panel derecho de inspección y edición de símbolo
    QLabel *m_lblOptTitle;
    QLabel *m_lblOptSymbol;
    QLabel *m_lblOptDepends;
    QComboBox *m_comboOptTriState;
    QLineEdit *m_editOptValue;
    QTextEdit *m_txtOptHelp;

    // UI Paso 4: Compilación e instalación
    QGroupBox *m_grpStep4;
    QSpinBox *m_spinJobs;
    QCheckBox *m_chkBuildInitrdAfter;
    QCheckBox *m_chkUpdateBootAfter;
    QPushButton *m_btnSaveConfigOnly;
    QPushButton *m_btnCompileAndInstall;
    QPushButton *m_btnCloseDialog;

    // Consola interna y barra de progreso persistente
    QLabel *m_lblTaskProgressStatus;
    QProgressBar *m_internalProgress;
    QTextEdit *m_internalConsole;
    QProcess *m_internalProcess;
    TaskType m_currentTask = TaskNone;
    QString m_pendingCompiledVersion;

    // Datos internos
    QString m_activeSourceDir;
    QList<KernelReleaseInfo> m_remoteReleases;
    QMap<QString, QString> m_dotConfigValues;        // SYMBOL -> "y" | "m" | "n" | "\"str\""
    QMap<QString, KconfigOption> m_kconfigOptions;   // SYMBOL -> KconfigOption
    QMap<QString, QTreeWidgetItem*> m_symbolToItem;
    bool m_updatingUi = false;
};

#endif // KERNELCONFIGDIALOG_H
