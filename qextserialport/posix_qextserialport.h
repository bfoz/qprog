
#ifndef _POSIX_QEXTSERIALPORT_H_
#define _POSIX_QEXTSERIALPORT_H_

#include <stdio.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include "qextserialbase.h"

class Posix_QextSerialPort:public QextSerialBase {
public:
    Posix_QextSerialPort();
    Posix_QextSerialPort(const QString & name);
    Posix_QextSerialPort(const PortSettings& settings);
    Posix_QextSerialPort(const QString & name, const PortSettings& settings);
    virtual ~Posix_QextSerialPort();

    virtual void setBaudRate(BaudRateType);
    virtual void setDataBits(DataBitsType);
    virtual void setParity(ParityType);
    virtual void setStopBits(StopBitsType);
    virtual void setFlowControl(FlowType);
    virtual void setTimeout(ulong, ulong);

    virtual bool open(OpenMode mode=0);
    virtual void close();
    virtual void flush();

    virtual qint64 size() const;
    virtual qint64 bytesAvailable();

    virtual void translateError(ulong error);

    virtual void setDtr(bool set=true);
    virtual void setRts(bool set=true);
    virtual ulong lineStatus();

    virtual qint64 readData(char * data, qint64 maxSize);
    virtual qint64 writeData(const char * data, qint64 maxSize);

private:
    Posix_QextSerialPort(const Posix_QextSerialPort&) {}
    Posix_QextSerialPort& operator=(const Posix_QextSerialPort&) {}

protected:
    QFile Posix_File;
	 int	fd;
    struct termios Posix_CommConfig;
    struct termios save_termios;
    struct timeval Posix_Timeout;
    struct timeval Posix_Copy_Timeout;
};

#endif
