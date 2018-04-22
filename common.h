/*
Main definitions file for RGB Switcher
    Copyright (C) 2018 pcm720 <pcm720@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see 
<http://www.gnu.org/licenses/>.
*/

#ifndef DEFINES_H_
#define DEFINES_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// inputs on PORT D
#define DISPLAY_180
#ifndef DISPLAY_180
	#define B_SC1 0x1E // ~0 & ~0xE0
	#define B_SC2 0x1D // ~1 & ~0xE0
	#define B_SC3 0x17 // ~3 & ~0xE0
	#define B_SC4 0x0F // ~4 & ~0xE0
#else
	#define B_SC1 0x0F
	#define B_SC2 0x17
	#define B_SC3 0x1D
	#define B_SC4 0x1E
#endif
#define SD_LOS 0x20 // MAX7461 LOS#

// special inputs
#define OFF 0x4
#define NOT_SELECTED 0xFF

// switch control pins on PORT C
#define SW_C1 0x0
#define SW_C2 0x1
#define SW_C3 0x2
#define SW_C4 0x3

// special draw states
#define SYNC_SEARCH 0x10
#define SYNC_LOST 0x11
#define EEPROM_READ_FAILED 0xF1
#define EEPROM_WRITE_FAILED 0xF2

extern volatile uint8_t activeButton;
extern volatile uint8_t offCounter;
extern volatile uint8_t powerSave;

typedef struct {
    uint8_t autoSwitchingEnabled;
    uint8_t DisableOnLOS;
    uint8_t defaultInput;
    uint8_t checksum;
} switchOptions;

#endif