/*	Filename:	centralwidget.cc
	Primary Qt interface class for QProg
	Interface for DIY PIC programmer hardware
	
	Created December 17, 2005 by Brandon Fosdick
	
	Copyright 2005 Brandon Fosdick (BSD License)

	$Id: centralwidget.cc,v 1.21 2008/03/07 22:48:34 bfoz Exp $
*/

#include <iostream>

#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QStringList>

#ifdef	Q_OS_DARWIN
#include <sstream>

#include <Carbon/Carbon.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>

#endif	//Q_OS_DARWIN

#include "chipinfo.h"
#include "intelhex.h"
#include "centralwidget.h"

#include "qextserialport.h"

CentralWidget::CentralWidget() : QWidget()
{
	QLabel	*ProgrammerDeviceNodeLabel = new QLabel("Programmer Port");
	QLabel	*TargetTypeLabel = new QLabel("Target Device");
	
	ProgrammerDeviceNode = new QComboBox();
	connect(ProgrammerDeviceNode, SIGNAL(activated(const QString&)), this, SLOT(onDeviceComboChange(const QString &)));

	TargetType = new QComboBox();
	connect(TargetType, SIGNAL(activated(const QString&)), this, SLOT(onTargetComboChange(const QString &)));

	EraseCheckBox = new QCheckBox("Erase before programming");
	VerifyCheckBox = new QCheckBox("Verify after programming");
	NewWindowOnReadCheckBox = new QCheckBox("Open new window on read");
	ProgramOnFileChangeCheckBox = new QCheckBox("Reprogram on file change");
	ProgramOnFileChangeCheckBox->setEnabled(false);

	//Connect the checkbox change signals so the state changes can be saved to settings
	connect(EraseCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onEraseCheckBoxChange(int)));
	connect(VerifyCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onVerifyCheckBoxChange(int)));
	connect(NewWindowOnReadCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onNewWindowOnReadCheckBoxChange(int)));
	connect(ProgramOnFileChangeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onProgramOnFileChangeCheckBoxChange(int)));

	QPushButton	*ProgramButton = new QPushButton("Program");
	QPushButton	*ReadButton = new QPushButton("Read");
	QPushButton	*VerifyButton = new QPushButton("Verify");
	QPushButton	*EraseButton = new QPushButton("Erase");
	connect(ProgramButton, SIGNAL(clicked()), this, SLOT(program_all()));
	connect(ReadButton, SIGNAL(clicked()), this, SLOT(read()));
	connect(VerifyButton, SIGNAL(clicked()), this, SLOT(onVerify()));
	connect(EraseButton, SIGNAL(clicked()), this, SLOT(bulk_erase()));

	FileName = new QComboBox();
	FileName->setMaxCount(5);

	QPushButton	*BrowseButton = new QPushButton("Browse");
	connect(BrowseButton, SIGNAL(clicked()), this, SLOT(browse()));
	QLabel		*FileNameLabel = new QLabel("File");

	QGridLayout	*Layout0 = new QGridLayout;
	Layout0->addWidget(ProgrammerDeviceNodeLabel, 0, 0);
	Layout0->addWidget(TargetTypeLabel, 1, 0);
#ifdef	Q_OS_LINUX
	ProgrammerDeviceNode->setMaxCount(1);
	Layout0->addWidget(ProgrammerDeviceNode, 0, 1, 1, 2);
	QPushButton	*DeviceBrowseButton = new QPushButton("Browse");
	connect(DeviceBrowseButton, SIGNAL(clicked()), this, SLOT(device_browse()));
	Layout0->addWidget(DeviceBrowseButton, 0, 3);
#else	//Q_OS_LINUX
	Layout0->addWidget(ProgrammerDeviceNode, 0, 2, 1, 2);
#endif	//Q_OS_LINUX

	Layout0->addWidget(TargetType, 1, 2, 1, 2);

	Layout0->addWidget(FileNameLabel, 2, 0);
	Layout0->addWidget(FileName, 2, 1, 1, 2);
	Layout0->addWidget(BrowseButton, 2, 3);
	
	Layout0->addWidget(EraseCheckBox, 3, 0, 1, 2);
	Layout0->addWidget(VerifyCheckBox, 3, 2, 1, 2);

