/*	Filename:	centralwidget.cc
	Primary Qt interface class for QProg
	Interface for DIY PIC programmer hardware
	
	Created December 17, 2005 by Brandon Fosdick
	
	Copyright 2005 Brandon Fosdick (BSD License)
*/

#include <iostream>

#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QStringList>

#ifdef	Q_OS_DARWIN

#include <Kernel/mach/std_types.h>
#include <IOKit/IOTypes.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/IOBSD.h>

#endif	//Q_OS_DARWIN

#include "chipinfo.h"
#include "intelhex.h"
#include	"kitsrus.h"
#include "centralwidget.h"

#include "qextserialport.h"

CentralWidget::CentralWidget() : QWidget()
{
	QLabel	*ProgrammerDeviceNodeLabel = new QLabel("Programmer Port");
	QLabel	*TargetTypeLabel = new QLabel("Target Device");
	
	ProgrammerDeviceNode = new QComboBox();
	TargetType = new QComboBox();
	connect(TargetType, SIGNAL(activated(const QString&)), this, SLOT(onTargetComboChange(const QString &)));

	EraseCheckBox = new QCheckBox("Erase before programming");
	VerifyCheckBox = new QCheckBox("Verify after programming");
	VerifyCheckBox->setEnabled(false);
	NewWindowOnReadCheckBox = new QCheckBox("Open new window on read");
	ProgramOnFileChangeCheckBox = new QCheckBox("Reprogram on file change");
	ProgramOnFileChangeCheckBox->setEnabled(false);

	QPushButton	*ProgramButton = new QPushButton("Program");
	QPushButton	*ReadButton = new QPushButton("Read");
	QPushButton	*VerifyButton = new QPushButton("Verify");
	VerifyButton->setEnabled(false);
	QPushButton	*EraseButton = new QPushButton("Erase");
	connect(ProgramButton, SIGNAL(clicked()), this, SLOT(program_all()));
	connect(ReadButton, SIGNAL(clicked()), this, SLOT(read()));
	connect(VerifyButton, SIGNAL(clicked()), this, SLOT(verify()));
	connect(EraseButton, SIGNAL(clicked()), this, SLOT(bulk_erase()));

	FileName = new QComboBox();
	QPushButton	*BrowseButton = new QPushButton("Browse");
	connect(BrowseButton, SIGNAL(clicked()), this, SLOT(browse()));
	QLabel		*FileNameLabel = new QLabel("File");

	QGridLayout	*Layout0 = new QGridLayout;
	Layout0->addWidget(ProgrammerDeviceNodeLabel, 0, 0);
	Layout0->addWidget(TargetTypeLabel, 1, 0);
	Layout0->addWidget(ProgrammerDeviceNode, 0, 2, 1, 2);
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
	FillTargetCombo();		//Fill target device list from settings

	progressDialog = new QProgressDialog(this);
}

bool CentralWidget::FillTargetCombo()
{
	TargetType->clear();
	
	QSettings settings;
	settings.beginGroup("PartsDB");
	QStringList keys = settings.childGroups();
	settings.endGroup();
//	printf("Filling target combo with %d keys\n", keys.count());

	QStringListIterator i(keys);
	while(i.hasNext())
		TargetType->addItem(i.next());

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
	QSettings settings;
	settings.setValue("CentralWidget/TargetCombo/Last/Text", text);
	std::cout << "Target changed to " << text.toStdString() << std::endl;
}

void CentralWidget::browse()
{
	QString directory = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath());
	if( directory.size() != 0 )
	{
		QString str(directory);
		str.remove(0, str.lastIndexOf('/')+1);	//Don't display leading path info
		FileName->addItem(str, QVariant(directory));
		FileName->setCurrentIndex(FileName->count() - 1);
	}
}

