/*
RGB Switcher code for ATmega328P
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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <util/atomic.h>
#include "libs/u8g2-hal/hal.h"
#include "extra/xbm/xbm.h"

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

u8g2_t u8g2;
volatile uint8_t activeButton = NOT_SELECTED;
volatile uint8_t syncStatus = 0;
volatile uint8_t offCounter = 0;
volatile uint8_t powerSave = 0;

typedef struct {
    uint8_t autoSwitchingEnabled;
    uint8_t DisableOnLOS;
    uint8_t defaultInput;
    uint8_t checksum;
} switchOptions;

void init(switchOptions* options);
void toggle_powerSave(void);
void options_menu(switchOptions* options);

uint8_t switch_output(uint8_t out);
uint8_t auto_switch(void);
void manual_switch(uint8_t currentInput);

void display_init(void);
void draw(uint8_t input);
void draw_options(const switchOptions* options);

uint8_t eeprom_read_state(switchOptions* options);
uint8_t eeprom_write_state(switchOptions* options);
inline uint8_t get_checksum(switchOptions* options);

int main(void) {
	
	switchOptions options;
	init(&options);

	long sleep_timer = 0;
	uint8_t currentInput = options.defaultInput;
	uint8_t autoSwitchingState = options.autoSwitchingEnabled;
	uint8_t syncState = 0;
	uint8_t sdDisable = 0; // disables sync detection functionality

	activeButton = NOT_SELECTED;
	if (autoSwitchingState == 0) manual_switch(currentInput);
	else draw(SYNC_SEARCH);

	while(1) {
		if ((sleep_timer == 0xFFFFFF) && !powerSave) {
			toggle_powerSave();
		} else if (!powerSave) {
			sleep_timer++;

			if (offCounter > 10) {
				sleep_timer = 0;
				options_menu(&options);
				draw(currentInput);
				_delay_ms(500);
			} else {
				offCounter = 0;
			}

			if (!syncStatus) {
				if (options.autoSwitchingEnabled && autoSwitchingState && !sdDisable) {
					currentInput = auto_switch();
					if (currentInput != NOT_SELECTED) {
						draw(currentInput);
						autoSwitchingState = 0;
					} else {
						draw(SYNC_SEARCH);
						sleep_timer = 0;
					}
				}
				if (options.DisableOnLOS && syncState) {
						currentInput = OFF;
						switch_output(currentInput);
						draw(SYNC_LOST);
						_delay_ms(2000);
						if (!options.autoSwitchingEnabled) draw(currentInput);
						autoSwitchingState = 1;
						syncState = 0;
						sleep_timer = 0;
				}
			} else if (syncStatus) syncState = 1;
			if (!autoSwitchingState) draw(currentInput); // update screen

			if ((activeButton != NOT_SELECTED) && (offCounter == 0)) {
				sleep_timer = 0;
				sdDisable = 1;
				currentInput = activeButton;
				manual_switch(currentInput);
				activeButton = NOT_SELECTED;
			}
		} else sleep_timer = 0;
	}
}

void init(switchOptions* options) {
	DDRB = 0x20; // enable SCK LED
	DDRD = 0x00; // set PD to input
	DDRC = ~0x0F; // initialize switch control pins as inputs
	PORTD = 0x00; // disable pull-ups
	PORTC = 0x00; // disable pull-ups

	display_init(); // interrupts are enabled during TWI initialization

	if (eeprom_read_state(options)) {     // read config from EEPROM
        options->autoSwitchingEnabled = 1; // reading failed, revert to default parameters
        options->DisableOnLOS = 1;
		options->defaultInput = OFF; 
		options->checksum = 0x0;
    	eeprom_write_state(options); // try to save defaults
    }

	PCMSK2 = 0x3B; // prepare interrupts for every input pin except PD2
	EICRA |= 0x00; // set INT0 to trigger when PD2 is low
	PCICR = 0x4; // enable PCINT for PORTD
	EIMSK |= 0x01; // enable INT0
}

void toggle_powerSave(void) {
	PORTB ^= 0x20;
	powerSave ^= 1;
	u8g2_SetPowerSave(&u8g2, powerSave);
	_delay_ms(100);
	PORTB ^= 0x20;
}

void options_menu(switchOptions* options) {
    uint8_t stateChanged = 0;
    uint8_t loop = 1;

	activeButton = NOT_SELECTED;
	draw_options(options);
	offCounter = 0;
    while(loop) {
		draw_options(options);
        switch(activeButton) {
            case (SW_C1): // auto switching
				options->autoSwitchingEnabled ^= 0x1;
				stateChanged = 1;
                break;
            case (SW_C2): // auto disable on LOS
				options->DisableOnLOS ^= 0x1;
				stateChanged = 1;
                break;
            case (SW_C3): // default input
				options->defaultInput++;
				if (options->defaultInput > 4) options->defaultInput = 0;
				stateChanged = 1;
                break;
            case (OFF): // exit menu
				loop = 0;
                break;
            default:
                break;
        }
		offCounter = 0;
		activeButton = NOT_SELECTED;
    }
	if (stateChanged) eeprom_write_state(options);
}

/*-------------------------------------------------
				Switching logic
-------------------------------------------------*/