#ifdef	Q_OS_DARWIN
	Layout0->addWidget(NewWindowOnReadCheckBox, 4, 0, 1, 2);
#endif	//Q_OS_DARWIN
	Layout0->addWidget(ProgramOnFileChangeCheckBox, 4, 2, 1, 2);
	
	Layout0->addWidget(ProgramButton, 5, 0);
	Layout0->addWidget(ReadButton, 5, 1);
	Layout0->addWidget(VerifyButton, 5, 2);
	Layout0->addWidget(EraseButton, 5, 3);
	
	setLayout(Layout0);

	FillPortCombo();		//Fill the programmer dropdown with the available ports

	//Set the device combo to the last used port
	QString last_device = settings.value("CentralWidget/DeviceCombo/Last/Text").toString();
	int j=0;
	if( !last_device.isEmpty() )
		if( (j = ProgrammerDeviceNode->findText(last_device)) != -1 )
			ProgrammerDeviceNode->setCurrentIndex(j);
	
	//Restore the checkbox states from settings
	EraseCheckBox->setCheckState((Qt::CheckState)settings.value("CentralWidget/EraseBeforeProgrammingCheckBox/checkState").toInt());
	VerifyCheckBox->setCheckState((Qt::CheckState)settings.value("CentralWidget/VerifyAfterProgrammingCheckBox/checkState").toInt());
	NewWindowOnReadCheckBox->setCheckState((Qt::CheckState)settings.value("CentralWidget/NewWindowOnReadCheckBox/checkState").toInt());
	ProgramOnFileChangeCheckBox->setCheckState((Qt::CheckState)settings.value("CentralWidget/ProgramOnFileChangeCheckBox/checkState").toInt());

	//Restore the file list from settings
	j = settings.beginReadArray("CentralWidget/FileName/Last");
	for(int i = 0; i < j; ++i)
	{
		settings.setArrayIndex(i);
		FileName->addItem(settings.value("Name").toString(), settings.value("Path"));
	}
	settings.endArray();
	
	progressDialog = new QProgressDialog(this);
	progressDialog->setModal(true);
}

void CentralWidget::onEraseCheckBoxChange(int state)
{
	settings.setValue("CentralWidget/EraseBeforeProgrammingCheckBox/checkState", state);
}

void CentralWidget::onVerifyCheckBoxChange(int state)
{
	settings.setValue("CentralWidget/VerifyAfterProgrammingCheckBox/checkState", state);
}

void CentralWidget::onNewWindowOnReadCheckBoxChange(int state)
{
	settings.setValue("CentralWidget/NewWindowOnReadCheckBox/checkState", state);
}

void CentralWidget::onProgramOnFileChangeCheckBoxChange(int state)
{
	settings.setValue("CentralWidget/ProgramOnFileChangeCheckBox/checkState", state);
}

bool CentralWidget::FillTargetCombo()
{
	TargetType->clear();

	QSettings	settings;
	if( !settings.childGroups().contains("DeviceInfo") )
	{
		QMessageBox::information(this, tr(""), tr("No Device Info"));
		return false;
	}

	settings.beginGroup("DeviceInfo");

	if( !settings.childGroups().contains("Devices") )
	{
		QMessageBox::information(this, tr(""), tr("No Devices"));
		return false;
	}

	size_t numDevices = settings.beginReadArray("Devices");
	QStringList devices = settings.childGroups();
	numDevices = devices.count();	// Rude hack to deal with QSettings bug

	for(size_t i=0; i<numDevices; ++i)
	{
		settings.setArrayIndex(i);
		QVariant n = settings.value("Name");
		if( n.isValid() && (n.toString().length()!=0) )
			TargetType->addItem(n.toString(), (unsigned)i);
	}
	settings.endArray();
	settings.endGroup();
//	printf("Filling target combo with %d keys\n", keys.count());

	//Set the combo to the last used target
	QString last_target = settings.value("CentralWidget/TargetCombo/Last/Text").toString();
	int j=0;
	if( !last_target.isEmpty() )
		if( (j = TargetType->findText(last_target)) != -1 )
			TargetType->setCurrentIndex(j);

	return true;
}

