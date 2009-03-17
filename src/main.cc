/*	Filename:	main.cc
	Main file for QProg
	Interface for DIY PIC programmer hardware
	
	Created December 17, 2005 by Brandon Fosdick
	
	Copyright 2005 Brandon Fosdick (BSD License)

	$Id: main.cc,v 1.8 2009/03/17 06:01:13 bfoz Exp $
*/

#include<QApplication>

#ifdef	Q_OS_DARWIN
#include "carbon_cocoa.h"
#endif	//Q_OS_DARWIN

#include "delegate.h"
#include "mainwindow.h"

Delegate delegate;

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

#ifdef	Q_OS_DARWIN
	Cocoa::initialize();
#endif	//Q_OS_DARWIN

	//These need to be set before QProg is constructed
    QCoreApplication::setOrganizationName("bfoz.net");
	QCoreApplication::setOrganizationDomain("bfoz.net");
	QCoreApplication::setApplicationName("QProg");

    // Post a startup event to the application delegate
    app.postEvent(&delegate, new QEvent((QEvent::Type)Delegate::Startup), Qt::LowEventPriority);

	return app.exec();
}
