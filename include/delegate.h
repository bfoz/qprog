/*  Filename: delegate.h
    QProg application delegate

    Created	March 15, 2009	Brandon Fosdick <bfoz@bfoz.net>

    Copyright 2005 Brandon Fosdick (BSD License)
*/

#ifndef	DELEGATE_H
#define	DELEGATE_H

#include <QApplication>		// For QEvent

class QBuffer;
class QHttp;
class QHttpResponseHeader;
class QProgressDialog;

class Delegate : public QObject
{
    Q_OBJECT

protected:
    virtual void customEvent(QEvent*);
    
public:
    enum { Startup = QEvent::User, StartupDidFinish, StartupDidFail };
    Delegate() : buffer(NULL), http(NULL) {}

private slots:
    void getDeviceInfo();
    void httpCancel();
    void httpCleanup();
    void httpResponseHeader(const QHttpResponseHeader &responseHeader);
    void httpRequestFinished(int requestId, bool error);
    void httpStateChanged(int state);
    void updateProgress(int bytesRead, int totalBytes);

private:
    QBuffer*	buffer;
    QHttp*	http;
    QProgressDialog*	progressDialog;
    bool httpRequestAborted;
    int httpGetId;

    void startup();

};

extern Delegate delegate;

#endif	// DELEGATE_H
