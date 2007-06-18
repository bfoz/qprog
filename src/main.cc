/*	Filename:	main.cc
	Main file for QProg
	Interface for DIY PIC programmer hardware
	
	Created December 17, 2005 by Brandon Fosdick
	
	Copyright 2005 Brandon Fosdick (BSD License)

	$Id: main.cc,v 1.3 2007/06/18 00:00:00 bfoz Exp $
*/

#include<QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	//These need to be set before QProg is constructed
	QCoreApplication::setOrganizationName("bfoz");
	QCoreApplication::setOrganizationDomain("bfoz.net");
	QCoreApplication::setApplicationName("QProg");

	MainWindow mainwin;
	
	mainwin.show();
	return app.exec();
}
