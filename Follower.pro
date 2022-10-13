QT       += core gui
QT       += qml
QT       += winextras
QT       += concurrent
QT       += network
#QT       += multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    TableEditor/cmdeditor.cpp \
    TableEditor/inputmethodeditor.cpp \
    TableEditor/noteeditor.cpp \
    TableEditor/tablebase.cpp \
    TableEditor/tableeditor.cpp \
    Utils/WinUtility.cpp \
    Utils/cacheiconprovider.cpp \
    Utils/gaptimer.cpp \
    Utils/hook.cpp \
    Utils/inputmethod.cpp \
    Utils/keystate.cpp \
    Utils/logfilehandler.cpp \
    Utils/path.cpp \
    Utils/request.cpp \
    Utils/systemapi.cpp \
    Utils/timeclipboard.cpp \
    cmdlistwidget.cpp \
    codeeditor.cpp \
    executor.cpp \
    main.cpp \
    powersettingdia.cpp \
    shortcutdia.cpp \
    updateform.cpp \
    widget.cpp

HEADERS += \
    TableEditor/cmdeditor.h \
    TableEditor/inputmethodeditor.h \
    TableEditor/noteeditor.h \
    TableEditor/tablebase.h \
    TableEditor/tableeditor.h \
    Utils/PolicyConfig.h \
    Utils/WinUtility.h \
    Utils/cacheiconprovider.h \
    Utils/gaptimer.h \
    Utils/hook.h \
    Utils/inputmethod.h \
    Utils/keystate.h \
    Utils/logfilehandler.h \
    Utils/path.h \
    Utils/request.h \
    Utils/systemapi.h \
    Utils/timeclipboard.h \
    cmdlistwidget.h \
    codeeditor.h \
    executor.h \
    powersettingdia.h \
    shortcutdia.h \
    updateform.h \
    widget.h

FORMS += \
    powersettingdia.ui \
    shortcutdia.ui \
    tablebase.ui \
    updateform.ui \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    Function-List.txt \
    Log.txt \
    Need-to-Update.txt \
    icon.rc

RC_FILE += icon.rc

RESOURCES += \
    Res.qrc

LIBS += -lpsapi -luser32 -lWinmm -lole32


msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}
