INCLUDEPATH += $$PWD
HEADERS += $$PWD/core/types.h $$PWD/algorithm/interfaces.h $$PWD/storage/storage.h
SOURCES += $$PWD/core/types.cpp
SOURCES += $$PWD/algorithm/opencv.cpp
HEADERS += $$PWD/service/session.h
HEADERS += $$PWD/ui/dialog.h
SOURCES += $$PWD/ui/dialog.cpp $$PWD/service/worker.cpp
SOURCES += $$PWD/storage/platform.cpp
win32:LIBS += -lcrypt32
macx:LIBS += -framework Security -framework CoreFoundation
unix:!macx:packagesExist(libsecret-1) {
    CONFIG += link_pkgconfig
    PKGCONFIG += libsecret-1
    DEFINES += FACE_WITH_SECRET
}
SOURCES += $$PWD/service/session.cpp $$PWD/storage/serialization.cpp
contains(CONFIG, face_opencv) {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4
    DEFINES += FACE_WITH_OPENCV
}
