/****************************************************************************
** Meta object code from reading C++ file 'kernelconfigdialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "kernelconfigdialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'kernelconfigdialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_KernelConfigDialog_t {
    QByteArrayData data[29];
    char stringdata0[538];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_KernelConfigDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_KernelConfigDialog_t qt_meta_stringdata_KernelConfigDialog = {
    {
QT_MOC_LITERAL(0, 0, 18), // "KernelConfigDialog"
QT_MOC_LITERAL(1, 19, 27), // "kernelInstalledNotification"
QT_MOC_LITERAL(2, 47, 0), // ""
QT_MOC_LITERAL(3, 48, 13), // "kernelVersion"
QT_MOC_LITERAL(4, 62, 18), // "environmentChanged"
QT_MOC_LITERAL(5, 81, 22), // "fetchKernelOrgReleases"
QT_MOC_LITERAL(6, 104, 29), // "onKernelOrgReleasesDownloaded"
QT_MOC_LITERAL(7, 134, 14), // "QNetworkReply*"
QT_MOC_LITERAL(8, 149, 5), // "reply"
QT_MOC_LITERAL(9, 155, 24), // "onSourceSelectionChanged"
QT_MOC_LITERAL(10, 180, 32), // "downloadAndExtractSelectedKernel"
QT_MOC_LITERAL(11, 213, 24), // "toggleAdvancedKernelMode"
QT_MOC_LITERAL(12, 238, 26), // "inheritCurrentKernelConfig"
QT_MOC_LITERAL(13, 265, 17), // "runLocalModConfig"
QT_MOC_LITERAL(14, 283, 15), // "loadKconfigTree"
QT_MOC_LITERAL(15, 299, 18), // "onTreeItemSelected"
QT_MOC_LITERAL(16, 318, 16), // "QTreeWidgetItem*"
QT_MOC_LITERAL(17, 335, 7), // "current"
QT_MOC_LITERAL(18, 343, 8), // "previous"
QT_MOC_LITERAL(19, 352, 20), // "onOptionStateChanged"
QT_MOC_LITERAL(20, 373, 17), // "filterKconfigTree"
QT_MOC_LITERAL(21, 391, 5), // "query"
QT_MOC_LITERAL(22, 397, 17), // "saveDotConfigOnly"
QT_MOC_LITERAL(23, 415, 29), // "compileAndInstallCustomKernel"
QT_MOC_LITERAL(24, 445, 25), // "readInternalProcessOutput"
QT_MOC_LITERAL(25, 471, 25), // "onInternalProcessFinished"
QT_MOC_LITERAL(26, 497, 8), // "exitCode"
QT_MOC_LITERAL(27, 506, 20), // "QProcess::ExitStatus"
QT_MOC_LITERAL(28, 527, 10) // "exitStatus"

    },
    "KernelConfigDialog\0kernelInstalledNotification\0"
    "\0kernelVersion\0environmentChanged\0"
    "fetchKernelOrgReleases\0"
    "onKernelOrgReleasesDownloaded\0"
    "QNetworkReply*\0reply\0onSourceSelectionChanged\0"
    "downloadAndExtractSelectedKernel\0"
    "toggleAdvancedKernelMode\0"
    "inheritCurrentKernelConfig\0runLocalModConfig\0"
    "loadKconfigTree\0onTreeItemSelected\0"
    "QTreeWidgetItem*\0current\0previous\0"
    "onOptionStateChanged\0filterKconfigTree\0"
    "query\0saveDotConfigOnly\0"
    "compileAndInstallCustomKernel\0"
    "readInternalProcessOutput\0"
    "onInternalProcessFinished\0exitCode\0"
    "QProcess::ExitStatus\0exitStatus"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_KernelConfigDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   94,    2, 0x06 /* Public */,
       4,    0,   97,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       5,    0,   98,    2, 0x08 /* Private */,
       6,    1,   99,    2, 0x08 /* Private */,
       9,    0,  102,    2, 0x08 /* Private */,
      10,    0,  103,    2, 0x08 /* Private */,
      11,    0,  104,    2, 0x08 /* Private */,
      12,    1,  105,    2, 0x08 /* Private */,
      14,    0,  108,    2, 0x08 /* Private */,
      15,    2,  109,    2, 0x08 /* Private */,
      19,    0,  114,    2, 0x08 /* Private */,
      20,    1,  115,    2, 0x08 /* Private */,
      22,    0,  118,    2, 0x08 /* Private */,
      23,    0,  119,    2, 0x08 /* Private */,
      24,    0,  120,    2, 0x08 /* Private */,
      25,    2,  121,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   13,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 16, 0x80000000 | 16,   17,   18,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   21,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 27,   26,   28,

       0        // eod
};

void KernelConfigDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<KernelConfigDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->kernelInstalledNotification((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->environmentChanged(); break;
        case 2: _t->fetchKernelOrgReleases(); break;
        case 3: _t->onKernelOrgReleasesDownloaded((*reinterpret_cast< QNetworkReply*(*)>(_a[1]))); break;
        case 4: _t->onSourceSelectionChanged(); break;
        case 5: _t->downloadAndExtractSelectedKernel(); break;
        case 6: _t->toggleAdvancedKernelMode(); break;
        case 7: _t->inheritCurrentKernelConfig((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->loadKconfigTree(); break;
        case 9: _t->onTreeItemSelected((*reinterpret_cast< QTreeWidgetItem*(*)>(_a[1])),(*reinterpret_cast< QTreeWidgetItem*(*)>(_a[2]))); break;
        case 10: _t->onOptionStateChanged(); break;
        case 11: _t->filterKconfigTree((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 12: _t->saveDotConfigOnly(); break;
        case 13: _t->compileAndInstallCustomKernel(); break;
        case 14: _t->readInternalProcessOutput(); break;
        case 15: _t->onInternalProcessFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QProcess::ExitStatus(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QNetworkReply* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (KernelConfigDialog::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&KernelConfigDialog::kernelInstalledNotification)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (KernelConfigDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&KernelConfigDialog::environmentChanged)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject KernelConfigDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_KernelConfigDialog.data,
    qt_meta_data_KernelConfigDialog,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *KernelConfigDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *KernelConfigDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_KernelConfigDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int KernelConfigDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void KernelConfigDialog::kernelInstalledNotification(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void KernelConfigDialog::environmentChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
