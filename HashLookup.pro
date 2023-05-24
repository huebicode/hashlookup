QT       += core gui concurrent svg svgwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    customdelegate.cpp \
    customsortfilterproxymodel.cpp \
    customtableview.cpp \
    fileprocessor.cpp \
    headersortingadapter.cpp \
    itemprocessor.cpp \
    main.cpp \
    widget.cpp \
    yaraprocessor.cpp

HEADERS += \
    Column.h \
    customdelegate.h \
    customsortfilterproxymodel.h \
    customtableview.h \
    fileprocessor.h \
    headersortingadapter.h \
    itemprocessor.h \
    widget.h \
    yaraprocessor.h

FORMS += \
    widget.ui

INCLUDEPATH += $$PWD/include
INCLUDEPATH += $$PWD/include/yara
LIBS += -L$$PWD/libs/yara -llibyara

LIBS += -L$$PWD/libs/openssl -lcrypto
LIBS += -L$$PWD/libs/libmagic -lmagic
LIBS += -L$$PWD/libs/zlib -lz
LIBS += -L$$PWD/libs/liblzma -llzma

RESOURCES += \
    res.qrc

win32:RC_FILE = HashLookup.rc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


