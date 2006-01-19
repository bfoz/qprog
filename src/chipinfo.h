/*	Filename: chipinfo.h
	Parse and handle the chipinfo file
	
	Created	March 6, 2005	Brandon Fosdick
	
	Copyright 2005 Brandon Fosdick
	All rights reserved.
	This code is made available to the public under a BSD-like license, a copy of which
	should have been provided with this code in the file LICENSE. For a copy of the BSD 
	license template please visit http://www.opensource.org/licenses/bsd-license.php
*/

#ifndef	CHIPINFO_H
#define	CHIPINFO_H

#include <string>

#include <sys/types.h>

namespace chipinfo
{
	struct chipinfo
	{
		typedef	uint32_t	rom_size_type;
		typedef	uint16_t	eeprom_size_type;
		
		//Core ROM blank values
		#define	BLANK_12BIT   0x0FFF
		#define	BLANK_14BIT   0x3FFF
		#define	BLANK_16BIT   0xFFFF

		#define	EEPROM_START_12BIT	0x2100
		#define	EEPROM_START_14BIT	0x2100
		#define	EEPROM_START_16BIT	0xF0000
		
		#define	CONFIG_START_12BIT	0x2007		
		#define	CONFIG_START_14BIT	0x2007		
		#define	CONFIG_START_16BIT	0x30000

		#define	ID_START_14BIT	0x2000
		#define	ID_START_16BIT	0x0000		//FIXME

		//Core Type Codes	for chipinfo file
		#define	Core16_C	0   // 18F6x2x
		#define	Core16_A	1	 // 18Fx230x330
		#define	Core16_B	2	 // 18Fxx2xx8
		#define	Core14_G	3	 // 16F87, 88
		#define	Core12_A	4	 // 12C50x 12 bit
		#define	Core14_A	5	 // 12C67x, 16C50x, 16Cxxx,
		#define	Core14_B	6	 // 16C8x 16F8x, 16F87x 16F62x
		#define	Core14_C	7	 // 16F7x 16F7x7
		#define	Core14_D	8	 // 12F67x
		#define	Core14_E	9	 // 16F87x-A
		#define	Core14_F	10   // 16F818
		#define	Core12_B	11   // 16F57
		#define	Core10_A	12   // 10Fxxx

		chipinfo() : fast_power(false) {}

		std::string	name;						//Chip name
		uint16_t	chip_id;
		rom_size_type	rom_size;			//Number of ROM words
		eeprom_size_type	eeprom_size; 	  //EEPROM size in bytes
		uint16_t	fuse_blank;
		uint32_t	rom_blank;		//Value of a blank ROM word
		uint8_t	program_delay;
		uint8_t	erase_mode;
		uint8_t	power_sequence;
		uint8_t	program_tries;
		uint8_t	core_type;
		uint8_t	over_program;
		bool	cal_word;
		bool	band_gap;
		bool	fast_power;
		bool	single_panel;

		bool	set(std::string, std::string);
		
		bool	is14bit();
		bool	is16bit();
		
		uint32_t	get_eeprom_start();
		uint32_t	get_config_start();
		uint32_t	get_id_start();
		uint32_t	get_blank_value();
	};

}

#endif	//CHIPINFO_H
