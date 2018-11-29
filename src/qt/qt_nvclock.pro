
TEMPLATE = app
CONFIG  += qt warning debug

INCLUDEPATH += . ../backend
LIBS += ../backend/libbackend.a

HEADERS += qt_nvclock.h qt_xfree.h
SOURCES += main.cpp qt_nvclock.cpp qt_xfree.cpp
