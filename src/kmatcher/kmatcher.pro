# -------------------------------------------------
# Project created by QtCreator 2009-10-27T15:53:28
# -------------------------------------------------
QT -= gui
TARGET = kmatcher
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += main.cpp \
    iidyn.cpp
HEADERS += iidyn.h
QMAKE_CXXFLAGS_RELEASE += -fomit-frame-pointer -funroll-loops -fvariable-expansion-in-unroller

