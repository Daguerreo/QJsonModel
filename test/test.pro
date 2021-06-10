QT += testlib core
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
   ../qjsonmodel.cpp \
   tst_qjsonmodeltest.cpp

HEADERS += \
   ../qjsonmodel.h

INCLUDEPATH += \
   $$PWD/..

RESOURCES += \
   ../resources.qrc
