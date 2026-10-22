QT += core gui widgets
CONFIG += c++11 console
TEMPLATE = app
TARGET = face_harness
include(../face_auth.pri)
SOURCES += harness.cpp
