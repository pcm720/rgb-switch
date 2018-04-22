/*
Screen-related functions
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

#include "libs/i2c/i2c.h"
#include "libs/u8g2/csrc/u8g2.h"
#include "libs/u8g2/csrc/u8x8.h"
#include "xbm.h"
#include "display.h"

u8g2_t u8g2;
uint8_t u8x8_byte_atmega328p_hw_i2c(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr);
uint8_t u8g2_gpio_and_delay_atmega328p(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr);

void display_init(void){
    #ifndef DISPLAY_180
        u8g2_Setup_sh1106_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_atmega328p_hw_i2c, u8g2_gpio_and_delay_atmega328p);
    #else
        u8g2_Setup_sh1106_i2c_128x64_noname_f(&u8g2, U8G2_R2, u8x8_byte_atmega328p_hw_i2c, u8g2_gpio_and_delay_atmega328p);
    #endif
    u8g2_InitDisplay(&u8g2);        //Send initialization code to the display
    u8g2_ClearDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_SetFont(&u8g2, u8g2_font_6x12_t_symbols);
    u8g2_SetFontRefHeightExtendedText(&u8g2);
    u8g2_SetFontPosTop(&u8g2);
    u8g2_SetFontDirection(&u8g2, 0);
}

void toggle_powerSave(void) {
    powerSave ^= 1;
    u8g2_SetPowerSave(&u8g2, powerSave);
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
        if (PIND & SD_LOS) u8g2_DrawGlyph(&u8g2, 122, 1, 0x2713);
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

uint8_t u8x8_byte_atmega328p_hw_i2c(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr)
{
  switch(msg)
  {
    case U8X8_MSG_BYTE_SEND:
      i2c_write_array((uint8_t *)arg_ptr, arg_int);
      break;
    case U8X8_MSG_BYTE_INIT:
      i2c_init();
      sei();
      break;
    case U8X8_MSG_BYTE_SET_DC:
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      i2c_start(u8x8_GetI2CAddress(u8x8), I2C_WRITE);
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      i2c_stop();
      break;
    default:
      return 0;
  }
  return 1;
}

uint8_t u8g2_gpio_and_delay_atmega328p(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr){
	
	switch(msg){
		case U8X8_MSG_GPIO_AND_DELAY_INIT:
			break;
		case U8X8_MSG_DELAY_MILLI:
			for(uint8_t i = arg_int; i > 0; i--) {
				_delay_ms(1);
			}
			break;
		case U8X8_MSG_DELAY_10MICRO:
			_delay_us(10);
			break;
		default:
			return 0;
		}							
	return 1;
}