void CentralWidget::onTargetComboChange(const QString &text)
{
	settings.setValue("CentralWidget/TargetCombo/Last/Text", text);
}

void CentralWidget::onDeviceComboChange(const QString &text)
{
	settings.setValue("CentralWidget/DeviceCombo/Last/Text", text);
}

void CentralWidget::browse()
{
	QString initdir;
	if( FileName->currentIndex() == -1 )
		initdir = QDir::currentPath();
	else
		initdir = (FileName->itemData(FileName->currentIndex())).toString();
			
	QString directory = QFileDialog::getOpenFileName(this, tr("Open File"), initdir);
	if( directory.size() != 0 )
	{
		QString str(directory);
		str.remove(0, str.lastIndexOf('/')+1);	//Don't display leading path info
		FileName->insertItem(0, str, QVariant(directory));
		FileName->setCurrentIndex(0);

		//Save the new file list to settings
		settings.beginWriteArray("CentralWidget/FileName/Last");
		for(int i = 0; i < FileName->count(); ++i)
		{
			settings.setArrayIndex(i);
			settings.setValue("Name", FileName->itemText(i));
			settings.setValue("Path", FileName->itemData(i));
		}
		settings.endArray();
	}
}

#ifdef	Q_OS_LINUX

void CentralWidget::device_browse()
{
	QString initdir;
	if( ProgrammerDeviceNode->currentIndex() == -1 )
		initdir = "/dev";
	else
		initdir = (ProgrammerDeviceNode->itemData(ProgrammerDeviceNode->currentIndex())).toString();
	
	QString directory = QFileDialog::getOpenFileName(this, tr("Open File"), initdir);
	if( directory.size() != 0 )
	{
		QString str(directory);
		str.remove(0, str.lastIndexOf('/')+1);	//Don't display leading path info
		ProgrammerDeviceNode->clear();
		ProgrammerDeviceNode->addItem(str, QVariant(directory));

		settings.setValue("CentralWidget/DeviceCombo/Last/Text", directory);
	}	
}

#endif	//Q_OS_LINUX

//Load the chip info from the settings
bool loadChipInfo(QString &part, chipinfo::chipinfo &chip_info)
{
	QSettings	settings;
	if( !settings.childGroups().contains("DeviceInfo") )
		return false;

	settings.beginGroup("DeviceInfo");
	
	if( !settings.childGroups().contains("Devices") )
		return false;
	
	size_t numDevices = settings.beginReadArray("Devices");
	QStringList devices = settings.childGroups();
	numDevices = devices.count();	// Rude hack to deal with QSettings bug
	
	for(size_t i=0; i<numDevices; ++i)
	{
		settings.setArrayIndex(i);
		QVariant n = settings.value("Name");
		if( n.isValid() && (n.toString()==part) )
		{
			QStringList keys = settings.childKeys();
			QStringListIterator i(keys);
			while(i.hasNext())
			{
				QString	key(i.next());
				QString	value(settings.value(key).toString());
				if( value.size() == 0 )	//Skip empty keys
					continue;
				chip_info.set(key.toStdString(), value.toStdString());
			}
		}
	}
	settings.endArray();
	settings.endGroup();

	return true;
}


bool do_reset(kitsrus::kitsrus_t &programmer)
{
	if(!programmer.hard_reset())
	{
//		std::cout << "Hard reset failed. Trying a K149 reset...\n";
		//Try assuming that the programmer is a Kit149
		programmer.set_149();
		if(!programmer.hard_reset())
		{
			QMessageBox::critical(0, "Error", "Could not reset programmer");
			return false;
		}
	}
	
	//Enter command mode
	if(!programmer.command_mode())
	{
		QMessageBox::critical(0, "Error", "Could not enter Command Mode");
		return false;
	}
	return true;
}

