/****************************************************************************
** Meta object code from reading C++ file 'systemhealthwidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "systemhealthwidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'systemhealthwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SystemHealthWidget_t {
    QByteArrayData data[12];
    char stringdata0[195];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SystemHealthWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SystemHealthWidget_t qt_meta_stringdata_SystemHealthWidget = {
    {
QT_MOC_LITERAL(0, 0, 18), // "SystemHealthWidget"
QT_MOC_LITERAL(1, 19, 13), // "systemCleaned"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 18), // "runBrokenLibsAudit"
QT_MOC_LITERAL(4, 53, 27), // "onBrokenLibsProcessFinished"
QT_MOC_LITERAL(5, 81, 8), // "exitCode"
QT_MOC_LITERAL(6, 90, 20), // "QProcess::ExitStatus"
QT_MOC_LITERAL(7, 111, 10), // "exitStatus"
QT_MOC_LITERAL(8, 122, 13), // "scanDiskUsage"
QT_MOC_LITERAL(9, 136, 14), // "runSafeCleanup"
QT_MOC_LITERAL(10, 151, 21), // "readRootProcessOutput"
QT_MOC_LITERAL(11, 173, 21) // "onRootProcessFinished"

    },
    "SystemHealthWidget\0systemCleaned\0\0"
    "runBrokenLibsAudit\0onBrokenLibsProcessFinished\0"
    "exitCode\0QProcess::ExitStatus\0exitStatus\0"
    "scanDiskUsage\0runSafeCleanup\0"
    "readRootProcessOutput\0onRootProcessFinished"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SystemHealthWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   49,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    0,   50,    2, 0x08 /* Private */,
       4,    2,   51,    2, 0x08 /* Private */,
       8,    0,   56,    2, 0x08 /* Private */,
       9,    0,   57,    2, 0x08 /* Private */,
      10,    0,   58,    2, 0x08 /* Private */,
      11,    2,   59,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 6,    5,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 6,    5,    7,

       0        // eod
};

void SystemHealthWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SystemHealthWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->systemCleaned(); break;
        case 1: _t->runBrokenLibsAudit(); break;
        case 2: _t->onBrokenLibsProcessFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QProcess::ExitStatus(*)>(_a[2]))); break;
        case 3: _t->scanDiskUsage(); break;
        case 4: _t->runSafeCleanup(); break;
        case 5: _t->readRootProcessOutput(); break;
        case 6: _t->onRootProcessFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QProcess::ExitStatus(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SystemHealthWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SystemHealthWidget::systemCleaned)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SystemHealthWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_SystemHealthWidget.data,
    qt_meta_data_SystemHealthWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SystemHealthWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SystemHealthWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SystemHealthWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SystemHealthWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void SystemHealthWidget::systemCleaned()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
