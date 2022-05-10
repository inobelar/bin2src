# QMake project file

TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

# ----------------------------------------------------------
# Add 'parg' library
INCLUDEPATH += $$PWD/third_party/parg/
HEADERS += $$PWD/third_party/parg/parg.h
SOURCES += $$PWD/third_party/parg/parg.c
# ----------------------------------------------------------

SOURCES += \
    $$PWD/sources/main.c
