# -------------------------------------------------
# Project created by QtCreator 2009-10-29T19:08:01
# -------------------------------------------------
QT -= gui
TARGET = letsurfagain
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
INCLUDEPATH += "C:\Programmi\OpenCV\cxcore\include" \
    "C:\Programmi\OpenCV\cv\include" \
    "C:\Programmi\OpenCV\cvaux\include" \
    "C:\Programmi\OpenCV\otherlibs\highgui"
win32:LIBS += -L"C:\Programmi\OpenCV\lib"
LIBS += -lcv \
    -lhighgui \
    -lcxcore
SOURCES += main.cpp \
    surf.cpp \
    fasthessian.cpp \
    utils.cpp \
    ipoint.cpp \
    integral.cpp
HEADERS += surf.h \
    ipoint.h \
    surflib.h \
    fasthessian.h \
    utils.h \
    surflib.h \
    surf.h
QMAKE_CXXFLAGS += -Wno-missing-braces -Wno-uninitialized
QMAKE_CXXFLAGS_RELEASE += -fomit-frame-pointer -funroll-loops -fvariable-expansion-in-unroller