uint8_t switch_output(uint8_t out) {
	ATOMIC_BLOCK(ATOMIC_FORCEON) { //disable interrupts
		if ((DDRC & ~0xF0) != (1 << out)) { // check that selected switch is not already enabled
			DDRC &= ~0x0F; // disable all switches on PORT C
			_delay_ms(5);
			DDRC |= (1 << out);
			_delay_ms(5);
			return 1;
		} else return 0;
	}
}

uint8_t auto_switch(void) {
	uint8_t activeInput = NOT_SELECTED;
    for (uint8_t i = 0; i < 4; i++) {
        switch_output(i);
        _delay_ms(15);
        if (syncStatus == 1) {
			activeInput = i;
			i = 5;
		}
    }
	return activeInput;
}

void manual_switch(uint8_t currentInput) {
	uint8_t status = 0;
	ATOMIC_BLOCK(ATOMIC_FORCEON) { //disable interrupts
		if (currentInput != OFF) {
			status = switch_output(currentInput);
		} else {
			DDRC &= ~0x0F;
			status = 1;
		}

		if (status) {
			draw(currentInput);
		}
	}
}

/*-------------------------------------------------
			Screen-related functions
-------------------------------------------------*/

void display_init(void){
	#ifndef DISPLAY_180
		u8g2_Setup_sh1106_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_atmega328p_hw_i2c, u8g2_gpio_and_delay_atmega328p);
	#else
		u8g2_Setup_sh1106_i2c_128x64_noname_f(&u8g2, U8G2_R2, u8x8_byte_atmega328p_hw_i2c, u8g2_gpio_and_delay_atmega328p);
    #endif
	u8g2_InitDisplay(&u8g2);		//Send initialization code to the display
	u8g2_ClearDisplay(&u8g2);
	u8g2_SetPowerSave(&u8g2, 0);
	u8g2_SetFont(&u8g2, u8g2_font_6x12_t_symbols);
	u8g2_SetFontRefHeightExtendedText(&u8g2);
	u8g2_SetFontPosTop(&u8g2);
	u8g2_SetFontDirection(&u8g2, 0);
}

