/* Filename: kitsrus.cc
	Interface to Tony Nixon's PIC Programmers via the Kitsrus protocol
	This works for Kits 128, 149A, 149B, 150, 182

	Copyright (c) 2005, Terran Development Corporation
	All rights reserved.
	This code is made available to the public under a BSD-like license, a copy of which
	should have been provided with this code in the file LICENSE. For a copy of the BSD 
	license template please visit http://www.opensource.org/licenses/bsd-license.php

	$Id: kitsrus.cc,v 1.10 2008/03/12 03:27:58 bfoz Exp $
 * */
#include <fcntl.h>
#include <iostream>

#include "kitsrus.h"
#include "intelhex.h"

namespace kitsrus
{
#ifndef	Q_WS_WIN
	#define	HIBYTE(a)	(uint8_t)(a>>8)
	#define	LOBYTE(a)	(uint8_t)(a&0x00FF)
#endif	//Q_WS_WIN

	//Switch from power-on mode to command mode
	bool kitsrus_t::command_mode()
	{
		write('P');
		if( read() == 'P' )
			return true;
		else
			return false;
	}

	//Do a soft reset of the device
	//Send a 1 to the device. If it is in the command table it will reset. Either way it should return 'Q'
	bool kitsrus_t::soft_reset()
	{
		//Send a 1 to the device.
		// If it is in the command table it will reset. Either way it should return 'Q'
		write(CMD_RESET);
		if((read()) == 'Q')
			return true;
		else
			return false;
	}

