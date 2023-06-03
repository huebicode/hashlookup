QT       += core gui concurrent svg svgwidgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

CONFIG -= embed_manifest_exe
win32:RC_FILE = HashLookup.rc # embedding own manifest file

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

SOURCES += \
    customdelegate.cpp \
    customsortfilterproxymodel.cpp \
    customtableview.cpp \
    fileprocessor.cpp \
    headersortingadapter.cpp \
    itemprocessor.cpp \
    main.cpp \
    widget.cpp \
    yaraprocessor.cpp \
    zipper.cpp

HEADERS += \
    Column.h \
    customdelegate.h \
    customsortfilterproxymodel.h \
    customtableview.h \
    fileprocessor.h \
    headersortingadapter.h \
    itemprocessor.h \
    widget.h \
    yaraprocessor.h \
    zipper.h

FORMS += \
    widget.ui

INCLUDEPATH += $$PWD/include
INCLUDEPATH += $$PWD/include/yara
INCLUDEPATH += $$PWD/include/zlib

#CONFIG (debug) {
#    LIBS += -L$$PWD/lib/debug -lmagic -llibyara -lAdvapi32 -llibcrypto -lquazip1-qt6d
#}

CONFIG (release) {
    LIBS += -L$$PWD/lib/release -lmagic -llibyara -lAdvapi32 -llibcrypto -lquazip1-qt6
}

RESOURCES += \
    res.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target



