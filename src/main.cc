/*  Main file for QProg
    Interface for DIY PIC programmer hardware

    Created December 17, 2005 by Brandon Fosdick

    Copyright 2005 Brandon Fosdick (BSD License)
*/

#include<QApplication>

#include "delegate.h"
#include "mainwindow.h"

Delegate delegate;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // These need to be set before QProg is constructed
    QCoreApplication::setOrganizationName("bfoz.net");
    QCoreApplication::setOrganizationDomain("bfoz.net");
    QCoreApplication::setApplicationName("QProg");

    // Post a startup event to the application delegate
    app.postEvent(&delegate, new QEvent((QEvent::Type)Delegate::Startup), Qt::LowEventPriority);

    return app.exec();
}
