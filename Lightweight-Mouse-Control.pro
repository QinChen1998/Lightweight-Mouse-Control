QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# QtAwesome icon library integration
CONFIG += fontAwesomeFree
include(QtAwesome/QtAwesome/QtAwesome.pri)

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Include paths
INCLUDEPATH += include

# Source files
SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/mousepoint.cpp \
    src/mouserecorder.cpp \
    src/mouseplayer.cpp \
    src/pathmanager.cpp \
    src/hotkeymanager.cpp \
    src/settingsdialog.cpp \
    src/compactwindow.cpp \
    src/customspinbox.cpp

# Header files
HEADERS += \
    include/mainwindow.h \
    include/mousepoint.h \
    include/mouserecorder.h \
    include/mouseplayer.h \
    include/pathmanager.h \
    include/hotkeymanager.h \
    include/settingsdialog.h \
    include/compactwindow.h \
    include/customspinbox.h

# UI files
FORMS += \
    ui/mainwindow.ui \
    ui/compactwindow.ui

# Windows-specific settings for global hotkeys and mouse control
win32 {
    LIBS += -luser32 -lgdi32
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
