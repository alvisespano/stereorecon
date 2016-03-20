# -------------------------------------------------
# Project created by QtCreator 2009-04-25T10:28:25
# -------------------------------------------------
TARGET = StereoReconstruction
TEMPLATE = app
SOURCES += main.cpp \
    demoapplication.cpp \
    multiviewstereo.cpp \
    klt/klt.c \
    klt/klt_util.c \
    klt/trackFeatures.c \
    klt/error.c \
    klt/convolve.c \
    klt/selectGoodFeatures.c \
    klt/pyramid.c
HEADERS += demoapplication.h \
    multiviewstereo.h \
    klt/klt.h \
    klt/klt_util.h \
    klt/error.h \
    klt/convolve.h \
    klt/base.h \
    klt/pyramid.h \
    stereomath.h
FORMS += demoapplication.ui
RESOURCES += internalResources.qrc
# DEFINES += QT_OPENGL_SUPPORT
QT += opengl
INCLUDEPATH += opencv-lib/include/opencv
