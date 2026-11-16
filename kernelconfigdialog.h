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

signals:
    void requestPrivilegedExecution(const QString &binaryPath,
                                    const QStringList &args,
                                    const QString &statusMsg);

private slots:
    // Paso 1: Fuentes y Kernel.org
    void fetchKernelOrgReleases();
    void onKernelOrgReleasesDownloaded(QNetworkReply *reply);
    void onSourceSelectionChanged();
    void downloadAndExtractSelectedKernel();

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

private:
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

    QNetworkAccessManager *m_netManager;
    QString m_rootDevice;
    QString m_rootFs;
    QString m_bootloaderType;
    QString m_bootloaderConfigPath;
    QString m_efiSlackwareDir;
    bool m_keepFallbackEntry;

    // UI Paso 1: Selección / Descarga de Kernel
    QComboBox *m_comboSourceSelect;
    QPushButton *m_btnFetchKernelOrg;
    QPushButton *m_btnDownloadSource;
    QLabel *m_lblSourceStatus;

    // UI Paso 2: Herencia de módulos actuales
    QPushButton *m_btnInheritFullConfig;
    QPushButton *m_btnInheritLocalModConfig;
    QLabel *m_lblConfigStatus;

    // UI Paso 3: Configurador visual Kconfig
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
    QSpinBox *m_spinJobs;
    QCheckBox *m_chkBuildInitrdAfter;
    QCheckBox *m_chkUpdateBootAfter;
    QPushButton *m_btnSaveConfigOnly;
    QPushButton *m_btnCompileAndInstall;

    // Datos internos
    QString m_activeSourceDir;
    QList<KernelReleaseInfo> m_remoteReleases;
    QMap<QString, QString> m_dotConfigValues;        // SYMBOL -> "y" | "m" | "n" | "\"str\""
    QMap<QString, KconfigOption> m_kconfigOptions;   // SYMBOL -> KconfigOption
    QMap<QString, QTreeWidgetItem*> m_symbolToItem;
    bool m_updatingUi = false;
};

#endif // KERNELCONFIGDIALOG_H
