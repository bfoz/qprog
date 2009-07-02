/*  QProg application delegate

    Created	March 15, 2009	Brandon Fosdick <bfoz@bfoz.net>

    Copyright 2005 Brandon Fosdick (BSD License)
*/

#include <QHttp>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSettings>
#include <QTextStream>

#include "centralwidget.h"
#include "delegate.h"
#include "mainwindow.h"

#define	POST_STARTUP_DID_FINISH	qApp->postEvent(this, new QEvent((QEvent::Type)Delegate::StartupDidFinish), Qt::LowEventPriority)
#define	POST_STARTUP_DID_FAIL	qApp->postEvent(this, new QEvent((QEvent::Type)Delegate::StartupDidFail), Qt::LowEventPriority)

// Handle late initialization
void Delegate::startup()
{
    //Check for the old PartsDB entries and delete them if they exist
    QSettings settings;
    if( settings.childGroups().contains("PartsDB") )
    {
	int ret = QMessageBox::question(0, "Delete obsolete database?", "An old, incompatable, version of the device info database has been detected. Would you like to delete it?", QMessageBox::Yes | QMessageBox::No);
	if( ret == QMessageBox::Yes )
	{
	    settings.remove("PartsDB");
	    QMessageBox::information(0, "Deleted", "The old database has been deleted");
	}
    }

    //See if the device info exists and warn the user if it doesn't
    if( settings.childGroups().contains("DeviceInfo") )
	POST_STARTUP_DID_FINISH;
    else
    {
	QMessageBox msgBox;
	msgBox.setText("Device Info is Missing");
	msgBox.setInformativeText("Download now?");
	msgBox.setIcon(QMessageBox::Question);
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);

	switch( msgBox.exec() )
	{
	    case QMessageBox::Yes:
		getDeviceInfo();
		break;
	    case QMessageBox::No:
	    {
		msgBox.setText("QProg won't work properly without Device Info");
		msgBox.setInformativeText("If you change your mind later you can select Update from the Device Info menu");
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.exec();
		POST_STARTUP_DID_FINISH;
		break;
	    }
	}
    }
}

void Delegate::customEvent(QEvent* e)
{
    switch( e->type() )
    {
	case (QEvent::Type)Startup:
	    startup();
	    break;
	case (QEvent::Type)StartupDidFinish:
	{
	    (new MainWindow())->show();
	    break;
	}
	case (QEvent::Type)StartupDidFail:
	    qApp->quit();
	    break;
	default:
	    QObject::customEvent(e);
    }
}

//  Get the device info database and store it
//   All device info is stored with a prefix of "DeviceInfo"
void Delegate::getDeviceInfo()
{
    if( !buffer )
	buffer = new QBuffer;

    progressDialog = new QProgressDialog();
    progressDialog->setWindowTitle("Updating device info");
    progressDialog->setCancelButtonText("Cancel");
    progressDialog->setMinimumDuration(0);
    progressDialog->setModal(true);
    progressDialog->show();		//Force the dialog to open immediately
    connect(progressDialog, SIGNAL(canceled()), this, SLOT(httpCancel()));

    if( !http )
    {
	http = new QHttp("bfoz.net");
	httpRequestAborted = false;
	connect(http, SIGNAL(dataReadProgress(int, int)), this, SLOT(updateProgress(int, int)));
	connect(http, SIGNAL(requestFinished(int, bool)), this, SLOT(httpRequestFinished(int, bool)));
	connect(http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)), this, SLOT(httpResponseHeader(const QHttpResponseHeader &)));
	connect(http, SIGNAL(stateChanged(int)), this, SLOT(httpStateChanged(int)));
    }

    // Start the request
    httpGetId = http->get("/projects/qprog/api/?command=export", buffer);
}

void Delegate::httpCancel()
{
    httpRequestAborted = true;
    http->abort();
    httpCleanup();
}

void Delegate::httpCleanup()
{
    progressDialog->hide();
    if( buffer )
    {
	delete buffer;
	buffer = NULL;
    }
}

void Delegate::httpResponseHeader(const QHttpResponseHeader &responseHeader)
{
    if(responseHeader.statusCode() != 200)
    {
	QMessageBox msgBox;
	msgBox.setText("Download Failed");
	msgBox.setInformativeText(tr("%1").arg(responseHeader.reasonPhrase()));
	msgBox.setIcon(QMessageBox::Critical);
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setDefaultButton(QMessageBox::Ok);

	httpCancel();
	POST_STARTUP_DID_FAIL;
    }
}

void Delegate::httpRequestFinished(int requestId, bool error)
{
    if( httpRequestAborted || (requestId != httpGetId) )
    {
	httpCleanup();
	POST_STARTUP_DID_FAIL;
	return;
    }

    if(error)
    {
	QMessageBox::information(NULL, tr("HTTP"), tr("Download failed"));
	httpCleanup();
	POST_STARTUP_DID_FAIL;
	return;
    }

    // Store the server copy of the device info at the user,organization level
    //  so the user can make changes without corrupting the local copy
    QSettings	settings(QCoreApplication::organizationName());

    // Warn if the settings file isn't writable
    if( !settings.isWritable() )
	qWarning("Read Only Settings! You need write permission for %s", qPrintable(settings.fileName()));

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

    httpCleanup();

    QMessageBox msgBox;
    msgBox.setText("Download Successful");
    msgBox.setInformativeText(tr("Retrieved %1 bytes").arg(buffer->size()));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();

    POST_STARTUP_DID_FINISH;
}

// Change the progress dialog text according to the state of the connection
void Delegate::httpStateChanged(int state)
{
    switch(state)
    {
	case QHttp::HostLookup:
	    progressDialog->setLabelText("Looking up bfoz.net");
	    break;
	case QHttp::Connecting:
	    progressDialog->setLabelText("Connecting");
	    break;
	case QHttp::Reading:
	    progressDialog->setLabelText("Downloading");
	    break;
	case QHttp::Closing:
	    progressDialog->setLabelText("Disconnecting");
	    break;
    }
}


void Delegate::updateProgress(int bytesRead, int totalBytes)
{
    if(httpRequestAborted)
	return;

    progressDialog->setMaximum(totalBytes);
    progressDialog->setValue(bytesRead);
}
