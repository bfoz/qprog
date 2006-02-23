/*	Filename:	centralwidget.h
	Primary Qt interface class for QProg
	Interface for DIY PIC programmer hardware
	
	Created December 17, 2005 by Brandon Fosdick
	
	Copyright 2005 Brandon Fosdick (BSD License)
*/

#ifndef	CENTRALWIDGET_H
#define	CENTRALWIDGET_H

#include <iostream>

#include <QCheckBox>
#include <QComboBox>
#include <QWidget>
#include <QPushButton>
#include <QProgressDialog>
#include <QSettings>

#include	"kitsrus.h"

class CentralWidget : public QWidget
{
	Q_OBJECT
public:
	CentralWidget();
	bool	FillTargetCombo();

	//Handle a progress update from the programmer
	//	Returns false if the cancel button was clicked, otherwise true
	bool handleProgress(int i, int max_i)
	{
		progressDialog->setMaximum(max_i);
		progressDialog->setValue(i);
		return !progressDialog->wasCanceled();
	}

private slots:
	void onEraseCheckBoxChange(int);
	void onVerifyCheckBoxChange(int);
	void onNewWindowOnReadCheckBoxChange(int);
	void onProgramOnFileChangeCheckBoxChange(int);
	void onTargetComboChange(const QString &);
	void onDeviceComboChange(const QString &);
	void browse();
#ifdef	Q_OS_LINUX
	void device_browse();
#endif	//Q_OS_LINUX

	void program_all();
	void read();
	void bulk_erase();
	void onVerify();

private:
	QComboBox	*FileName;
	QComboBox	*ProgrammerDeviceNode;
	QComboBox	*TargetType;

	QCheckBox	*EraseCheckBox;
	QCheckBox	*VerifyCheckBox;
	QCheckBox	*NewWindowOnReadCheckBox;
	QCheckBox	*ProgramOnFileChangeCheckBox;
	QProgressDialog *progressDialog;

	QSettings	settings;

	bool FillPortCombo();
	
	QString currentPath()
	{
		return ProgrammerDeviceNode->itemData(ProgrammerDeviceNode->currentIndex()).toString();
	}
	
	bool doProgrammerInit(kitsrus::kitsrus_t&);
};

#endif	//CENTRALWIDGET_H