bool do_erase(kitsrus::kitsrus_t &programmer)
{
//	programmer.chip_power_on();		//Activate programming voltages
	if( !programmer.erase_chip() )	//Erase the chip first
	{
		QMessageBox::critical(0, "Error", "Could not erase part");
		return false;
	}
	programmer.chip_power_off();		//Turn the chip off
	return true;
}

bool do_rom_write(kitsrus::kitsrus_t &programmer, intelhex::hex_data &HexData)
{
	const intelhex::hex_data::size_type num_rom_bytes = HexData.size_below_addr(programmer.get_rom_size());
	
	if( num_rom_bytes > 0 )
	{
		programmer.chip_power_on();		//Activate programming voltages
//		std::cout << "Programming " << num_rom_bytes << " ROM words for " << PartName << std::endl;
		if( !programmer.write_rom(HexData) )
		{
			std::cerr << "Error programming ROM\n";
			programmer.hard_reset();		//Do a hard reset to clear the error and turn power off
			return false;								// and then bail out
		}
		programmer.chip_power_off();		//Turn the chip off
	}
	else
		std::cout << "No ROM words in file\n";
	return true;
}

bool do_config_write(kitsrus::kitsrus_t &programmer, intelhex::hex_data &HexData)
{
	programmer.chip_power_on();		//Activate programming voltages
//	std::cout << "Programming Config for " << PartName << std::endl;
	if( !programmer.write_config(HexData) )
	{
		programmer.hard_reset();		//Do a hard reset to clear the error and turn power off
		return false;
	}
	programmer.chip_power_off();		//Turn the chip off
	return true;
}

bool do_eeprom_write(kitsrus::kitsrus_t &programmer, intelhex::hex_data &HexData)
{
	const intelhex::hex_data::size_type num_eeprom_bytes = HexData.size_in_range(programmer.get_eeprom_start(), programmer.get_eeprom_start() + programmer.get_eeprom_size());
	
	if(num_eeprom_bytes > 0)
	{
		programmer.chip_power_on();		//Activate programming voltages
//		std::cout << "Programming " << num_eeprom_bytes << " EEPROM bytes for " << PartName << std::endl;
		if( !programmer.write_eeprom(HexData) )
		{
			programmer.hard_reset();		//Do a hard reset to clear the error and turn power off
			return false;
		}
		programmer.chip_power_off();		//Turn the chip off
	}
	else
		std::cout << "No EEPROM bytes in file\n";
	return true;
}

bool do_rom_read(kitsrus::kitsrus_t &programmer, intelhex::hex_data &HexData)
{
	programmer.chip_power_on();		//Activate programming voltages
	if( !programmer.read_rom(HexData) )	//Read ROM
	{
		programmer.hard_reset();		//Do a hard reset to clear the error and turn power off
		return false;
	}
	programmer.chip_power_off();	//Turn the chip off
	return true;
}

bool do_config_read(kitsrus::kitsrus_t &programmer, intelhex::hex_data &HexData)
{
	programmer.chip_power_on();			//Activate programming voltages
	if( !programmer.read_config(HexData) )	//Read Config
	{
		programmer.hard_reset();		//Do a hard reset to clear the error and turn power off
		return false;
	}
	programmer.chip_power_off();		//Turn the chip off
	return true;
}

bool do_eeprom_read(kitsrus::kitsrus_t &programmer, intelhex::hex_data &HexData)
{
	programmer.chip_power_on();		//Activate programming voltages
	if( !programmer.read_eeprom(HexData) )	//Read EEPROM
	{
		programmer.hard_reset();		//Do a hard reset to clear the error and turn power off
		return false;
	}
	programmer.chip_power_off();		//Turn the chip off
	return true;
}

bool handle_progress(void* p, int i, int max_i)
{
	return static_cast<CentralWidget*>(p)->handleProgress(i, max_i);
}