//Load the chip info from the settings
void loadChipInfo(QString &part, chipinfo::chipinfo &chip_info)
{
	QSettings	settings;
	settings.beginGroup("PartsDB");
	settings.beginGroup(part);
	QStringList keys = settings.childKeys();
	
//	std::cout << "Loading chipinfo for " << part.toStdString() << std::endl;

	QStringListIterator i(keys);
	while(i.hasNext())
	{
		QString	key(i.next());
		QString	value(settings.value(key).toString());
		if( value.size() == 0 )	//Skip empty keys
			continue;
		chip_info.set(key.toStdString(), value.toStdString());
//		std::cout << key.toStdString() << " -> " << value.toStdString() << std::endl;
	}
}


bool do_reset(kitsrus::kitsrus_t &programmer)
{
	if(!programmer.hard_reset())
	{
		std::cout << "Hard reset failed. Trying a K149 reset...\n";
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
//	programmer.chip_power_off();		//Turn the chip off
//	std::cout << "Erased " /*<< PartName*/ << std::endl;
	return true;
}

void do_rom_write(kitsrus::kitsrus_t &programmer, intelhex::hex_data &HexData)
{
	const intelhex::hex_data::size_type num_rom_bytes = HexData.size_below_addr(programmer.get_rom_size());
	
	if( num_rom_bytes > 0 )
	{
		programmer.chip_power_on();		//Activate programming voltages
//		std::cout << "Programming " << num_rom_bytes << " ROM words for " << PartName << std::endl;
		if( !programmer.write_rom(HexData) )
		{
			std::cerr << "Error programming ROM\n";
			programmer.hard_reset();		//Do a hard reset
			exit(EXIT_FAILURE);								// and then bail out
		}
		programmer.chip_power_off();		//Turn the chip off
	}
	else
		std::cout << "No ROM words in file\n";
}

void do_config_write(kitsrus::kitsrus_t &programmer, intelhex::hex_data &HexData)
{
	programmer.chip_power_on();		//Activate programming voltages
//	std::cout << "Programming Config for " << PartName << std::endl;
	programmer.write_config(HexData);
	programmer.chip_power_off();		//Turn the chip off
}

void do_eeprom_write(kitsrus::kitsrus_t &programmer, intelhex::hex_data &HexData)
{
	const intelhex::hex_data::size_type num_eeprom_bytes = HexData.size_in_range(programmer.get_eeprom_start(), programmer.get_eeprom_start() + programmer.get_eeprom_size());
	
	if(num_eeprom_bytes > 0)
	{
		programmer.chip_power_on();		//Activate programming voltages
//		std::cout << "Programming " << num_eeprom_bytes << " EEPROM bytes for " << PartName << std::endl;
		programmer.write_eeprom(HexData);
		programmer.chip_power_off();		//Turn the chip off
	}
	else
		std::cout << "No EEPROM bytes in file\n";
}

bool do_rom_read(kitsrus::kitsrus_t &programmer, intelhex::hex_data &HexData)
{
	programmer.chip_power_on();		//Activate programming voltages
	programmer.read_rom(HexData);	//Read ROM
	programmer.chip_power_off();	//Turn the chip off
	return true;
}

bool do_config_read(kitsrus::kitsrus_t &programmer, intelhex::hex_data &HexData)
{
	programmer.chip_power_on();			//Activate programming voltages
	programmer.read_config(HexData);	//Read Config
	programmer.chip_power_off();		//Turn the chip off
	return true;
}

bool do_eeprom_read(kitsrus::kitsrus_t &programmer, intelhex::hex_data &HexData)
{
	programmer.chip_power_on();		//Activate programming voltages
	programmer.read_eeprom(HexData);	//Read EEPROM
	programmer.chip_power_off();		//Turn the chip off
	return true;
}

void CentralWidget::program_all()
{
	chipinfo::chipinfo	chip_info;
	QString	target(TargetType->itemText(TargetType->currentIndex()));

	loadChipInfo(target, chip_info);	//Load the chip info from the settings
	
	//Grab the currently selected file name
	QString file_name = (FileName->itemData(FileName->currentIndex())).toString();
	if( file_name.isEmpty() )
	{
		QMessageBox::critical(this, "Error", "You must select a file to program");
		return;
	}

	//Put this in a block to close the serial port early
	{
		QString	path(currentPath());
		kitsrus::kitsrus_t	prog(path, chip_info);	//Programmer interface
		intelhex::hex_data HexData(file_name.toStdString());	//Load the hex file
		
		if( !prog.open() )			//Open the port
		{
			QMessageBox::critical(this, "Error", "Could not open serial port");
			return;
		}

		do_reset(prog);			//Reset the programmer

		//Check the protocol version
		std::string protocol = prog.get_protocol();
		if( protocol != "P018" )
		{
			QMessageBox::critical(this, "Error", tr("Wrong protocol version ( %1 )").arg(QString(protocol.c_str())));
			return;
		}

		prog.init_program_vars();	//Initialize programming variables
		//If erase before programming...
		if( EraseCheckBox->isChecked() )
			do_erase(prog);

		//Set a progress callback
		//Create the progress dialog

		//Do the programming sequence
		//	For some reason config has to be written first or the programmer locks up
		do_config_write(prog, HexData);
		do_eeprom_write(prog, HexData);
		do_rom_write(prog, HexData);
	}

	QMessageBox::information(this, "Program all", "Programming All");
}

void CentralWidget::read()
{
	chipinfo::chipinfo	chip_info;
	intelhex::hex_data	HexData;

	QString	target(TargetType->itemText(TargetType->currentIndex()));

	loadChipInfo(target, chip_info);	//Load the chip info from the settings

	QString	path(ProgrammerDeviceNode->itemData(ProgrammerDeviceNode->currentIndex()).toString());
	
	//Put this in a block to close the serial port early
	{
		kitsrus::kitsrus_t	prog(path, chip_info);	//Programmer interface
		
		if( !prog.open() )			//Open the port
		{
			QMessageBox::critical(this, "Error", "Could not open serial port");
			return;
		}
		do_reset(prog);			//Reset the programmer
		
		//Check the protocol version
		std::string protocol = prog.get_protocol();
		if( protocol != "P018" )
		{
			QMessageBox::critical(this, "Error", tr("Wrong protocol version ( %1 )").arg(QString(protocol.c_str())));
			return;
		}

		prog.init_program_vars();	//Initialize programming variables

		std::cout << "Reading " << prog.get_rom_size() << " ROM words\n";
		do_rom_read(prog, HexData);
		std::cout << "Reading CONFIG\n";
		do_config_read(prog, HexData);
		std::cout << "Reading " << prog.get_eeprom_size() << " EEPROM bytes\n";
		do_eeprom_read(prog, HexData);
	}
	std::cout << "Read " << HexData.size() << " words from " << target.toStdString() << " into " << std::endl;

	QString out_file(QFileDialog::getSaveFileName(this));
	if( !out_file.isEmpty() )
		HexData.write(out_file.toStdString().c_str());

	//Set a progress callback
	//Create the progress dialog
//	QMessageBox::information(this, "Read", "Read");
//	std::cout << "done read\n";

}

void CentralWidget::verify()
{
	QMessageBox::information(this, "Verify", "Verify");
}

void CentralWidget::bulk_erase()
{
	chipinfo::chipinfo	chip_info;
	QString	target(TargetType->itemData(TargetType->currentIndex()).toString());
	
	loadChipInfo(target, chip_info);	//Load the chip info from the settings

	QString	path(ProgrammerDeviceNode->itemData(ProgrammerDeviceNode->currentIndex()).toString());

	//Put this in a block to close the serial port early
	{
		kitsrus::kitsrus_t	prog(path, chip_info);	//Programmer interface
		
		if( !prog.open() )			//Open the port
		{
			QMessageBox::critical(this, "Error", "Could not open serial port");
			return;
		}
		do_reset(prog);			//Reset the programmer
		
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
