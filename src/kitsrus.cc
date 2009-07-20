/*  Interface to Tony Nixon's PIC Programmers via the Kitsrus protocol
    This works for Kits 128, 149A, 149B, 150, 182

    Copyright 2005 Brandon Fosdick (BSD License)
*/

#include <fcntl.h>
#include <iostream>

#include "kitsrus.h"
#include "intelhex.h"

static const char* firmwareNames[] =
{
    "Kit 128",
    "Kit 149A",
    "Kit 149B",
    "Kit 150",
    "Kit 170",
    "Kit 182"
};

namespace kitsrus
{
#ifndef	Q_WS_WIN
    #define	HIBYTE(a)	(uint8_t)(a>>8)
    #define	LOBYTE(a)	(uint8_t)(a&0x00FF)
#endif	//Q_WS_WIN

    // Switch from power-on mode to command mode
    bool kitsrus_t::command_mode()
    {
	write('P');
	if( read() == 'P' )
	    return true;
	else
	    return false;
    }

    // Do a soft reset of the device
    // Send a 1 to the device. If it is in the command table it will reset. Either way it should return 'Q'
    bool kitsrus_t::soft_reset()
    {
	// Send a 1 to the device.
	//  If it is in the command table it will reset. Either way it should return 'Q'
	write(CMD_RESET);
	if((read()) == 'Q')
	    return true;
	else
	    return false;
    }

    //Do a hard reset of the device
    bool kitsrus_t::hard_reset()
    {
	set_dtr();	//S et DTR high
#ifdef	Q_WS_WIN	// Deal with win32 stupidity
	Sleep(100);
#else
	usleep(10);	// Delay
#endif
	clear_dtr();	// Set DTR low

	if( read()=='B' )
	{
	    firmware = read();	//Ignore the firmware type
	    qDebug("Found Firmware Type=%X '%s'\n", firmware, firmwareName());
	    return true;
	}
	else
	    return false;
    }

    bool kitsrus_t::init_program_vars()
    {
	write(CMD_INITVAR);
	write(HIBYTE(info.rom_size));
	write(LOBYTE(info.rom_size));
	write(HIBYTE(info.eeprom_size));
	write(LOBYTE(info.eeprom_size));
	write(info.core_type);   //FIXME  CoreType
	uint8_t i = (info.cal_word)?0x01:0x00;
	i |= (info.band_gap)?0x02:0x00;
	i |= (info.single_panel)?0x04:0x00;
	i |= (info.fast_power)?0x08:0x00;
	write(i);	 //Program flags
	write(info.program_delay);
	write(info.power_sequence);
	write(info.erase_mode);
	write(info.program_tries);
	write(info.over_program);
	if(read() == 'I')
	    return true;
	return false;
    }


    bool kitsrus_t::chip_power_on()
    {
	write(CMD_VPP_ON);
	if( read() == 'V' )
	    return true;
	else
	    return false;
    }

    bool kitsrus_t::chip_power_off()
    {
	write(CMD_VPP_OFF);
	if( read() == 'v' )
	    return true;
	else
	    return false;
    }

    bool kitsrus_t::chip_power_cycle()
    {
	write(CMD_VPP_CYCLE);
	if( read() == 'V' )
	    return true;
	else
	    return false;
    }