//Handle the actual write sequence
bool do_write_all(kitsrus::kitsrus_t& prog, intelhex::hex_data &HexData, bool erase_first, QProgressDialog *progressDialog)
{
	//If erase before programming...
	if( erase_first )
		do_erase(prog);
	
	//Do the programming sequence
	//	For some reason config has to be written first or the programmer locks up
	progressDialog->setLabelText("Writing Config");	//Set the progress dialog label
	if( !do_config_write(prog, HexData) )
		return false;
	
	progressDialog->setLabelText("Writing EEPROM");	//Set the progress dialog label
	if( !do_eeprom_write(prog, HexData) )
		return false;
	
	progressDialog->setLabelText("Writing ROM");	//Set the progress dialog label
	if( !do_rom_write(prog, HexData) )					//Write the ROM words
		return false;

	return true;
}

//Handle the actual write sequence
bool do_read_all(kitsrus::kitsrus_t& prog, intelhex::hex_data &HexData, QProgressDialog *progressDialog)
{
	//		std::cout << "Reading " << prog.get_rom_size() << " ROM words\n";
	progressDialog->setLabelText("Reading ROM");	//Set the progress dialog label
	if( !do_rom_read(prog, HexData) )
		return false;
	
	progressDialog->setLabelText("Reading Config");	//Set the progress dialog label
	if( !do_config_read(prog, HexData) )
		return false;

	//		std::cout << "Reading " << prog.get_eeprom_size() << " EEPROM bytes\n";
	progressDialog->setLabelText("Reading EEPROM");	//Set the progress dialog label
	if( !do_eeprom_read(prog, HexData) )
		return false;

	return true;
}

bool CentralWidget::doProgrammerInit(kitsrus::kitsrus_t& prog)
{
	if( !prog.open() )			//Open the port
	{
		QMessageBox::critical(this, "Error", "Could not open serial port");
		return false;
	}

	if( !do_reset(prog) )			//Reset the programmer
		return false;

	//Check the protocol version
	std::string protocol = prog.get_protocol();

	if( protocol != "P018" )
	{
		QMessageBox::critical(this, "Error", tr("Wrong protocol version ( %1 )").arg(QString(protocol.c_str())));
		return false;
	}

	prog.init_program_vars();	//Initialize programming variables
	
	prog.set_callback(&handle_progress, this);	//Set the progress callback
	
	return true;
}

void CentralWidget::program_all()
{
	chipinfo::chipinfo	chip_info;
	QString	target(TargetType->itemText(TargetType->currentIndex()));

	//Load the chip info from the settings
	if( !loadChipInfo(target, chip_info) )
	    return;
	
	//Grab the currently selected file name
	if( FileName->currentIndex() == -1 )
	{
		browse();	//Open a file dialog if a file hasn't been selected yet
		if( FileName->currentIndex() == -1 )
		{
			QMessageBox::critical(this, "Error", "You must select a file to program");
			return;
		}
	}
	QString file_name = (FileName->itemData(FileName->currentIndex())).toString();

	//Put this in a block to close the serial port early
	{
		QString	path(currentPath());
		kitsrus::kitsrus_t	prog(path, chip_info);	//Programmer interface
		intelhex::hex_data HexData(file_name.toStdString());	//Load the hex file

		if( !doProgrammerInit(prog) )
			return;
		
		if( !do_write_all(prog, HexData, EraseCheckBox->isChecked(), progressDialog) )
		{
			progressDialog->reset();
			QMessageBox::critical(this, "Error", tr("Error writing to chip"));
			return;
		}
	}
	if( VerifyCheckBox->isChecked() )
		onVerify();
}

