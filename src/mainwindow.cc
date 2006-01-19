/*	Filename:	mainwindow.cc
	Subclass of QMainWindow
	Interface for DIY PIC programmer hardware
	
	Created January 6, 2006 by Brandon Fosdick
	
	Copyright 2005 Brandon Fosdick (BSD License)
*/

#include <iostream>
#include <QApplication>
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
	updateInfoAct->setStatusTip(tr("Update the Chip Info"));
	connect(updateInfoAct, SIGNAL(triggered()), this, SLOT(updateChipInfo()));

#ifndef	Q_OS_DARWIN	//Let the File menu be created automatically on Darwin
//	QMenu	*fileMenu = menuBar()->addMenu(tr("&File"));
#endif

	QMenu *chipinfoMenu = menuBar()->addMenu("Chip Info");
	chipinfoMenu->addAction(updateInfoAct);
	
	progressDialog = new QProgressDialog(this);
	http = new QHttp(this);
	httpRequestAborted = false;
	connect(http, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateDataReadProgress(int, int)));
	connect(http, SIGNAL(requestFinished(int, bool)), this, SLOT(httpRequestFinished(int, bool)));
	connect(http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)), this, SLOT(readResponseHeader(const QHttpResponseHeader &)));
	connect(progressDialog, SIGNAL(canceled()), this, SLOT(cancelDownload()));

	//See if the chip info exists and warn the user if it doesn't
/*	QSettings settings;
	if( !settings.childGroups().contains("chipinfo") )
		QMessageBox::warning(this, "No Chip Info", "QtProg requires a set of chip information records to continue.\n\nWould you like to download them now?", tr("&Yes"), tr("&No"), 0, 0, 1);
*/
}

/*	Update the chip info stored in the settings
		All chip info is stored with a prefix of "PartsDB"
*/
void MainWindow::updateChipInfo()
{
	if(buffer != NULL)
	{
		delete buffer;
		buffer = NULL;
	}
	buffer = new QBuffer;
	
	http->setHost("bfoz.net");
	httpGetId = http->get("/projects/pocket/PartsDB/export.php?format=extattr&noprefix=1", buffer);

	progressDialog->setWindowTitle("Updating chip info");
	progressDialog->setCancelButtonText("Cancel");
	progressDialog->setLabelText("Downloading");
	progressDialog->setModal(true);
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
	
	QSettings	settings;
	settings.beginGroup("PartsDB");
	QTextStream in(buffer->data());
	while(!in.atEnd())
	{
		QString key(in.readLine());
		QString value(in.readLine());
//		printf("%s\t=> %s\n", key.toStdString().c_str(), value.toStdString().c_str());
		settings.setValue(key, value);
	}
	settings.endGroup();
	QMessageBox::information(this, tr("Download successful"), tr("%1 bytes read").arg(buffer->size()));

/*	QStringList keys = settings.allKeys();
	QStringListIterator i(keys);
	while(i.hasNext())
		std::cout << i.next().toStdString() << std::endl;
*/
	static_cast<CentralWidget*>(centralWidget())->FillTargetCombo();	//Force the target type combobox to be reloaded

	if(buffer != NULL)
	{
		delete buffer;
		buffer = NULL;
	}
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
