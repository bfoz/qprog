/*	Filename:	mainwindow.h
	Subclass of QMainWindow
	Interface for DIY PIC programmer hardware
	
	Created January 6, 2006 by Brandon Fosdick
	
	Copyright 2006 Brandon Fosdick (BSD License)

	$Id: mainwindow.h,v 1.4 2007/09/02 22:56:54 bfoz Exp $
*/

#ifndef	MAINWINDOW_H
#define	MAINWINDOW_H

#include <QBuffer>
#include <QHttp>
#include <QMainWindow>
#include <QProgressDialog>

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow();

private slots:
	void handleAbout();
	void updateDeviceInfo();
	void cancelDownload();
	void httpRequestFinished(int, bool);
	void readResponseHeader(const QHttpResponseHeader &);
	void updateDataReadProgress(int, int);
	void updateDeviceInfoFromFile();

private:
	QBuffer	*buffer;
	QHttp	*http;
	QProgressDialog *progressDialog;
	bool httpRequestAborted;
	int httpGetId;
};

#endif	//MAINWINDOW_H