#ifdef	Q_OS_DARWIN
void handle_open_new_text(intelhex::hex_data& HexData)
{
	OSErr err = noErr;
	FSRef	ref;
	LSApplicationParameters	app_parms = {0, kLSLaunchNoParams, &ref, NULL, NULL, NULL, NULL};
	ProcessSerialNumber	psn;

	//Find the preferred application for text files
	if( LSGetApplicationForInfo('TEXT', kLSUnknownCreator, NULL, kLSRolesAll, &ref, NULL) != noErr )
	{
		QMessageBox::critical(NULL, "Error", "Could not find preferred text editor");
		return;
	}
	
	if( LSOpenApplication(&app_parms, &psn) != noErr )	//Launch the application
	{
		QMessageBox::critical(NULL, "Error", "Could not open preferred text editor");
		return;
	}
	
	AppleEvent createEvent;
	//Build an event to open a new document
	if( AEBuildAppleEvent(kAECoreSuite, kAECreateElement, typeProcessSerialNumber, &psn, sizeof(psn), kAutoGenerateReturnID, kAnyTransactionID, &createEvent, NULL, "kocl:type(docu)") != noErr )
	{
		AppleEvent replyEvent;

/*		Handle pretty_handle;
		err = AEPrintDescToHandle(&createEvent, &pretty_handle);
		if( err == noErr )
			printf("event: %s\n", *pretty_handle);
		else
			printf("bad pretty print %d\n", err);
*/
		//Send an Apple Event to open a new document
		if( AESendMessage(&createEvent, &replyEvent, kAEWaitReply, kAEDefaultTimeout) != noErr )
		{
			AppleEvent openEvent;
			AEDesc	docSpec, insertLoc;
			std::ostringstream text;

			//Get the doc spec from the reply event
			AEGetParamDesc(&replyEvent, keyDirectObject, typeWildCard, &docSpec);
			AEDisposeDesc(&createEvent);	//The create event is no longer needed

			//Build an Apple Event to send the text to the editor
			HexData.write(text);
			AEBuildAppleEvent(kAECoreSuite, kAECreateElement, typeProcessSerialNumber, &psn, sizeof(psn), kAutoGenerateReturnID, kAnyTransactionID, &openEvent, NULL, "kocl:type(cpar), data:TEXT(@)", text.str().c_str());
//			AEPutAttributeDesc(&openEvent, keySubjectAttr, &docSpec);	//Add the doc spec as an attribute
			AEBuildDesc(&insertLoc, NULL, "insl{kobj:@, kpos:end}", &docSpec);	//Make an insert location
			AEPutParamDesc(&openEvent, keyAEInsertHere, &insertLoc);	//Insert the insert location

			//Send an Apple Event to append the new text
			AESendMessage(&openEvent, NULL, kAENoReply, kAEDefaultTimeout);
			
			//Cleanup
			AEDisposeDesc(&docSpec);
			AEDisposeDesc(&insertLoc);
			AEDisposeDesc(&openEvent);
			AEDisposeDesc(&replyEvent);
		}
	}
}
#endif	//Q_OS_DARWIN

void CentralWidget::read()
{
	chipinfo::chipinfo	chip_info;
	intelhex::hex_data	HexData;

	QString	target(TargetType->itemText(TargetType->currentIndex()));

	//Load the chip info from the settings
	if( !loadChipInfo(target, chip_info) )
	    return;

	QString	path(ProgrammerDeviceNode->itemData(ProgrammerDeviceNode->currentIndex()).toString());

	//Put this in a block to close the serial port early
	{
		kitsrus::kitsrus_t	prog(path, chip_info);	//Programmer interface
		
		if( !doProgrammerInit(prog) )
			return;

		if( !do_read_all(prog, HexData, progressDialog) )
		{
			progressDialog->reset();
			QMessageBox::critical(this, "Error", tr("Error reading chip"));
		}
	}


#ifdef	Q_OS_DARWIN
	if( NewWindowOnReadCheckBox->isChecked() )
		handle_open_new_text(HexData);
	else
	{
#endif	//Q_OS_DARWIN
		QString out_file(QFileDialog::getSaveFileName(this));
		if( !out_file.isEmpty() )
			HexData.write(out_file.toStdString().c_str());
#ifdef	Q_OS_DARWIN
	}
#endif	//Q_OS_DARWIN
}

