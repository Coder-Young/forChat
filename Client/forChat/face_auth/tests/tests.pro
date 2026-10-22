QT += core gui testlib
CONFIG += console testcase c++11
CONFIG -= app_bundle
TEMPLATE = app
TARGET = face_tests
INCLUDEPATH += ..
SOURCES += tests.cpp ../core/types.cpp ../algorithm/opencv.cpp
