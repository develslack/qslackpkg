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
    QByteArrayData data[24];
    char stringdata0[412];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_KernelConfigDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_KernelConfigDialog_t qt_meta_stringdata_KernelConfigDialog = {
    {
QT_MOC_LITERAL(0, 0, 18), // "KernelConfigDialog"
QT_MOC_LITERAL(1, 19, 26), // "requestPrivilegedExecution"
QT_MOC_LITERAL(2, 46, 0), // ""
QT_MOC_LITERAL(3, 47, 10), // "binaryPath"
QT_MOC_LITERAL(4, 58, 4), // "args"
QT_MOC_LITERAL(5, 63, 9), // "statusMsg"
QT_MOC_LITERAL(6, 73, 22), // "fetchKernelOrgReleases"
QT_MOC_LITERAL(7, 96, 29), // "onKernelOrgReleasesDownloaded"
QT_MOC_LITERAL(8, 126, 14), // "QNetworkReply*"
QT_MOC_LITERAL(9, 141, 5), // "reply"
QT_MOC_LITERAL(10, 147, 24), // "onSourceSelectionChanged"
QT_MOC_LITERAL(11, 172, 32), // "downloadAndExtractSelectedKernel"
QT_MOC_LITERAL(12, 205, 26), // "inheritCurrentKernelConfig"
QT_MOC_LITERAL(13, 232, 17), // "runLocalModConfig"
QT_MOC_LITERAL(14, 250, 15), // "loadKconfigTree"
QT_MOC_LITERAL(15, 266, 18), // "onTreeItemSelected"
QT_MOC_LITERAL(16, 285, 16), // "QTreeWidgetItem*"
QT_MOC_LITERAL(17, 302, 7), // "current"
QT_MOC_LITERAL(18, 310, 8), // "previous"
QT_MOC_LITERAL(19, 319, 20), // "onOptionStateChanged"
QT_MOC_LITERAL(20, 340, 17), // "filterKconfigTree"
QT_MOC_LITERAL(21, 358, 5), // "query"
QT_MOC_LITERAL(22, 364, 17), // "saveDotConfigOnly"
QT_MOC_LITERAL(23, 382, 29) // "compileAndInstallCustomKernel"

    },
    "KernelConfigDialog\0requestPrivilegedExecution\0"
    "\0binaryPath\0args\0statusMsg\0"
    "fetchKernelOrgReleases\0"
    "onKernelOrgReleasesDownloaded\0"
    "QNetworkReply*\0reply\0onSourceSelectionChanged\0"
    "downloadAndExtractSelectedKernel\0"
    "inheritCurrentKernelConfig\0runLocalModConfig\0"
    "loadKconfigTree\0onTreeItemSelected\0"
    "QTreeWidgetItem*\0current\0previous\0"
    "onOptionStateChanged\0filterKconfigTree\0"
    "query\0saveDotConfigOnly\0"
    "compileAndInstallCustomKernel"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_KernelConfigDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    3,   74,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,   81,    2, 0x08 /* Private */,
       7,    1,   82,    2, 0x08 /* Private */,
      10,    0,   85,    2, 0x08 /* Private */,
      11,    0,   86,    2, 0x08 /* Private */,
      12,    1,   87,    2, 0x08 /* Private */,
      14,    0,   90,    2, 0x08 /* Private */,
      15,    2,   91,    2, 0x08 /* Private */,
      19,    0,   96,    2, 0x08 /* Private */,
      20,    1,   97,    2, 0x08 /* Private */,
      22,    0,  100,    2, 0x08 /* Private */,
      23,    0,  101,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QStringList, QMetaType::QString,    3,    4,    5,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   13,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 16, 0x80000000 | 16,   17,   18,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   21,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void KernelConfigDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<KernelConfigDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->requestPrivilegedExecution((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QStringList(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 1: _t->fetchKernelOrgReleases(); break;
        case 2: _t->onKernelOrgReleasesDownloaded((*reinterpret_cast< QNetworkReply*(*)>(_a[1]))); break;
        case 3: _t->onSourceSelectionChanged(); break;
        case 4: _t->downloadAndExtractSelectedKernel(); break;
        case 5: _t->inheritCurrentKernelConfig((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->loadKconfigTree(); break;
        case 7: _t->onTreeItemSelected((*reinterpret_cast< QTreeWidgetItem*(*)>(_a[1])),(*reinterpret_cast< QTreeWidgetItem*(*)>(_a[2]))); break;
        case 8: _t->onOptionStateChanged(); break;
        case 9: _t->filterKconfigTree((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: _t->saveDotConfigOnly(); break;
        case 11: _t->compileAndInstallCustomKernel(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 2:
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
            using _t = void (KernelConfigDialog::*)(const QString & , const QStringList & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&KernelConfigDialog::requestPrivilegedExecution)) {
                *result = 0;
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
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void KernelConfigDialog::requestPrivilegedExecution(const QString & _t1, const QStringList & _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