void CentralWidget::onVerify()
{
	chipinfo::chipinfo	chip_info;
	QString	target(TargetType->itemText(TargetType->currentIndex()));
	
	//Load the chip info from the settings
	if( !loadChipInfo(target, chip_info) )
	    return;
	
	//Grab the currently selected file name
	if( FileName->currentIndex() == -1 )
	{
		browse();	//Open a file dialog if a file hasn't been selected yet
		if( FileName->currentIndex() == -1 )
		{
			QMessageBox::critical(this, "Error", "You must select a file to verify against");
			return;
		}
	}
	QString file_name = (FileName->itemData(FileName->currentIndex())).toString();
	
	//Put this in a block to close the serial port early
	{
		QString	path(currentPath());
		kitsrus::kitsrus_t	prog(path, chip_info);	//Programmer interface
		intelhex::hex_data HexData(file_name.toStdString());	//Load the hex file
		
		if( !doProgrammerInit(prog) )
			return;
		
		intelhex::hex_data VerifyData;
		if( !do_read_all(prog, VerifyData, progressDialog) )
		{
			progressDialog->reset();
			QMessageBox::critical(this, "Error", tr("Error reading chip"));
		}

	// Compare ROM
	const bool flash = intelhex::compare(HexData, VerifyData, chip_info.romBlank(), chip_info.romBegin(), chip_info.romEnd());
	// Compare EEPROM
	const bool eeprom = intelhex::compare(HexData, VerifyData, chip_info.eepromBlank(), chip_info.eepromBegin(), chip_info.eepromEnd());
	// Compare Config
//	const bool config = intelhex::compare(HexData, VerifyData, chip_info.configBlank(), chip_info.configBegin(), chip_info.configEnd());

	QMessageBox::information(this, "Verify Results", 
				 tr("Flash\t%1\nEEPROM\t%2\nConfig\t%3")
				    .arg(flash?"Pass":"Fail")
				    .arg(eeprom?"Pass":"Fail")
				    .arg("Not Verified")
				 );
	}
}

void CentralWidget::bulk_erase()
{
	chipinfo::chipinfo	chip_info;
	QString	target(TargetType->itemData(TargetType->currentIndex()).toString());
	
	//Load the chip info from the settings
	if( !loadChipInfo(target, chip_info) )
	    return;

	QString	path(ProgrammerDeviceNode->itemData(ProgrammerDeviceNode->currentIndex()).toString());

	//Put this in a block to close the serial port early
	{
		kitsrus::kitsrus_t	prog(path, chip_info);	//Programmer interface
		
		if( !prog.open() )			//Open the port
		{
			QMessageBox::critical(this, "Error", "Could not open serial port");
			return;
		}
		if( !do_reset(prog) )			//Reset the programmer
			return;
		
		//Check the protocol version
		std::string protocol = prog.get_protocol();
		if( protocol != "P018" )
		{
			QMessageBox::critical(this, "Error", tr("Wrong protocol version ( %1 )").arg(QString(protocol.c_str())));
			return;
		}
		
		do_erase(prog);		//Do the erase	
	}

	QMessageBox::information(this, "Bulk Erase", "Successfully Erased");
}

#ifdef	Q_OS_DARWIN

kern_return_t FindPorts(io_iterator_t *matchingServices)
{
    kern_return_t       kernResult;
    mach_port_t         masterPort;
    CFMutableDictionaryRef  classesToMatch;

    kernResult = IOMasterPort(MACH_PORT_NULL, &masterPort);	
    if (KERN_SUCCESS != kernResult)
    {
		QMessageBox::critical(NULL, "Error", "IOMasterPort returned error");
        printf("IOMasterPort returned %d\n", kernResult);
		return kernResult;
    }

    // Serial devices are instances of class IOSerialBSDClient.	
    classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    if (classesToMatch == NULL)
    {
        printf("IOServiceMatching returned a NULL dictionary.\n");
    }
    else
	{
		CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDRS232Type));
		// Each serial device object has a property with key
        // kIOSerialBSDTypeKey and a value that is one of
        // kIOSerialBSDAllTypes, kIOSerialBSDModemType, 
        // or kIOSerialBSDRS232Type. You can change the
        // matching dictionary to find other types of serial
        // devices by changing the last parameter in the above call
        // to CFDictionarySetValue.
    }

    kernResult = IOServiceGetMatchingServices(masterPort, classesToMatch, matchingServices);
    if (KERN_SUCCESS != kernResult)
    {
		QMessageBox::critical(NULL, "Error", "IOServiceGetMatchingServices returned error");
        printf("IOServiceGetMatchingServices returned %d\n", kernResult);
    }
	return kernResult;
}

