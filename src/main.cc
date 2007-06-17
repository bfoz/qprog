/*	Filename:	main.cc
	Main file for QtProg
	Interface for DIY PIC programmer hardware
	
	Created December 17, 2005 by Brandon Fosdick
	
	Copyright 2005 Brandon Fosdick (BSD License)

	$Id: main.cc,v 1.2 2007/06/17 05:03:20 bfoz Exp $
*/

#include<QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	//These need to be set before QtProg is constructed
	QCoreApplication::setOrganizationName("bfoz");
	QCoreApplication::setOrganizationDomain("bfoz.net");
	QCoreApplication::setApplicationName("QtProg");

	MainWindow mainwin;
	
	mainwin.show();
	return app.exec();
}
