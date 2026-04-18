QT += testlib gui widgets

CONFIG += qt warn_on testcase c++17 console
CONFIG -= app_bundle

TEMPLATE = app
TARGET = tst_graph

INCLUDEPATH += ..

SOURCES += \
    tst_graph.cpp \
    ../Graph.cpp

HEADERS += \
    ../Graph.h
