/*	Filename:	main.cc
	Main file for QProg
	Interface for DIY PIC programmer hardware
	
	Created December 17, 2005 by Brandon Fosdick
	
	Copyright 2005 Brandon Fosdick (BSD License)

	$Id: main.cc,v 1.7 2008/03/16 18:47:16 bfoz Exp $
*/

#include<QApplication>

#ifdef	Q_OS_DARWIN
#include "carbon_cocoa.h"
#endif	//Q_OS_DARWIN

#include "mainwindow.h"

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

	MainWindow mainwin;

	// Post a startup event to the main window
	app.postEvent(&mainwin, new QEvent((QEvent::Type)MainWindow::Startup), Qt::LowEventPriority);

	mainwin.show();
	return app.exec();
}
