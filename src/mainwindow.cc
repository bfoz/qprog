/*  Subclass of QMainWindow
    Interface for DIY PIC programmer hardware

    Created January 6, 2006 by Brandon Fosdick

    Copyright 2005 Brandon Fosdick (BSD License)
*/

#include <iostream>
#include <QApplication>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>

#include "delegate.h"
#include "mainwindow.h"
#include "centralwidget.h"

MainWindow::MainWindow() : buffer(NULL)
{
    CentralWidget* central = new CentralWidget();
    central->FillTargetCombo();
    setCentralWidget(central);

    QAction	*updateInfoAct = new QAction(QString("Update"), this);
    updateInfoAct->setStatusTip(tr("Update the Device Info"));
    connect(updateInfoAct, SIGNAL(triggered()), &delegate, SLOT(getDeviceInfo()));

    QAction *aboutAction = new QAction("About", this);
    aboutAction->setStatusTip("About");
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(handleAbout()));

#ifndef	Q_OS_DARWIN	//Let the File menu be created automatically on Darwin
//  QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
#endif

    QMenu *deviceInfoMenu = menuBar()->addMenu("Device Info");
    deviceInfoMenu->addAction("Update", this, SLOT(updateDeviceInfo()))->setStatusTip("Update the Device Info");
    deviceInfoMenu->addAction("Update From File", this, SLOT(updateDeviceInfoFromFile()))->setStatusTip("Update the Device Info from a file");

//  chipinfoMenu->addAction(updateInfoAct);
//  menuBar()->addAction("About", this, SLOT(handleAbout()));
#ifdef	Q_OS_DARWIN
    // On OSX the about menu needs to be nested so the automagic will work
    //  so hide it in the device info menu
    deviceInfoMenu->addAction(aboutAction);
#else
    menuBar()->addSeparator();
    menuBar()->addMenu("&Help")->addAction(aboutAction);
#endif
}

void MainWindow::customEvent(QEvent* e)
{
    QMainWindow::customEvent(e);
}

void MainWindow::handleAbout()
{
    QMessageBox::about(this, tr("QProg %1").arg(QPROG_VERSION),
	tr("<p align=center><b>QProg %1</b></p><p align=center>&copy; 2005-2008 Brandon Fosdick<br><a href=\"http://www.opensource.org/licenses/bsd-license.php\">BSD License</a></p><p align=center><a href=\"http://bfoz.net/projects/qprog/\">http://bfoz.net/projects/qprog/</a></p>").arg(QPROG_VERSION));
}

void MainWindow::updateDeviceInfoFromFile()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath());
    if( filename.size() != 0 )
    {
	QFile	file(filename);
	if( !file.open(QIODevice::ReadOnly) )
	{
	    QMessageBox::critical(0, "Error", "Could not open device info file");
	}
	else
	{
	    // Store the device info at the system level so the user can make
	    //  changes without corrupting the local copy of the database.
	    QSettings	settings(QSettings::SystemScope, "bfoz.net", "QProg");
	    QTextStream in(&file);
	    while(!in.atEnd())
	    {
		QString line(in.readLine());
		if( line.contains('=') )
		{
		    QStringList qsl = line.split('=');
		    settings.setValue(qsl.at(0), qsl.at(1));
		}
	    }
	    static_cast<CentralWidget*>(centralWidget())->FillTargetCombo();	//Force the target type combobox to be reloaded

	    QMessageBox::information(this, tr("Update From File"), tr("Update"));
	}
    }
}