    bool kitsrus_t::write_rom(intelhex::hex_data &HexData)
    {
	uint8_t i;
	intelhex::hex_data::address_t	j(0);
	uint16_t k;
	intelhex::hex_data::size_type size;

	// Set the value for device to be programmed
	HexData.fill(info.get_blank_value());

	//Figure out how many ROM words need to be written
	size = 1 + HexData.max_addr_below((info.rom_size-1)*2);

	//Send program rom command
	write(CMD_WRITE_ROM);
	write( (size & 0xFF00) >> 8);  //Send size hi
	write(size & 0x00FF); //Send size low

	while(1)
	{
	    switch(read())
	    {
		case 'P':
		    emit_callback((j>size)?size:j,size);
		    return true;
		case 'N':
		    std::cerr << __FUNCTION__ << ": Got N at address ";
		    k = read();
		    k = k << 8;
		    k |= (0x00FF & read());
		    std::cerr << std::hex << k;
		    std::cerr << " with word ";
		    k = read();
		    k = k << 8;
		    k |= (0x00FF & read());
		    std::cerr << std::hex << k;
		    std::cerr << std::endl;
		    return false;
		case 'Y':
		    for(i=0; i < 32; ++i, ++j)
			write( HexData.get(j) );
		    if( !emit_callback((j>size)?size:j,size) )	//Emit callback and check for cancellation
			return false;
		    break;
		default:
		    std::cerr << __FUNCTION__ << ": Got unexpected character\n";
		    return false;
	    }
	}
	return true;
    }

//#define	WRITE_EEPROM_DEBUG
    bool kitsrus_t::write_eeprom(intelhex::hex_data &HexData)
    {
	intelhex::hex_data::address_t	j(info.get_eeprom_start());
	intelhex::hex_data::address_t	eeprom_start(info.get_eeprom_start());
	intelhex::hex_data::address_t	eeprom_end;
	uint16_t progress(0);
	intelhex::hex_data::size_type size;

	// Set the value for device to be programmed
	HexData.fill(0xFF);

	// Ideally we would figure out how many ROM words are going to be written
	//  and then write only that. But, to make things simpler we'll just write
	//  to all of the ROM.
#if defined(WRITE_EEPROM_DEBUG)
	std::cout << __FUNCTION__ << ": eeprom_start = " << std::hex << j << std::endl;
	std::cout << __FUNCTION__ << ": eeprom_size = " << std::hex << info.eeprom_size << std::endl;
#endif
	size = HexData.size_in_range(j, info.get_eeprom_start() + info.eeprom_size);
	// Make size an even number
	if( (size % 2) != 0 )
	    ++size;
	eeprom_end = j + size - 1;
#if defined(WRITE_EEPROM_DEBUG)
	std::cout << __FUNCTION__ << ": size = " << size << std::endl;
	std::cout << __FUNCTION__ << ": eeprom_end = " << std::hex << eeprom_end << std::endl;
#endif

	// Send program rom command
	write(CMD_WRITE_EEPROM);
	write( (size & 0xFF00) >> 8);  //Send size hi
	write(size & 0x00FF); //Send size low

	while(1)
	{
	    switch(read())
	    {
		case 'P':
		    emit_callback((progress>size)?size:progress,size);
		    return true;
		case 'Y':
		    write(HexData.get(j));
#if defined(WRITE_EEPROM_DEBUG)
		    std::cout << __FUNCTION__ << ": wrote " << std::hex << HexData[j] << "\n";
#endif
		    ++j;
		    write(HexData.get(j));
#if defined(WRITE_EEPROM_DEBUG)
		    std::cout << __FUNCTION__ << ": wrote " << std::hex << HexData[j] << "\n";
#endif
		    ++j;
		    progress = j - eeprom_start;
		    if( !emit_callback((progress>size)?size:progress,size) )	//Emit callback and check for cancellation
			    return false;
		    break;
		default:
		    std::cerr << __FUNCTION__ << ": Got unexpected character\n";
		    return false;
	    }
	}
	return true;
    }

    bool kitsrus_t::write_config(intelhex::hex_data &HexData)
    {
	std::vector<uint8_t> tmp_config(22, 0xFF);

	intelhex::hex_data::address_t	i;
	// If the ID bits were specified use them
	//  otherwise use blanks
	i = info.get_id_start();
	if( HexData.isset(i) )
	{
	    tmp_config[0] = HexData[i++];
	    tmp_config[1] = HexData[i++];
	    tmp_config[2] = HexData[i++];
	    tmp_config[3] = HexData[i];
	}
	tmp_config[4] = 'F';
	tmp_config[5] = 'F';
	tmp_config[6] = 'F';
	tmp_config[7] = 'F';

	i = info.get_config_start();
	if( i == 0 )
	    return false;		// Config bits are never at address zero
	const intelhex::hex_data::address_t end(i + 2*info.numConfigWords());
	for(unsigned j=8; i < end; ++i, ++j)
	    tmp_config[j] = HexData[i];

	unsigned progress(0);
	const unsigned finished(info.is16bit() ? 50 : 25);
	write(CMD_WRITE_CONFIG);	// 16F parts
	write('0');
	write('0');
	progress += 3;
	for( i=0; i<22; ++i, ++progress)
	{
	    write(tmp_config[i]);
	    if( !emit_callback(progress, finished) )	//Emit callback and check for cancellation
		return false;
	}

	read();	// Throw away the ack

	if( info.is16bit() )
	{
	    write(CMD_WRITE_FUSE);		// 18F parts
	    write('0');
	    write('0');
	    progress += 3;
	    for( i=0; i<22; ++i, ++progress)
	    {
		write(tmp_config[i]);
		if( !emit_callback(progress, finished) )	//Emit callback and check for cancellation
		    return false;
	    }
	    read();	//Throw away the ack
	}

	emit_callback(finished, finished);
	return true;
    }

