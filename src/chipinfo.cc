/*	Filename: chipinfo.cc
	Parse and handle the chipinfo file
	
	Created	March 6, 2005	Brandon Fosdick
	
	Copyright 2005 Brandon Fosdick
	All rights reserved.
	This code is made available to the public under a BSD-like license, a copy of which
	should have been provided with this code in the file LICENSE. For a copy of the BSD 
	license template please visit http://www.opensource.org/licenses/bsd-license.php

	$Id: chipinfo.cc,v 1.6 2008/04/01 04:07:45 bfoz Exp $
*/

#include <iostream>

#include "chipinfo.h"

namespace chipinfo
{
	const std::string	key_ConfigWordDescriptions("ConfigWordDescriptions");
	
	bool chipinfo::set(std::string key, std::string value)
	{
		if( (key=="CHIPname") || (key=="Name") )
			name = value;
		else if(key=="INCLUDE") {}
		else if(key=="SocketImage") {}
		else if(key=="EraseMode")
			erase_mode = strtol(value.c_str(), NULL, 10);
		else if(key=="FlashChip") {}
	else if( key=="FastPowerSequence" )
	    fast_power = (value=="1");
		else if(key=="PowerSequence")
		{
			if(value=="Vcc")
				power_sequence = 0;
			else if(value=="VccVpp1")
				power_sequence = 1;
			else if(value=="VccVpp2")
				power_sequence = 2;
			else if(value=="Vpp1Vcc")
				power_sequence = 3;
			else if(value=="Vpp2Vcc")
				power_sequence = 4;
		}
		else if(key=="ProgramDelay")
			program_delay = strtol(value.c_str(), NULL, 10);
		else if(key=="ProgramTries")
			program_tries = strtol(value.c_str(), NULL, 10);
		else if(key=="OverProgram")
			over_program = strtol(value.c_str(), NULL, 10);
		else if(key=="CoreType")
		{
			if(value=="bit16_C")		// 18F6x2x
			{
				core_type = Core16_C;
				rom_blank = BLANK_16BIT;
			}
			if(value=="bit16_A")		// 18Fx230x330
			{
				core_type = Core16_A;
				rom_blank = BLANK_16BIT;
			}
			if(value=="bit16_B")		// 18Fxx2xx8
			{
				core_type = Core16_B;
				rom_blank = BLANK_16BIT;
			}
			if(value=="bit12_A")		// 12C50x 12 bit
			{
				core_type = Core12_A;
				rom_blank = BLANK_12BIT;
			}
			if(value=="bit14_A")		// 12C67x, 16C50x, 16Cxxx
			{
				core_type = Core14_A;
				rom_blank = BLANK_14BIT;
			}
			if(value=="bit14_B")		// 16C8x 16F8x, 16F87x 16F62x
			{
				core_type = Core14_B;
				rom_blank = BLANK_14BIT;
			}
			if(value=="bit14_C")		// 16F7x 16F7x7
			{
				core_type = Core14_C;
				rom_blank = BLANK_14BIT;
			}
			if(value=="bit14_D")		// 12F67x
			{
				core_type = Core14_D;
				rom_blank = BLANK_14BIT;
			}
			if(value=="bit14_E")		// 16F87x-A
			{
				core_type = Core14_E;
				rom_blank = BLANK_14BIT;
			}
			if(value=="bit14_F")		// 16F818
			{
				core_type = Core14_F;
				rom_blank = BLANK_14BIT;
			}
			if(value=="bit14_G")		// 16F87, 88
			{
				core_type = Core14_G;
				rom_blank = BLANK_14BIT;
			}
			if(value=="bit14_H")		// 10Fxxx
			{
				core_type = Core10_A;
				rom_blank = BLANK_12BIT;
			}
			if(value=="bit12_B")		// 16F57
			{
				core_type = Core12_B;
				rom_blank = BLANK_12BIT;
			}
			
			if(core_type == Core16_A)	//Set iff Core16_A
				single_panel = true;
			else
				single_panel = false;
		}
		else if(key=="FUSEblank")
			fuse_blank = strtol(value.c_str(), NULL, 16);
		else if(key=="CPwarn") {}
		else if(key=="CALword")
		{
			if(value == "Y")
				cal_word = true;
			else
				cal_word = false;
		}
		else if(key=="BandGap")
		{
			if(value == "Y")
				band_gap = true;
			else
				band_gap = false;
		}
		else if(key=="ICSPonly") {}
		else if(key=="ChipID")
			chip_id = strtol(value.c_str(), NULL, 16);
		else if(key=="Type") {}
		else if(key=="ChipID1")	{}
		else if(key=="FlashROM")	{}
		else if(key=="NumConfigWords")
			num_config_words = strtol(value.c_str(), NULL, 10);
		else if(key=="NumEEPROMBytes")
			eeprom_size = strtol(value.c_str(), NULL, 10);
		else if(key=="NumPayloadBits")	{}
		else if(key=="NumPayloadCommandBits")	{}
		else if(key=="NumROMWords")
			rom_size = strtoul(value.c_str(), NULL, 10);
		else if(key=="SocketImageType")	{}
		else if(key=="Status")	{}
		else if( equal(key_ConfigWordDescriptions.begin(), key_ConfigWordDescriptions.end(), key.begin()) )	{}
		else if ( (key=="ID") || 	// Keys to ignore
			  (key=="CreateTimeStamp") 
			) {}
		else
		{
			std::cout << "Unrecognized key: " << key << " => " << value << std::endl;
			return false;
		}
		return true;
	}

	bool	chipinfo::is14bit()
	{
		switch(core_type)
		{
			case Core14_G:
			case Core14_A:
			case Core14_B:
			case Core14_C:
			case Core14_D:
			case Core14_E:
			case Core14_F:
				return true;
			default:
				return false;
		}
	}
	
	bool	chipinfo::is16bit()
	{
		switch(core_type)
		{
			case Core16_C:
			case Core16_A:
			case Core16_B:
				return true;
			default:
				return false;
		}
	}
		
	uint32_t	chipinfo::get_eeprom_start()
	{
		if( (core_type==Core16_C) || (core_type==Core16_A) || (core_type==Core16_B))
			return EEPROM_START_16BIT;
		else
			return EEPROM_START_14BIT;
	}

    uint32_t	chipinfo::get_config_start()
    {
	if( (core_type==Core12_A) || (core_type==Core12_B) )
	    return CONFIG_START_12BIT;
	else if( (core_type==Core16_C) || (core_type==Core16_A) || (core_type==Core16_B) )
	    return CONFIG_START_16BIT;
	else
	    return CONFIG_START_14BIT;
    }

    uint32_t	chipinfo::get_id_start()
    {
	if( is12bit() )
	    return rom_size;	// ID locations immediately follow program words
	else if( is14bit() )
	    return ID_START_14BIT;
	else if( is16bit() )
	    return ID_START_16BIT;
	else
	    return 0;	//FIXME
    }

	uint32_t	chipinfo::get_blank_value()
	{
		if( (core_type==Core16_C) || (core_type==Core16_A) || (core_type==Core16_B))
			return BLANK_16BIT;
		else if( (core_type==Core12_A) || (core_type==Core12_B) || (core_type==Core10_A) )
			return BLANK_12BIT;
		else
			return BLANK_14BIT;
	}

}
