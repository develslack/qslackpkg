QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    kernelconfigdialog.cpp \
    usbcreatorwidget.cpp

HEADERS += \
    mainwindow.h \
    kernelconfigdialog.h \
    usbcreatorwidget.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# ============================================================================
# VERSIONADO AUTOMÁTICO: v-[numero_version]-compilación[fecha_compilacion]
# ============================================================================
BASE_VERSION = 1.0.0
GIT_TAG = $$system(git describe --tags --abbrev=0 2>/dev/null)
BUILD_DATE = $$system(date +%Y%m%d)
BUILD_TIME = $$system(date +%H:%M)

!isEmpty(GIT_TAG) {
    CLEAN_VER = $$replace(GIT_TAG, ^v-?, )
} else {
    CLEAN_VER = $$BASE_VERSION
}

FULL_APP_VERSION = "v-$${CLEAN_VER}-compilación$${BUILD_DATE}"
FULL_BUILD_DT = "$${BUILD_DATE} $${BUILD_TIME}"

VERSION_HEADER_CONTENT = \
    "$${LITERAL_HASH}ifndef VERSION_H" \
    "$${LITERAL_HASH}define VERSION_H" \
    "$${LITERAL_HASH}define APP_VERSION \"$$FULL_APP_VERSION\"" \
    "$${LITERAL_HASH}define APP_BUILD_DATETIME \"$$FULL_BUILD_DT\"" \
    "$${LITERAL_HASH}endif"

write_file($$OUT_PWD/version.h, VERSION_HEADER_CONTENT)
INCLUDEPATH += $$OUT_PWD