	//Do a hard reset of the device
	bool kitsrus_t::hard_reset()
	{
		set_dtr();		//Set DTR high
#ifdef	Q_WS_WIN	//Deal with win32 stupidity
		Sleep(100);
#else
		usleep(10);		//Delay
#endif
		clear_dtr();	//Set DTR low

		if( read()=='B' )
		{
			read();	//Ignore the firmware type
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
//		uint8_t	c;
		uint8_t i;
		intelhex::hex_data::address_t	j(0);
		uint16_t k;
		intelhex::hex_data::size_type size;
		
		//Figure out how many ROM bytes need to be written
		size = (1 + HexData.max_addr_below(info.rom_size-1))*2;
//		std::cout << __FUNCTION__ << ": size = " << size << std::endl;

		//Send program rom command
		write(CMD_WRITE_ROM);
		write( (size & 0xFF00) >> 8);  //Send size hi
		write(size & 0x00FF); //Send size low

		while(1)
		{
			switch(read())
			{
				case 'P':
//					std::cout << ".\n";
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
					for(i=0; i<(32/2); ++i)
					{
//					std::cout << __FUNCTION__ << ": Pre get\n";
						const intelhex::hex_data::element_t a = HexData.get(j, info.get_blank_value());
//					std::cout << __FUNCTION__ << ": " << std::hex << a << " @ " << std::hex << j << "\n";
						write( (a & 0xFF00) >> 8);
						write( a & 0x00FF);
						++j;
					}
					if( !emit_callback((j>size)?size:j,size) )	//Emit callback and check for cancellation
						return false;
//					std::cout << "." << std::flush;
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
//		uint8_t	c;
//		uint8_t i;
		intelhex::hex_data::address_t	j(info.get_eeprom_start());
		intelhex::hex_data::address_t	eeprom_start(info.get_eeprom_start());
		intelhex::hex_data::address_t	eeprom_end;
		uint16_t progress;
		intelhex::hex_data::size_type size;
		
		//Ideally we would figure out how many ROM words are going to be written
		//	and then write only that. But, to make things simpler we'll just write
		//	to all of the ROM.
#if defined(WRITE_EEPROM_DEBUG)
		std::cout << __FUNCTION__ << ": eeprom_start = " << std::hex << j << std::endl;
		std::cout << __FUNCTION__ << ": eeprom_size = " << std::hex << info.eeprom_size << std::endl;
#endif
		size = HexData.size_in_range(j, info.get_eeprom_start() + info.eeprom_size);
		//Make size an even number
		if( (size % 2) != 0 )
			++size;
		eeprom_end = j + size - 1;
#if defined(WRITE_EEPROM_DEBUG)
		std::cout << __FUNCTION__ << ": size = " << size << std::endl;
		std::cout << __FUNCTION__ << ": eeprom_end = " << std::hex << eeprom_end << std::endl;
#endif

		//Send program rom command
		write(CMD_WRITE_EEPROM);
		write( (size & 0xFF00) >> 8);  //Send size hi
		write(size & 0x00FF); //Send size low

		while(1)
		{
			switch(read())
			{
				case 'P':
//					std::cout << ".\n";
					emit_callback((progress>size)?size:progress,size);
					return true;
				case 'Y':			
#if !defined(WRITE_EEPROM_DEBUG)
//					std::cout << "." << std::flush;
#endif
					write( HexData.get(j, 0xFF) & 0x00FF);
#if defined(WRITE_EEPROM_DEBUG)
					std::cout << __FUNCTION__ << ": wrote " << std::hex << HexData[j] << "\n";
#endif
					++j;
					write( HexData.get(j, 0xFF) & 0x00FF);
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
		
//		std::cout << __FUNCTION__ << std::endl;
		
		intelhex::hex_data::address_t	i;
		//If the ID bits were specified use them
		//	otherwise use blanks
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
		const intelhex::hex_data::address_t end(i + info.numConfigWords());
		for(unsigned j=8; i < end; ++i, j+=2)
		{
			if( !HexData.isset(i) )
				continue;
			tmp_config[j] = HexData[i] & 0x00FF;
			tmp_config[j+1] = (HexData[i] & 0xFF00) >> 8;
		}

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
//		std::cout << __FUNCTION__ << "1" << std::endl;
		read();	//Throw away the ack
//		std::cout << __FUNCTION__ << "2" << std::endl;

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
		intelhex::hex_data::element_t a;
		
		write(CMD_READ_ROM);
//		std::cout << "About to read " << info.rom_size << " words\n";
		for(unsigned i=0; i<info.rom_size; ++i)
		{
//			std::cout << "\r" << i;
			a = (read() << 8) & 0xFF00;
			a |= read() & 0x00FF;
			HexData[i] = a;
			if( !emit_callback(i+1, info.rom_size) )	//Emit callback and check for cancellation
				return false;
		}

//		std::cout << "\r";
//		std::cout << "\nRead " << info.rom_size << " words\n";
		return true;
	}

	bool kitsrus_t::read_eeprom(intelhex::hex_data &HexData)
	{
		intelhex::hex_data::address_t i(info.get_eeprom_start());
		const intelhex::hex_data::address_t stop(i + info.eeprom_size);

//		intelhex::hex_data::element_t a;
		intelhex::hex_data::address_t j(1);

//		std::cout << __FUNCTION__ << ": eeprom_start = " << std::hex << i << std::endl;
//		std::cout << __FUNCTION__ << ": eeprom_stop = " << std::hex << stop << std::endl;
//		std::cout << __FUNCTION__ << ": eeprom_size = " << std::hex << info.eeprom_size << std::endl;

		write(CMD_READ_EEPROM);
		for(; i<stop; ++i)
		{
			HexData[i] = read();
			if( !emit_callback(j++, info.eeprom_size) )	//Emit callback and check for cancellation
				return false;
//			std::cout << __FUNCTION__ << ": HexData[" << std::hex << i << "] = " << std::hex << HexData[i] << std::endl;
//			a = read();
//			std::cout << __FUNCTION__ << ": j = " << std::hex << (j++) << "\t" << std::hex << a << "\n";
		}
		return true;
	}

	bool kitsrus_t::read_config(intelhex::hex_data &HexData)
	{
		intelhex::hex_data::element_t	a[26];
		write(CMD_READ_CONFIG);
		
		uint8_t b = read();	//Throw away the ack
		if(b != 'C')
			std::cerr << __FUNCTION__ << ": Bad config ack\n\tExpected C got " << b << std::endl;
//		else
//			std::cout << __FUNCTION__ << ": Got ack\n";

		for(unsigned i=0; i<26; ++i)
		{
			a[i] = read();
			if( !emit_callback(i+1, 26) )	//Emit callback and check for cancellation
				return false;
//			std::cout << __FUNCTION__ << ": read " << std::hex << a[i] << "\n";
		}
		
		if(info.is14bit())
		{
			HexData[0x2000] = a[2];
			HexData[0x2001] = a[3];
			HexData[0x2002] = a[4];
			HexData[0x2003] = a[5];
		}

		intelhex::hex_data::address_t j(info.get_config_start());
		if( j == 0 )	// Config bits are never at address zero
			return false;
		const intelhex::hex_data::address_t end(j + info.numConfigWords());
		for(unsigned i=0x0A; j < end; i+=2, ++j)
		{
			HexData[j] = (a[i+1] << 8) | a[i];
		}

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
//		else
//			std::cout << __FUNCTION__ << ": Got Y\n";
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

	std::string kitsrus_t::kit_name()
	{
		switch(firmware)
		{
			case KIT_128:
				return "Kit 128";
			case KIT_149A:
				return "Kit 149A";
			case KIT_149B:
				return "Kit 149B";
			case KIT_150:
				return "Kit 150";
			case KIT_170:
				return "Kit 170";
			case KIT_182:
				return "Kit 182";
			case KIT_185:
				return "Kit 185";
			default:
				return "Unknown";
		}
	}

	//Ugly kludge to work around the K149 reset logic
	//	This function is only used to set the firmware type for reset purposes
	//	Once the programmer has been reset, get_version() should be used to get the real
	//		firmware type
	void kitsrus_t::set_149()
	{
		firmware = KIT_149A;
	}

}	//namespace pocket