    void kitsrus_t::write_calibration()
    {}

    //Read from a PIC into a hex_data structure
    bool kitsrus_t::read_rom(intelhex::hex_data &HexData)
    {
	const unsigned length = 2*info.rom_size;

	write(CMD_READ_ROM);
	for(unsigned i=0; i < length; ++i)
	{
	    HexData[i] = read();
	    if( !emit_callback(i+1, length) )	//Emit callback and check for cancellation
		return false;
	}
	return true;
    }

    bool kitsrus_t::read_eeprom(intelhex::hex_data &HexData)
    {
	intelhex::hex_data::address_t i(info.get_eeprom_start());
	const intelhex::hex_data::address_t stop(i + info.eeprom_size);

	intelhex::hex_data::address_t j(1);

	write(CMD_READ_EEPROM);
	for(; i<stop; ++i)
	{
	    HexData[i] = read();
	    if( !emit_callback(j++, info.eeprom_size) )	//Emit callback and check for cancellation
		return false;
	}
	return true;
    }

    bool kitsrus_t::read_config(intelhex::hex_data &HexData)
    {
	intelhex::value_type a[26];
	write(CMD_READ_CONFIG);

	uint8_t b = read();	//Throw away the ack
	if(b != 'C')
	    std::cerr << __FUNCTION__ << ": Bad config ack\n\tExpected C got " << b << std::endl;

	for(unsigned i=0; i<26; ++i)
	{
	    a[i] = read();
	    if( !emit_callback(i+1, 26) )	//Emit callback and check for cancellation
		    return false;
	}

	// Store the config bytes
	if( info.is12bit() || info.is14bit() )
	{
	    intelhex::hex_data::address_t j(info.get_id_start());
	    HexData[j++] = a[2];
	    HexData[j++] = a[3];
	    HexData[j++] = a[4];
	    HexData[j++] = a[5];
	}

	intelhex::hex_data::address_t j(info.get_config_start());
	if( j == 0 )	// Config bits are never at address zero
	    return false;
	const intelhex::hex_data::address_t end(j + 2*info.numConfigWords());
	for(unsigned i=0x0A; j < end; ++i, ++j)
	    HexData[j] = a[i];

	return true;
    }

    bool kitsrus_t::erase_chip()
    {
	write(CMD_ERASE);
	uint8_t a = read();
	if( a != 'Y')
	{
	    std::cerr << __FUNCTION__ << ": Bad erase\n\tExpected Y got: " << a << std::endl;
	    return false;
	}
	return true;
    }

    bool kitsrus_t::detect_chip()
    {
	write(CMD_IN_SOCKET);
	if( read() == 'A' )
	{
	    read();
	    return true;
	}
	else
	    return false;
    }

    int kitsrus_t::get_version()
    {
	if(firmware < 0)
	{
	    write(CMD_GET_VERSION);
	    firmware = read();
	}
	return firmware;
    }

    std::string kitsrus_t::get_protocol()
    {
	std::string s;
	write(CMD_GET_PROTOCOL);
	s.push_back(read());
	s.push_back(read());
	s.push_back(read());
	s.push_back(read());
	return s;
    }

    const char *const  kitsrus_t::firmwareName()
    {
	if( (firmware >= 0) && (firmware < (int)(sizeof(firmwareNames)/sizeof(char*))) )
	    return firmwareNames[firmware];
	return NULL;
    }

    // Ugly kludge to work around the K149 reset logic
    //	This function is only used to set the firmware type for reset purposes
    //	Once the programmer has been reset, get_version() should be used to get the real
    //	firmware type
    void kitsrus_t::set_149()
    {
	firmware = KIT_149A;
    }

}	//namespace kitsrus