//Enumerate the available ports and fill up the relevant combo box
//	Returns true if a port is found, false otherwise
bool CentralWidget::FillPortCombo()
{
	io_object_t     Service;
	bool   result = false;
	char PortPath[256];
	CFIndex maxPathSize = 256;
	io_iterator_t   serialPortIterator;

	FindPorts(&serialPortIterator);		//Get an iterator to a list of ports

	*PortPath = '\0';	//Initialize the string

	// Iterate across all devices found
	while( (Service = IOIteratorNext(serialPortIterator)) )
	{
		// Get the callout device's path (/dev/cu.xxxxx). 
		// The callout device should almost always be
		// used. You would use the dialin device (/dev/tty.xxxxx) when 
		// monitoring a serial port for
		// incoming calls, for example, a fax listener.
		CFTypeRef deviceAsCFString = IORegistryEntryCreateCFProperty(Service, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0);
		if(deviceAsCFString)
		{
			// Convert the path from a CFString to a NULL-terminated C string
			Boolean result = CFStringGetCString((const __CFString*)deviceAsCFString, (char *)(&PortPath), maxPathSize, kCFStringEncodingASCII);
			CFRelease(deviceAsCFString);
			if(result)
			{
				QString str(PortPath);
				str.remove(0, str.lastIndexOf('/')+1);	//Don't display leading path info
				ProgrammerDeviceNode->addItem(str, QVariant(PortPath));
				result = true;
			}
		}
		IOObjectRelease(Service);  //Release the io_service_t now that we are done with it.
	}

	IOObjectRelease(serialPortIterator);    // Release the iterator
	return result;
}
#endif	//Q_OS_DARWIN

#ifdef	Q_WS_X11
#	ifdef	Q_OS_LINUX
bool CentralWidget::FillPortCombo()
{
}

#	else	//Q_OS_LINUX

//Enumerate the available ports and fill up the relevant combo box
//	Returns true if a port is found, false otherwise
bool CentralWidget::FillPortCombo()
{
	QDir	dir("/dev");
	if( !dir.exists() )
		return false;

	dir.setFilter(QDir::System);
	QStringList 	names("cu*");
	dir.setNameFilters(names);
	
	QStringList devs = dir.entryList();
	devs = devs.filter(QRegExp("\\d\\s*$"));	//Filter the .init and .lock nodes on FreeBSD
	QStringListIterator i(devs);
	while(i.hasNext())
	{
		QString path(i.next());
		ProgrammerDeviceNode->addItem(i.peekPrevious(), QVariant(path.prepend("/dev/")));
	}
	
	return true;
}	

#endif	//Q_OS_LINUX

#endif	//Q_WS_X11

#ifdef	Q_WS_WIN

//Enumerate the available ports and fill up the relevant combo box
//	Returns true if a port is found, false otherwise
bool CentralWidget::FillPortCombo()
{
	ProgrammerDeviceNode->addItem("COM 1", QVariant("COM1"));
	ProgrammerDeviceNode->addItem("COM 2", QVariant("COM2"));
	ProgrammerDeviceNode->addItem("COM 3", QVariant("COM3"));
	ProgrammerDeviceNode->addItem("COM 4", QVariant("COM4"));

	return true;
}	

#endif	//Q_WS_WIN
