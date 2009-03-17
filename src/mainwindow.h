/*	Filename:	mainwindow.h
	Subclass of QMainWindow
	Interface for DIY PIC programmer hardware
	
	Created January 6, 2006 by Brandon Fosdick
	
	Copyright 2006 Brandon Fosdick (BSD License)

	$Id: mainwindow.h,v 1.7 2009/03/17 06:01:13 bfoz Exp $
*/

#ifndef	MAINWINDOW_H
#define	MAINWINDOW_H

#include <QBuffer>
#include <QEvent>
#include <QHttp>
#include <QMainWindow>
#include <QProgressDialog>

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	enum { Startup = QEvent::User }; 

	MainWindow();

protected:
	virtual void customEvent(QEvent*);

private slots:
	void handleAbout();
	void updateDeviceInfoFromFile();

private:
	QBuffer	*buffer;
	QHttp	*http;
	QProgressDialog *progressDialog;
	bool httpRequestAborted;
	int httpGetId;

	void startup();
};

#endif	//MAINWINDOW_H
