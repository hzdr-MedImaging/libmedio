# vim:set ts=2:set et:nowrap
#
# buildAtten 
# 

TEMPLATE = app
TARGET = ./sumSino
CONFIG += release
QT = gui core xml

unix {
	OBJECTS_DIR	= .obj
	MOC_DIR	= .moc
	UI_DIR = .ui
}

win32 {
	OBJECTS_DIR	= _obj
	MOC_DIR	= _moc
	UI_DIR = _ui
}

DEPENDPATH += /usr/local/petlib/include/medio /usr/local/petlib/include/mtrack /usr/local/petlib/include/rtdebug . 
INCLUDEPATH += /usr/local/petlib/include/medio /usr/local/petlib/include/mtrack /usr/local/petlib/include/rtdebug . 

LIBS += /usr/local/petlib/lib/libmedio.so.2 -L/usr/local/petlib/lib -lmtrack

# Input
HEADERS += CIOTransform.h \
           CMultiDimArray.h
SOURCES += CIOTransform.cpp \
           main.cpp
