QT       += core gui widgets serialport

TARGET = ExitDTM
TEMPLATE = app

SOURCES += main.cpp\
    DtmMainWindow.cpp

HEADERS  += DtmMainWindow.h

FORMS    += DtmMainWindow.ui

RESOURCES +=

#Windows application version information
win32:RC_FILE = version.rc

#Windows application icon
win32:RC_ICONS = images/ExitDTM32.ico

#Mac application icon
ICON = MacExitDTMIcon.icns

DISTFILES +=
