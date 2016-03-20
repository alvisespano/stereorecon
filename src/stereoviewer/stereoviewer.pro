# -------------------------------------------------
# Project created by QtCreator 2009-07-02T17:30:48
# -------------------------------------------------
QT += opengl
TARGET = stereoviewer
TEMPLATE = app
SOURCES += main.cpp \
    config.cpp \
    mainwindow.cpp \
    stereoview.cpp \
    globals.cpp \
    stereographicsview.cpp \
    glscene.cpp \
    cvlab/string_utilities.cpp \
    cvlab/cvlab/stl_file.cpp \
    cvlab/cvlab/mesh.cpp \
    cvlab/cvlab/math3d.cpp \
    cvlab/cvlab/cloud3d.cpp \
    prelude/prelude_config.cpp \
    prelude/threads.cpp \
    prelude/misc.cpp \
    prelude/maths.cpp \
    prelude/log.cpp \
    prelude/io.cpp \
    exif/paths.c \
    exif/myglob.c \
    exif/makernote.c \
    exif/jpgfile.c \
    exif/jhead.c \
    exif/iptc.c \
    exif/gpsinfo.c \
    exif/exif.c
HEADERS += mainwindow.h \
    stereoview.h \
    config.h \
    globals.h \
    stereographicsview.h \
    glscene.h \
    prelude.h \
    exif.h \
    cvlab/string_utilities.h \
    cvlab/cvlab/stl_file.h \
    cvlab/cvlab/mesh.h \
    cvlab/cvlab/math3d.h \
    cvlab/cvlab/kdtree.h \
    cvlab/cvlab/exceptions.h \
    cvlab/cvlab/cloud3d.h \
    prelude/threads.h \
    prelude/sync.h \
    prelude/maths.h \
    prelude/log.h \
    prelude/io.h \
    prelude/misc.h \
    prelude/traits.h \
    prelude/prelude_config.h \
    prelude/stdafx.h \
    exif/jhead.h
FORMS += mainwindow.ui
INCLUDEPATH += C:\Programmi\OpenCV\cv\include \
    C:\Programmi\OpenCV\cxcore\include \
    .\cvlab
LIBS += -L"C:\Programmi\OpenCV\lib" \
    -L"C:\Programmi\OpenCV\bin" \
    -lcv \
    -lcxcore
DEFINES += QT_OPENGL_LIB
QMAKE_CFLAGS_RELEASE += -fomit-frame-pointer -funroll-loops -fvariable-expansion-in-unroller
QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS_RELEASE += -fomit-frame-pointer -funroll-loops -fvariable-expansion-in-unroller
OTHER_FILES += ../bundler/default_options.txt
