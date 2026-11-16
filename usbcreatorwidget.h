#ifndef USBCREATORWIDGET_H
#define USBCREATORWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QGroupBox>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QList>

struct UsbDriveInfo {
    QString deviceNode;     // Ej: /dev/sdb
    QString name;           // Ej: sdb
    QString model;          // Ej: SanDisk Cruzer Blade
    QString vendor;         // Ej: SanDisk
    QString sizeHuman;      // Ej: 14.9G
    qint64 sizeBytes = 0;
    QString transport;      // Ej: usb
    QString mountPoints;    // Particiones montadas actualmente
};

struct SlackwareImageRelease {
    QString label;          // Ej: "Slackware 15.0 (64-bit) — DVD de Instalación Completo (.iso)"
    QString branchDir;      // Ej: "slackware64-15.0"
    QString archTag;        // "64-bit" o "32-bit"
    QString imageKind;      // "iso" o "usbboot"
    QString expectedFile;   // Ej: "slackware64-15.0-install-dvd.iso" o vacío si es -current dinámico
};

class UsbCreatorWidget : public QWidget {
    Q_OBJECT

public:
    explicit UsbCreatorWidget(QNetworkAccessManager *netManager,
                              const QString &mirrorUrl,
                              QWidget *parent = nullptr);
    ~UsbCreatorWidget();

    bool isBusy() const;

signals:
    void usbCreationCompleted(const QString &summaryMessage);

private slots:
    void scanUsbDevices();
    void onUsbDeviceChanged();
    void browseLocalImage();
    void onImagePathChanged();
    void calculateImageChecksum();
    void openOfficialImageDownloadDialog();
    void startUsbCreation();

    void readProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    enum UsbTask {
        TaskNone,
        TaskDownloadImage,
        TaskWriteUsb
    };

    void setControlsEnabled(bool enabled);
    QString formatBytes(qint64 bytes) const;
    QString getAppIsoDirectory() const;
    QString getNormalizedMirrorRoot() const;
    QList<SlackwareImageRelease> buildAvailableSlackwareReleases() const;

    QNetworkAccessManager *m_netManager;
    QString m_mirrorUrl;

    // Paso 1: Selección de imagen ISO / IMG
    QGroupBox *m_grpImage;
    QLineEdit *m_editImagePath;
    QPushButton *m_btnBrowseImage;
    QPushButton *m_btnDownloadOfficialImg;
    QPushButton *m_btnVerifyHash;
    QLabel *m_lblImageInfo;

    // Paso 2: Selección de unidad USB
    QGroupBox *m_grpDevice;
    QComboBox *m_comboUsbDevices;
    QPushButton *m_btnRefreshDevices;
    QLabel *m_lblDeviceDetails;

    // Paso 3: Modo de grabación (Estilo Rufus / Etcher)
    QGroupBox *m_grpMode;
    QRadioButton *m_radioHybridDd;
    QRadioButton *m_radioSlackwareUsbImg2Disk;
    QCheckBox *m_chkVerifyAfterWrite;
    QLabel *m_lblModeExplanation;

    // Paso 4: Confirmación y ejecución
    QGroupBox *m_grpAction;
    QCheckBox *m_chkConfirmDataLoss;
    QPushButton *m_btnStartWrite;
    QProgressBar *m_progressBar;
    QLabel *m_lblProgressStatus;
    QTextEdit *m_consoleLog;

    QProcess *m_process;
    UsbTask m_currentTask = TaskNone;
    QList<UsbDriveInfo> m_detectedDrives;
    qint64 m_selectedImageSize = 0;
};

#endif // USBCREATORWIDGET_H