void draw(uint8_t input) {
	PCICR &= 0xFB; // disable interrupts
	EIMSK &= 0xFE;
	u8g2_SetDrawColor(&u8g2, 1);
	u8g2_ClearBuffer(&u8g2);

	switch(input) {
		case SW_C1:
			u8g2_DrawStr(&u8g2, 31, 53, "PlayStation");
			u8g2_DrawXBMP(&u8g2, 38, 14, PS_width, PS_height, PS_bits);
			u8g2_DrawBox(&u8g2,1,1,128,11);
			u8g2_SetDrawColor(&u8g2, 0);
			u8g2_DrawStr(&u8g2, 1, 1, "Input 1");
			break;
			
		case SW_C2:
			u8g2_DrawStr(&u8g2, 37, 53, "Dreamcast");
			u8g2_DrawXBMP(&u8g2, 40, 14, DC_width, DC_height, DC_bits);
			u8g2_DrawBox(&u8g2,1,1,128,11);
			u8g2_SetDrawColor(&u8g2, 0);
			u8g2_DrawStr(&u8g2, 1, 1, "Input 2");
			break;
			
		case SW_C3:
			u8g2_DrawStr(&u8g2, 31, 53, "Nintendo 64");
			u8g2_DrawXBMP(&u8g2, 44, 14, N64_width, N64_height, N64_bits);
			u8g2_DrawBox(&u8g2,1,1,128,11);
			u8g2_SetDrawColor(&u8g2, 0);
			u8g2_DrawStr(&u8g2, 1, 1, "Input 3");
			break;
			
		case SW_C4:
			u8g2_DrawStr(&u8g2, 34, 53, "Mega Drive");
			u8g2_DrawBox(&u8g2,1,1,128,11);
			u8g2_SetDrawColor(&u8g2, 0);
			u8g2_DrawXBMP(&u8g2, 39, 14, MD_width, MD_height, MD_bits);
			u8g2_DrawStr(&u8g2, 1, 1, "Input 4");
			break;

		default: // OFF
			u8g2_DrawStr(&u8g2, 4, 24, "Hold OFF for options");
			u8g2_DrawBox(&u8g2,1,1,128,11);
			u8g2_SetDrawColor(&u8g2, 0);
			u8g2_DrawStr(&u8g2, 13, 1, "No input selected");
			break;

		case SYNC_SEARCH:
			u8g2_DrawStr(&u8g2, 4, 24, "Searching for signal");
			u8g2_DrawBox(&u8g2,1,1,128,11);
			u8g2_SetDrawColor(&u8g2, 0);
			u8g2_DrawStr(&u8g2, 22, 1, "No sync signal");
			break;
			
		case SYNC_LOST:
			u8g2_DrawStr(&u8g2, 31, 24, "Signal lost");
			u8g2_DrawBox(&u8g2,1,1,128,11);
			u8g2_SetDrawColor(&u8g2, 0);
			u8g2_DrawStr(&u8g2, 22, 1, "No sync signal");
			break;
			
		case EEPROM_READ_FAILED:
			u8g2_DrawStr(&u8g2, 7, 24, "EEPROM Read Failed!");
			u8g2_DrawBox(&u8g2,1,1,128,11);
			u8g2_SetDrawColor(&u8g2, 0);
			u8g2_DrawStr(&u8g2, 4, 1, "An error has occured");
			break;
			
		case EEPROM_WRITE_FAILED:
			u8g2_DrawStr(&u8g2, 4, 53, "EEPROM Write Failed!");
			u8g2_DrawBox(&u8g2,1,1,128,11);
			u8g2_SetDrawColor(&u8g2, 0);
			u8g2_DrawStr(&u8g2, 4, 1, "An error has occured");
			break;
		
	}

	if (input < OFF) { // Display sync status for enabled input
		u8g2_DrawStr(&u8g2, 92, 1, "Sync: ");
		if (syncStatus) u8g2_DrawGlyph(&u8g2, 122, 1, 0x2713);
		else u8g2_DrawGlyph(&u8g2, 122, 1, 0x2715);
	}

	u8g2_SendBuffer(&u8g2);
	PCICR |= 0x04; // enable interrupts
	EIMSK |= 0x01;
}

