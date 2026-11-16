/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[49];
    char stringdata0[832];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 9), // "runUpdate"
QT_MOC_LITERAL(2, 21, 0), // ""
QT_MOC_LITERAL(3, 22, 13), // "runUpgradeAll"
QT_MOC_LITERAL(4, 36, 14), // "runCleanSystem"
QT_MOC_LITERAL(5, 51, 16), // "runRemovePackage"
QT_MOC_LITERAL(6, 68, 9), // "shortName"
QT_MOC_LITERAL(7, 78, 8), // "fullName"
QT_MOC_LITERAL(8, 87, 24), // "showPendingUpdatesDialog"
QT_MOC_LITERAL(9, 112, 19), // "filterOfficialTable"
QT_MOC_LITERAL(10, 132, 24), // "showPackageDetailsDialog"
QT_MOC_LITERAL(11, 157, 3), // "row"
QT_MOC_LITERAL(12, 161, 6), // "column"
QT_MOC_LITERAL(13, 168, 23), // "showDotNewManagerDialog"
QT_MOC_LITERAL(14, 192, 26), // "showBlacklistManagerDialog"
QT_MOC_LITERAL(15, 219, 16), // "runInstallSbopkg"
QT_MOC_LITERAL(16, 236, 10), // "runSboSync"
QT_MOC_LITERAL(17, 247, 14), // "filterSboTable"
QT_MOC_LITERAL(18, 262, 19), // "onInstallSboClicked"
QT_MOC_LITERAL(19, 282, 14), // "SboPackageInfo"
QT_MOC_LITERAL(20, 297, 3), // "pkg"
QT_MOC_LITERAL(21, 301, 12), // "runAlienSync"
QT_MOC_LITERAL(22, 314, 18), // "runAlienUpgradeAll"
QT_MOC_LITERAL(23, 333, 16), // "filterAlienTable"
QT_MOC_LITERAL(24, 350, 21), // "onInstallAlienClicked"
QT_MOC_LITERAL(25, 372, 16), // "AlienPackageInfo"
QT_MOC_LITERAL(26, 389, 20), // "onRemoveAlienClicked"
QT_MOC_LITERAL(27, 410, 21), // "scanKernelEnvironment"
QT_MOC_LITERAL(28, 432, 24), // "onKernelSelectionChanged"
QT_MOC_LITERAL(29, 457, 27), // "onToggleUnlockStorageParams"
QT_MOC_LITERAL(30, 485, 7), // "checked"
QT_MOC_LITERAL(31, 493, 23), // "updateBootloaderPreview"
QT_MOC_LITERAL(32, 517, 21), // "runGenerateInitrdOnly"
QT_MOC_LITERAL(33, 539, 23), // "runUpdateBootloaderOnly"
QT_MOC_LITERAL(34, 563, 21), // "runFullKernelWorkflow"
QT_MOC_LITERAL(35, 585, 17), // "readProcessOutput"
QT_MOC_LITERAL(36, 603, 15), // "processFinished"
QT_MOC_LITERAL(37, 619, 8), // "exitCode"
QT_MOC_LITERAL(38, 628, 20), // "QProcess::ExitStatus"
QT_MOC_LITERAL(39, 649, 10), // "exitStatus"
QT_MOC_LITERAL(40, 660, 13), // "toggleConsole"
QT_MOC_LITERAL(41, 674, 24), // "checkUpdatesInBackground"
QT_MOC_LITERAL(42, 699, 21), // "onChangeLogDownloaded"
QT_MOC_LITERAL(43, 721, 14), // "QNetworkReply*"
QT_MOC_LITERAL(44, 736, 5), // "reply"
QT_MOC_LITERAL(45, 742, 19), // "onTrayIconActivated"
QT_MOC_LITERAL(46, 762, 33), // "QSystemTrayIcon::ActivationRe..."
QT_MOC_LITERAL(47, 796, 6), // "reason"
QT_MOC_LITERAL(48, 803, 28) // "openKernelConfiguratorDialog"

    },
    "MainWindow\0runUpdate\0\0runUpgradeAll\0"
    "runCleanSystem\0runRemovePackage\0"
    "shortName\0fullName\0showPendingUpdatesDialog\0"
    "filterOfficialTable\0showPackageDetailsDialog\0"
    "row\0column\0showDotNewManagerDialog\0"
    "showBlacklistManagerDialog\0runInstallSbopkg\0"
    "runSboSync\0filterSboTable\0onInstallSboClicked\0"
    "SboPackageInfo\0pkg\0runAlienSync\0"
    "runAlienUpgradeAll\0filterAlienTable\0"
    "onInstallAlienClicked\0AlienPackageInfo\0"
    "onRemoveAlienClicked\0scanKernelEnvironment\0"
    "onKernelSelectionChanged\0"
    "onToggleUnlockStorageParams\0checked\0"
    "updateBootloaderPreview\0runGenerateInitrdOnly\0"
    "runUpdateBootloaderOnly\0runFullKernelWorkflow\0"
    "readProcessOutput\0processFinished\0"
    "exitCode\0QProcess::ExitStatus\0exitStatus\0"
    "toggleConsole\0checkUpdatesInBackground\0"
    "onChangeLogDownloaded\0QNetworkReply*\0"
    "reply\0onTrayIconActivated\0"
    "QSystemTrayIcon::ActivationReason\0"
    "reason\0openKernelConfiguratorDialog"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      32,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  174,    2, 0x08 /* Private */,
       3,    0,  175,    2, 0x08 /* Private */,
       4,    0,  176,    2, 0x08 /* Private */,
       5,    2,  177,    2, 0x08 /* Private */,
       8,    0,  182,    2, 0x08 /* Private */,
       9,    0,  183,    2, 0x08 /* Private */,
      10,    2,  184,    2, 0x08 /* Private */,
      13,    0,  189,    2, 0x08 /* Private */,
      14,    0,  190,    2, 0x08 /* Private */,
      15,    0,  191,    2, 0x08 /* Private */,
      16,    0,  192,    2, 0x08 /* Private */,
      17,    0,  193,    2, 0x08 /* Private */,
      18,    1,  194,    2, 0x08 /* Private */,
      21,    0,  197,    2, 0x08 /* Private */,
      22,    0,  198,    2, 0x08 /* Private */,
      23,    0,  199,    2, 0x08 /* Private */,
      24,    1,  200,    2, 0x08 /* Private */,
      26,    1,  203,    2, 0x08 /* Private */,
      27,    0,  206,    2, 0x08 /* Private */,
      28,    0,  207,    2, 0x08 /* Private */,
      29,    1,  208,    2, 0x08 /* Private */,
      31,    0,  211,    2, 0x08 /* Private */,
      32,    0,  212,    2, 0x08 /* Private */,
      33,    0,  213,    2, 0x08 /* Private */,
      34,    0,  214,    2, 0x08 /* Private */,
      35,    0,  215,    2, 0x08 /* Private */,
      36,    2,  216,    2, 0x08 /* Private */,
      40,    0,  221,    2, 0x08 /* Private */,
      41,    0,  222,    2, 0x08 /* Private */,
      42,    1,  223,    2, 0x08 /* Private */,
      45,    1,  226,    2, 0x08 /* Private */,
      48,    0,  229,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    6,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   11,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 25,   20,
    QMetaType::Void, 0x80000000 | 25,   20,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   30,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 38,   37,   39,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 43,   44,
    QMetaType::Void, 0x80000000 | 46,   47,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->runUpdate(); break;
        case 1: _t->runUpgradeAll(); break;
        case 2: _t->runCleanSystem(); break;
        case 3: _t->runRemovePackage((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 4: _t->showPendingUpdatesDialog(); break;
        case 5: _t->filterOfficialTable(); break;
        case 6: _t->showPackageDetailsDialog((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 7: _t->showDotNewManagerDialog(); break;
        case 8: _t->showBlacklistManagerDialog(); break;
        case 9: _t->runInstallSbopkg(); break;
        case 10: _t->runSboSync(); break;
        case 11: _t->filterSboTable(); break;
        case 12: _t->onInstallSboClicked((*reinterpret_cast< const SboPackageInfo(*)>(_a[1]))); break;
        case 13: _t->runAlienSync(); break;
        case 14: _t->runAlienUpgradeAll(); break;
        case 15: _t->filterAlienTable(); break;
        case 16: _t->onInstallAlienClicked((*reinterpret_cast< const AlienPackageInfo(*)>(_a[1]))); break;
        case 17: _t->onRemoveAlienClicked((*reinterpret_cast< const AlienPackageInfo(*)>(_a[1]))); break;
        case 18: _t->scanKernelEnvironment(); break;
        case 19: _t->onKernelSelectionChanged(); break;
        case 20: _t->onToggleUnlockStorageParams((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 21: _t->updateBootloaderPreview(); break;
        case 22: _t->runGenerateInitrdOnly(); break;
        case 23: _t->runUpdateBootloaderOnly(); break;
        case 24: _t->runFullKernelWorkflow(); break;
        case 25: _t->readProcessOutput(); break;
        case 26: _t->processFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QProcess::ExitStatus(*)>(_a[2]))); break;
        case 27: _t->toggleConsole(); break;
        case 28: _t->checkUpdatesInBackground(); break;
        case 29: _t->onChangeLogDownloaded((*reinterpret_cast< QNetworkReply*(*)>(_a[1]))); break;
        case 30: _t->onTrayIconActivated((*reinterpret_cast< QSystemTrayIcon::ActivationReason(*)>(_a[1]))); break;
        case 31: _t->openKernelConfiguratorDialog(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 29:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QNetworkReply* >(); break;
            }
            break;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 32)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 32;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 32)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 32;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
