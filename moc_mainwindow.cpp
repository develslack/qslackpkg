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
    QByteArrayData data[42];
    char stringdata0[696];
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
QT_MOC_LITERAL(9, 112, 16), // "runInstallSbopkg"
QT_MOC_LITERAL(10, 129, 10), // "runSboSync"
QT_MOC_LITERAL(11, 140, 14), // "filterSboTable"
QT_MOC_LITERAL(12, 155, 19), // "onInstallSboClicked"
QT_MOC_LITERAL(13, 175, 14), // "SboPackageInfo"
QT_MOC_LITERAL(14, 190, 3), // "pkg"
QT_MOC_LITERAL(15, 194, 12), // "runAlienSync"
QT_MOC_LITERAL(16, 207, 18), // "runAlienUpgradeAll"
QT_MOC_LITERAL(17, 226, 16), // "filterAlienTable"
QT_MOC_LITERAL(18, 243, 21), // "onInstallAlienClicked"
QT_MOC_LITERAL(19, 265, 16), // "AlienPackageInfo"
QT_MOC_LITERAL(20, 282, 20), // "onRemoveAlienClicked"
QT_MOC_LITERAL(21, 303, 21), // "scanKernelEnvironment"
QT_MOC_LITERAL(22, 325, 24), // "onKernelSelectionChanged"
QT_MOC_LITERAL(23, 350, 27), // "onToggleUnlockStorageParams"
QT_MOC_LITERAL(24, 378, 7), // "checked"
QT_MOC_LITERAL(25, 386, 23), // "updateBootloaderPreview"
QT_MOC_LITERAL(26, 410, 21), // "runGenerateInitrdOnly"
QT_MOC_LITERAL(27, 432, 23), // "runUpdateBootloaderOnly"
QT_MOC_LITERAL(28, 456, 21), // "runFullKernelWorkflow"
QT_MOC_LITERAL(29, 478, 17), // "readProcessOutput"
QT_MOC_LITERAL(30, 496, 15), // "processFinished"
QT_MOC_LITERAL(31, 512, 8), // "exitCode"
QT_MOC_LITERAL(32, 521, 20), // "QProcess::ExitStatus"
QT_MOC_LITERAL(33, 542, 10), // "exitStatus"
QT_MOC_LITERAL(34, 553, 13), // "toggleConsole"
QT_MOC_LITERAL(35, 567, 24), // "checkUpdatesInBackground"
QT_MOC_LITERAL(36, 592, 21), // "onChangeLogDownloaded"
QT_MOC_LITERAL(37, 614, 14), // "QNetworkReply*"
QT_MOC_LITERAL(38, 629, 5), // "reply"
QT_MOC_LITERAL(39, 635, 19), // "onTrayIconActivated"
QT_MOC_LITERAL(40, 655, 33), // "QSystemTrayIcon::ActivationRe..."
QT_MOC_LITERAL(41, 689, 6) // "reason"

    },
    "MainWindow\0runUpdate\0\0runUpgradeAll\0"
    "runCleanSystem\0runRemovePackage\0"
    "shortName\0fullName\0showPendingUpdatesDialog\0"
    "runInstallSbopkg\0runSboSync\0filterSboTable\0"
    "onInstallSboClicked\0SboPackageInfo\0"
    "pkg\0runAlienSync\0runAlienUpgradeAll\0"
    "filterAlienTable\0onInstallAlienClicked\0"
    "AlienPackageInfo\0onRemoveAlienClicked\0"
    "scanKernelEnvironment\0onKernelSelectionChanged\0"
    "onToggleUnlockStorageParams\0checked\0"
    "updateBootloaderPreview\0runGenerateInitrdOnly\0"
    "runUpdateBootloaderOnly\0runFullKernelWorkflow\0"
    "readProcessOutput\0processFinished\0"
    "exitCode\0QProcess::ExitStatus\0exitStatus\0"
    "toggleConsole\0checkUpdatesInBackground\0"
    "onChangeLogDownloaded\0QNetworkReply*\0"
    "reply\0onTrayIconActivated\0"
    "QSystemTrayIcon::ActivationReason\0"
    "reason"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      27,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  149,    2, 0x08 /* Private */,
       3,    0,  150,    2, 0x08 /* Private */,
       4,    0,  151,    2, 0x08 /* Private */,
       5,    2,  152,    2, 0x08 /* Private */,
       8,    0,  157,    2, 0x08 /* Private */,
       9,    0,  158,    2, 0x08 /* Private */,
      10,    0,  159,    2, 0x08 /* Private */,
      11,    0,  160,    2, 0x08 /* Private */,
      12,    1,  161,    2, 0x08 /* Private */,
      15,    0,  164,    2, 0x08 /* Private */,
      16,    0,  165,    2, 0x08 /* Private */,
      17,    0,  166,    2, 0x08 /* Private */,
      18,    1,  167,    2, 0x08 /* Private */,
      20,    1,  170,    2, 0x08 /* Private */,
      21,    0,  173,    2, 0x08 /* Private */,
      22,    0,  174,    2, 0x08 /* Private */,
      23,    1,  175,    2, 0x08 /* Private */,
      25,    0,  178,    2, 0x08 /* Private */,
      26,    0,  179,    2, 0x08 /* Private */,
      27,    0,  180,    2, 0x08 /* Private */,
      28,    0,  181,    2, 0x08 /* Private */,
      29,    0,  182,    2, 0x08 /* Private */,
      30,    2,  183,    2, 0x08 /* Private */,
      34,    0,  188,    2, 0x08 /* Private */,
      35,    0,  189,    2, 0x08 /* Private */,
      36,    1,  190,    2, 0x08 /* Private */,
      39,    1,  193,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    6,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 13,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 19,   14,
    QMetaType::Void, 0x80000000 | 19,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   24,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 32,   31,   33,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 37,   38,
    QMetaType::Void, 0x80000000 | 40,   41,

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
        case 5: _t->runInstallSbopkg(); break;
        case 6: _t->runSboSync(); break;
        case 7: _t->filterSboTable(); break;
        case 8: _t->onInstallSboClicked((*reinterpret_cast< const SboPackageInfo(*)>(_a[1]))); break;
        case 9: _t->runAlienSync(); break;
        case 10: _t->runAlienUpgradeAll(); break;
        case 11: _t->filterAlienTable(); break;
        case 12: _t->onInstallAlienClicked((*reinterpret_cast< const AlienPackageInfo(*)>(_a[1]))); break;
        case 13: _t->onRemoveAlienClicked((*reinterpret_cast< const AlienPackageInfo(*)>(_a[1]))); break;
        case 14: _t->scanKernelEnvironment(); break;
        case 15: _t->onKernelSelectionChanged(); break;
        case 16: _t->onToggleUnlockStorageParams((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 17: _t->updateBootloaderPreview(); break;
        case 18: _t->runGenerateInitrdOnly(); break;
        case 19: _t->runUpdateBootloaderOnly(); break;
        case 20: _t->runFullKernelWorkflow(); break;
        case 21: _t->readProcessOutput(); break;
        case 22: _t->processFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QProcess::ExitStatus(*)>(_a[2]))); break;
        case 23: _t->toggleConsole(); break;
        case 24: _t->checkUpdatesInBackground(); break;
        case 25: _t->onChangeLogDownloaded((*reinterpret_cast< QNetworkReply*(*)>(_a[1]))); break;
        case 26: _t->onTrayIconActivated((*reinterpret_cast< QSystemTrayIcon::ActivationReason(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 25:
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
        if (_id < 27)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 27;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 27)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 27;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