void draw_options(const switchOptions* options){
	PCICR &= 0xFB;  // disable interrupts
	EIMSK &= 0xFE;

	u8g2_ClearBuffer(&u8g2);
	u8g2_SetDrawColor(&u8g2, 1);
	u8g2_DrawBox(&u8g2,1,1,128,11);
	u8g2_DrawStr(&u8g2, 1, 12, "1. Auto Switching: ");                                                                                                                                                                                                                                                               
	switch (options->autoSwitchingEnabled) {
		case 0:
			u8g2_DrawGlyph(&u8g2, 116, 12, 0x2715);
			break;
		case 1:
			u8g2_DrawGlyph(&u8g2, 116, 12, 0x2713);
			break;
	}
	u8g2_DrawStr(&u8g2, 1, 24, "2. Disable on LOS: ");
	switch (options->DisableOnLOS) {
		case 0:
			u8g2_DrawGlyph(&u8g2, 116, 24, 0x2715);
			break;
		case 1:
			u8g2_DrawGlyph(&u8g2, 116, 24, 0x2713);
			break;
	}
	u8g2_DrawStr(&u8g2, 1, 36, "3. Default Input: ");
	switch (options->defaultInput) {
		case SW_C1:
			u8g2_DrawStr(&u8g2, 116, 36, "1");
			break;
		case SW_C2:
			u8g2_DrawStr(&u8g2, 116, 36, "2");
			break;
		case SW_C3:
			u8g2_DrawStr(&u8g2, 116, 36, "3");
			break;
		case SW_C4:
			u8g2_DrawStr(&u8g2, 116, 36, "4");
			break;		
		case OFF:
			u8g2_DrawGlyph(&u8g2, 116, 36, 0x2715);
			break;
	}
	u8g2_DrawStr(&u8g2, 4, 48, "OFF to save and exit");
	u8g2_SetDrawColor(&u8g2, 0);
	u8g2_DrawStr(&u8g2, 28, 1, "Options Menu");
	u8g2_SendBuffer(&u8g2);

	if (offCounter > 10) _delay_ms(1000); // to keep interrupt from triggering before user has the time to react
	PCICR |= 0x04;  // enable interrupts
	EIMSK |= 0x01;
}

/*-------------------------------------------------
				EEPROM operations
-------------------------------------------------*/

uint8_t eeprom_read_state(switchOptions* options) {
    eeprom_read_block((void*)options, (void*)0, sizeof(*options));
    if (get_checksum(options) == options->checksum) return 0; // read successful
    else {
		draw(EEPROM_READ_FAILED);
		_delay_ms(5000);
		return 1; // read failed
	}
}

uint8_t eeprom_write_state(switchOptions* options) {
    options->checksum = get_checksum(options);
    eeprom_update_block((const void*)options, (void*)0, sizeof(*options));
    if (eeprom_read_state(options)) {
		draw(EEPROM_WRITE_FAILED);
		_delay_ms(5000);
		return 1; // write failed
	} else return 0; // write successful
}

inline uint8_t get_checksum(switchOptions* options) {
    return options->autoSwitchingEnabled + 
           options->DisableOnLOS + 
           options->defaultInput;
}

/*-------------------------------------------------
				Interrupt vectors
-------------------------------------------------*/

ISR(INT0_vect) { // OFF button interrupt
	uint8_t inputs = PIND;
	if (powerSave) {
		toggle_powerSave();
	} else if (~inputs & 0x4) {
		if (offCounter > 10) {
			activeButton = NOT_SELECTED;
			EIMSK = 0x00; // disable INT0, will be enabled later by options_menu draw functions
		} else {
			offCounter++;
			if (offCounter < 2) activeButton = OFF;
		}
	}
	_delay_ms(100);
}

ISR(PCINT2_vect) {
	uint8_t inputs = PIND;
	if (powerSave) {
		toggle_powerSave();
	} else {
		switch(inputs & 0x1F) {
			case (B_SC1):
				activeButton = SW_C1;
				break;
			case (B_SC2):
				activeButton = SW_C2;
				break;
			case (B_SC3): 
				activeButton = SW_C3;
				break; 
			case (B_SC4):
				activeButton = SW_C4;
				break;
			default:
				activeButton = NOT_SELECTED;
				break;
		}
		if (inputs & SD_LOS) syncStatus = 1;
		else syncStatus = 0;
	}
   	_delay_ms(100);
}