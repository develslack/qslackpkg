#ifndef MULTILIBWIDGET_H
#define MULTILIBWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QGroupBox>
#include <QTableWidget>
#include <QProcess>
#include <QNetworkAccessManager>

class MultilibWidget : public QWidget {
    Q_OBJECT

public:
    explicit MultilibWidget(QNetworkAccessManager *netManager,
                            const QString &officialMirrorUrl,
                            QWidget *parent = nullptr);
    ~MultilibWidget();

    bool isBusy() const;

signals:
    void multilibStateChanged();

private slots:
    void scanMultilibStatus();
    void runInstallOrUpdateMultilib();
    void runRevertToPure64();
    void readProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    enum MultilibTask {
        TaskNone,
        TaskInstallUpdate,
        TaskRevertPure64
    };

    void setControlsEnabled(bool enabled);
    QString detectSlackwareBranch() const;
    bool isRuleInBlacklist(const QString &rule) const;

    QNetworkAccessManager *m_netManager;
    QString m_officialMirrorUrl;
    QString m_detectedBranch; // "15.0" o "current"
    bool m_is64Bit = false;

    // UI Diagnóstico
    QGroupBox *m_grpStatus;
    QLabel *m_lblArchInfo;
    QLabel *m_lblCoreStatus;
    QLabel *m_lblCompat32Count;
    QLabel *m_lblBlacklistStatus;
    QPushButton *m_btnRescan;

    // UI Acciones
    QGroupBox *m_grpActions;
    QCheckBox *m_chkAutoBlacklist;
    QPushButton *m_btnInstallMultilib;
    QPushButton *m_btnRevertPure64;

    // Tabla de paquetes multilib / compat32 instalados
    QTableWidget *m_pkgTable;

    // Progreso y consola
    QProgressBar *m_progressBar;
    QLabel *m_lblProgressStatus;
    QTextEdit *m_consoleLog;

    QProcess *m_process;
    MultilibTask m_currentTask = TaskNone;
    QStringList m_installedCompat32List;
    QStringList m_installedCoreMultilibList;
};

#endif // MULTILIBWIDGET_H
