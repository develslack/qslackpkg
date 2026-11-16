#ifndef SYSTEMHEALTHWIDGET_H
#define SYSTEMHEALTHWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QGroupBox>
#include <QTableWidget>
#include <QProcess>

struct BrokenLibFinding {
    QString binaryPath;
    QString ownerPackage;
    QString missingLibrary;
    QString suggestedProvider;
};

struct InactiveKernelItem {
    QString version;
    QStringList filesInBoot;
    qint64 totalBytes = 0;
};

class SystemHealthWidget : public QWidget {
    Q_OBJECT

public:
    explicit SystemHealthWidget(QWidget *parent = nullptr);
    ~SystemHealthWidget();

    bool isBusy() const;

signals:
    void systemCleaned();

private slots:
    void runBrokenLibsAudit();
    void onBrokenLibsProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void scanDiskUsage();
    void runSafeCleanup();

    void readRootProcessOutput();
    void onRootProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QString formatBytes(qint64 bytes) const;
    qint64 calculateDirectorySize(const QString &dirPath) const;
    QString findOwnerPackageForFile(const QString &filePath) const;
    QString findProviderForMissingLib(const QString &libName) const;

    // Sección 1: Auditoría de librerías rotas (ldd)
    QGroupBox *m_grpLdd;
    QPushButton *m_btnScanLibs;
    QLabel *m_lblLddSummary;
    QTableWidget *m_tableBrokenLibs;

    // Sección 2: Limpieza segura de cachés y kernels inactivos
    QGroupBox *m_grpCleanup;
    QCheckBox *m_chkCleanSlackpkgCache;
    QCheckBox *m_chkCleanSboTmp;
    QCheckBox *m_chkCleanOldKernels;
    QLabel *m_lblInactiveKernelsDetail;
    QPushButton *m_btnRefreshDisk;
    QPushButton *m_btnExecuteCleanup;

    // Progreso y consola
    QProgressBar *m_progressBar;
    QLabel *m_lblStatus;
    QTextEdit *m_consoleLog;

    QProcess *m_auditProcess;
    QProcess *m_rootProcess;

    qint64 m_slackpkgCacheBytes = 0;
    qint64 m_sboTmpBytes = 0;
    QList<InactiveKernelItem> m_inactiveKernels;
};

#endif // SYSTEMHEALTHWIDGET_H
