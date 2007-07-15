######################################################################
# Automatically generated by qmake (2.00a) Sat Dec 17 23:34:02 2005
######################################################################

TEMPLATE = app
CONFIG	+= warn_on qt stl
macx {
	TARGET = QProg
	MOC_DIR = build
} else:win32 {
	TARGET = QProg
	CONFIG += static			#Static link Qt libs
	QMAKE_LFLAGS -= -mthreads	#remove dependency on mingwm10.dll (threaded exceptions)
	CONFIG -= exceptions		#disallow exceptions due to above
} else {
	TARGET = qprog
}

QT += network
#SUBDIRS = src

# Input
HEADERS += src/mainwindow.h src/centralwidget.h src/intelhex.h 
SOURCES += src/main.cc src/mainwindow.cc src/centralwidget.cc src/intelhex.cc

HEADERS	+= src/kitsrus.h
SOURCES	+= src/kitsrus.cc
HEADERS	+= src/chipinfo.h
SOURCES	+= src/chipinfo.cc

macx:LIBS += -framework IOKit

# qextserialport stuff
INCLUDEPATH += qextserialport
HEADERS	+= qextserialport/qextserialbase.h qextserialport/qextserialport.h
SOURCES	+= qextserialport/qextserialbase.cpp qextserialport/qextserialport.cpp

unix:HEADERS	+= qextserialport/posix_qextserialport.h
unix:SOURCES	+= qextserialport/posix_qextserialport.cpp
unix:DEFINES	+= _TTY_POSIX_

win32:HEADERS	+= qextserialport/win_qextserialport.h
win32:SOURCES	+= qextserialport/win_qextserialport.cpp
win32:DEFINES	+= _TTY_WIN_
