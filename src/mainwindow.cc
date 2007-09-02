/*	Filename:	mainwindow.cc
	Subclass of QMainWindow
	Interface for DIY PIC programmer hardware
	
	Created January 6, 2006 by Brandon Fosdick
	
	Copyright 2005 Brandon Fosdick (BSD License)

	$Id: mainwindow.cc,v 1.10 2007/09/02 23:46:52 bfoz Exp $
*/

#include <iostream>
#include <QApplication>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>

#include "mainwindow.h"
#include "centralwidget.h"

MainWindow::MainWindow() : buffer(NULL)
{
//	CentralWidget	*prog = new CentralWidget();
	setCentralWidget(new CentralWidget());
	
	QAction	*updateInfoAct = new QAction(QString("Update"), this);
	updateInfoAct->setStatusTip(tr("Update the Device Info"));
	connect(updateInfoAct, SIGNAL(triggered()), this, SLOT(updateDeviceInfo()));

	QAction *aboutAction = new QAction("About", this);
	aboutAction->setStatusTip("About");
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(handleAbout()));	
	
#ifndef	Q_OS_DARWIN	//Let the File menu be created automatically on Darwin
//	QMenu	*fileMenu = menuBar()->addMenu(tr("&File"));
#endif

	QMenu *deviceInfoMenu = menuBar()->addMenu("Device Info");
	deviceInfoMenu->addAction("Update", this, SLOT(updateDeviceInfo()))->setStatusTip("Update the Device Info");
	deviceInfoMenu->addAction("Update From File", this, SLOT(updateDeviceInfoFromFile()))->setStatusTip("Update the Device Info from a file");

//	chipinfoMenu->addAction(updateInfoAct);
//	menuBar()->addAction("About", this, SLOT(handleAbout()));
#ifdef	Q_OS_DARWIN
	//On OSX the about menu needs to be nested so the automagic will work
	//	so hide it in the device info menu
	deviceInfoMenu->addAction(aboutAction);
#else
	menuBar()->addSeparator();
	menuBar()->addMenu("&Help")->addAction(aboutAction);
#endif

	progressDialog = new QProgressDialog(this);
	http = new QHttp(this);
	httpRequestAborted = false;
	connect(http, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));
	connect(http, SIGNAL(requestFinished(int, bool)), this, SLOT(httpRequestFinished(int, bool)));
	connect(http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)), this, SLOT(readResponseHeader(const QHttpResponseHeader &)));
	connect(progressDialog, SIGNAL(canceled()), this, SLOT(cancelDownload()));
}

void MainWindow::customEvent(QEvent* e)
{
	if( e->type() == (QEvent::Type)Startup )
		startup();
	else
		QMainWindow::customEvent(e);
}

void MainWindow::startup()
{
	//Check for the old PartsDB entries and delete them if they exist
	QSettings settings;
	if( settings.childGroups().contains("PartsDB") )
	{
		int ret = QMessageBox::question(this, "Delete obsolete database?", "An old, incompatable, version of the device info database has been detected. Would you like to delete it?", QMessageBox::Yes | QMessageBox::No);
		if( ret == QMessageBox::Yes )
		{
			settings.remove("PartsDB");
			QMessageBox::information(this, "Deleted", "The old database has been deleted");
		}
	}

	//See if the device info exists and warn the user if it doesn't
	if( settings.childGroups().contains("DeviceInfo") )
	{
		//Load the target type combobox
		static_cast<CentralWidget*>(centralWidget())->FillTargetCombo();
	}
	else
	{
		int ret = QMessageBox::question(this, "Download Device Info?", "You seem to be missing a Device Info database. Would you like to download one now?", QMessageBox::Yes | QMessageBox::No);
		if( ret == QMessageBox::Yes )
			updateDeviceInfo();
		else
			QMessageBox::information(this, "Okay...", "QProg won't work very well until you download a Device database. If you change your mind later you can select Update from the Device Info menu");
	}
}

void MainWindow::handleAbout()
{
	QMessageBox::about(this, "<center><b>QProg</b></center>", "<center>&copy; 2005-2007 Brandon Fosdick<br><a href=\"http://www.opensource.org/licenses/bsd-license.php\">BSD License</a><br><br><a href=\"http://bfoz.net/projects/qprog/\">http://bfoz.net/projects/qprog/</a></center>");
}

/*	Update the device info stored in the settings
		All device info is stored with a prefix of "DeviceInfo"
*/
void MainWindow::updateDeviceInfo()
{
	if(buffer != NULL)
	{
		delete buffer;
		buffer = NULL;
	}
	buffer = new QBuffer;
	
	progressDialog->setWindowTitle("Updating device info");
	progressDialog->setCancelButtonText("Cancel");
	progressDialog->setLabelText("Downloading");
	progressDialog->setMinimumDuration(0);
	progressDialog->setModal(true);

	http->setHost("bfoz.net");
	httpGetId = http->get("/projects/qprog/api/?command=export", buffer);
}

void MainWindow::cancelDownload()
{
	httpRequestAborted = true;
	http->abort();
	if(buffer != NULL)
	{
		delete buffer;
		buffer = NULL;
	}
}


void MainWindow::httpRequestFinished(int requestId, bool error)
{
	if (httpRequestAborted)
	{
		progressDialog->hide();
		return;
	}

	if (requestId != httpGetId)
		return;

	progressDialog->hide();

	if(error)
	{
		QMessageBox::information(this, tr("HTTP"), tr("Download failed"));
		return;
	}
	
	
//	QMessageBox::information(this, tr("Download successful"), tr("%1 bytes read").arg(buffer->size()));
//	printf("%s", buffer->data().data());

	// Store the device info at the system level so the user can make
	//  changes without corrupting the local copy of the database.
	QSettings	settings(QSettings::SystemScope, "bfoz.net", "QProg");
	// There's no need to beginGroup() here because the keys returned
	//  by the server already have the group name included
//	settings.beginGroup("DeviceInfo");
	QTextStream in(buffer->data());
	while(!in.atEnd())
	{
		QString line(in.readLine());
		if( line.contains('=') )
		{
			QStringList qsl = line.split('=');
			settings.setValue(qsl.at(0), qsl.at(1));
		}
	}
//	settings.endGroup();
	QMessageBox::information(this, tr("Download successful"), tr("%1 bytes read").arg(buffer->size()));

	if(buffer != NULL)
	{
		delete buffer;
		buffer = NULL;
	}

	static_cast<CentralWidget*>(centralWidget())->FillTargetCombo();	//Force the target type combobox to be reloaded
}

void MainWindow::readResponseHeader(const QHttpResponseHeader &responseHeader)
{
	if(responseHeader.statusCode() != 200)
	{
		QMessageBox::information(this, tr("HTTP"), tr("Download failed: %1.").arg(responseHeader.reasonPhrase()));
		httpRequestAborted = true;
		progressDialog->hide();
		http->abort();
		return;
	}
}

void MainWindow::updateDataReadProgress(int bytesRead, int totalBytes)
{
	if(httpRequestAborted)
		return;

	progressDialog->setMaximum(totalBytes);
	progressDialog->setValue(bytesRead);
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
