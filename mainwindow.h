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
#include <QMap>
#include <QSet>

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
    QString installedFull; // Vacío si no está instalado
    bool hasUpdate = false;
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

    // Sbopkg slots
    void runSboSync();
    void filterSboTable();
    void onInstallSboClicked(const SboPackageInfo &pkg);

    // Proceso y sistema
    void readProcessOutput();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void toggleConsole();
    void checkUpdatesInBackground();
    void onChangeLogDownloaded(QNetworkReply *reply);
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void setupTrayIcon();
    QWidget* createOfficialTab();
    QWidget* createSboTab();

    void executePrivilegedCommand(const QString &binaryPath, const QStringList &args, const QString &statusMsg);
    void setUiEnabled(bool enabled);

    // Motores de escaneo
    void scanPendingUpgrades();
    QStringList queryRealSlackpkgUpgrades();
    void scanSboRepository();
    QString detectSboRepoPath();
    QStringList resolveSboDependencies(const QString &pkgName, const QMap<QString, QString> &installedMap);
    void resolveDepsRecursive(const QString &pkgName,
                              const QMap<QString, QString> &installedMap,
                              QSet<QString> &visited,
                              QStringList &orderedBuildList);

    QString getMirrorUrl();
    QString stripAnsiCodes(const QString &text);
    QMap<QString, QString> loadProxyEnvironment();

    QProcess *m_process;
    QNetworkAccessManager *m_netManager;
    QTimer *m_checkTimer;
    QTabWidget *m_tabs;

    // UI Pestaña Oficial (slackpkg)
    QLabel *m_statusBanner;
    QPushButton *m_btnPendingDetails;
    QTableWidget *m_packageTable;
    QPushButton *m_btnUpdate;
    QPushButton *m_btnUpgrade;
    QPushButton *m_btnClean;

    // UI Pestaña SlackBuilds (sbopkg)
    QLabel *m_sboStatusLabel;
    QPushButton *m_btnSboSync;
    QComboBox *m_comboSboCategory;
    QLineEdit *m_editSboSearch;
    QCheckBox *m_chkSboUpdatesOnly;
    QTableWidget *m_sboTable;

    // Consola compartida y progreso
    QTextEdit *m_consoleOutput;
    QPushButton *m_btnToggleConsole;
    QProgressBar *m_progressBar;

    // Datos en memoria
    QList<PendingUpdateItem> m_confirmedUpdates;
    QMap<QString, SboPackageInfo> m_sboCatalog;

    // Tray Icon
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    bool m_forceQuit = false;
    QString m_lastKnownChangeLogEntry;
};

#endif // MAINWINDOW_H
