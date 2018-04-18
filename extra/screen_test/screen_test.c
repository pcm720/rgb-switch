#include <avr/io.h>
#include <util/delay.h>
#include "../../libs/u8g2-hal/hal.h"
#include "../xbm/xbm.h"

#define false 0
#define true 1

// inputs on PORT D
#define B_SC1 0x1E // ~0 & ~0xE0
#define B_SC2 0x1D // ~1 & ~0xE0
#define B_SC3 0x17 // ~3 & ~0xE0
#define B_SC4 0x0F // ~4 & ~0xE0
#define B_OFF 0x1B // ~2 & ~0xE0
#define SD_LOS 5

// switch control pins on PORT C
#define SW_C1 0
#define SW_C2 1
#define SW_C3 2
#define SW_C4 3

// special inputs
#define OFF 20
#define NOT_SELECTED 0xFF

u8g2_t u8g2;

void u8g2_prepare(void) {
	u8g2_SetFont(&u8g2, u8g2_font_6x12_t_symbols);
	u8g2_SetFontRefHeightExtendedText(&u8g2);
	u8g2_SetDrawColor(&u8g2, 1);
	u8g2_SetFontPosTop(&u8g2);
	u8g2_SetFontDirection(&u8g2, 0);
}

void draw_logo(uint8_t logo) {
	switch(logo) {
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
	}
	
}

void no_input(void) {
	u8g2_DrawStr(&u8g2, 13, 32, "No input selected");
}

void options_screen(uint8_t* autoSwitchingEnabled, uint8_t* displayRotation, uint8_t* autoDisableOnLOS, uint8_t* defaultInput){
	u8g2_ClearBuffer(&u8g2);
	u8g2_prepare();
	u8g2_DrawBox(&u8g2,1,1,128,11);
	u8g2_DrawStr(&u8g2, 1, 12, "1. Auto Switching: ");
	switch (*autoSwitchingEnabled) {
		case 0:
			u8g2_DrawGlyph(&u8g2, 116, 12, 0x2715);
			break;
		case 1:
			u8g2_DrawGlyph(&u8g2, 116, 12, 0x2713);
			break;
	}
	u8g2_DrawStr(&u8g2, 1, 24, "2. Rotation: ");
	switch (*displayRotation) {
		case 0:
			u8g2_DrawStr(&u8g2, 116, 24, "0");
			break;
		case 1:
			u8g2_DrawStr(&u8g2, 110, 24, "90");
			break;
	}
	u8g2_DrawStr(&u8g2, 1, 36, "3. Disable on LOS: ");
	switch (*autoDisableOnLOS) {
		case 0:
			u8g2_DrawGlyph(&u8g2, 116, 36, 0x2715);
			break;
		case 1:
			u8g2_DrawGlyph(&u8g2, 116, 36, 0x2713);
			break;
	}
	u8g2_DrawStr(&u8g2, 1, 48, "4. Default Input: ");
	switch (*defaultInput) {
		case SW_C1:
			u8g2_DrawStr(&u8g2, 116, 48, "1");
			break;
		case SW_C2:
			u8g2_DrawStr(&u8g2, 116, 48, "2");
			break;
		case SW_C3:
			u8g2_DrawStr(&u8g2, 116, 48, "3");
			break;
		case SW_C4:
			u8g2_DrawStr(&u8g2, 116, 48, "4");
			break;		
	}
	u8g2_SetDrawColor(&u8g2, 0);
	u8g2_DrawStr(&u8g2, 28, 1, "Options Menu");
	u8g2_SendBuffer(&u8g2);
}

void draw(uint8_t input) {
	u8g2_SetPowerSave(&u8g2, 0);
	u8g2_ClearBuffer(&u8g2);
	u8g2_prepare();
	if (input != OFF && input != 0xFF) { 
		draw_logo(input);
		u8g2_DrawStr(&u8g2, 92, 1, "Sync: ");
		//u8g2_DrawGlyph(&u8g2, 122, 1, 0x2715);
		u8g2_DrawGlyph(&u8g2, 122, 1, 0x2713);
	}
	else if (input == OFF) u8g2_DrawStr(&u8g2, 10, 55, "Button OFF pressed");
	else no_input();
	u8g2_SendBuffer(&u8g2);
}

void displayInit(void){
	u8g2_Setup_sh1106_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_atmega328p_hw_i2c, u8g2_gpio_and_delay_atmega328p);
	u8g2_InitDisplay(&u8g2);		//Send initialization code to the display
	u8g2_ClearDisplay(&u8g2);
	u8g2_SetPowerSave(&u8g2, 0);
}

uint8_t readButtons(void) {
    uint8_t buttons = PIND & ~0xE0;
    if (buttons != 0x1F) {
         switch(buttons) {
            case (B_SC1):
                return SW_C1;
                break;
            case (B_SC2):
                return SW_C2;
                break;
            case (B_SC3):
                return SW_C3;
                break;
            case (B_SC4):
                return SW_C4;
                break;
            case (B_OFF):
                return OFF;
                break;
            default:
                break;
        }
    }
    return NOT_SELECTED;
}

void options_menu() {
    uint8_t pressedButton = 0;
    uint8_t stateChanged = 0;
    uint8_t loop = 1;

	uint8_t autoSwitchingEnabled = 1;
	uint8_t displayRotation = 0;
	uint8_t autoDisableOnLOS = 1;
	uint8_t defaultInput = SW_C1;
	options_screen(&autoSwitchingEnabled, &displayRotation, &autoDisableOnLOS, &defaultInput);
	_delay_ms(500);
    while(loop) {
		_delay_ms(150);
		options_screen(&autoSwitchingEnabled, &displayRotation, &autoDisableOnLOS, &defaultInput);
        pressedButton = readButtons();
        switch(pressedButton) {
            case (SW_C1): // auto switching
				autoSwitchingEnabled ^= 0x1;
                break;
            case (SW_C2): // set display rotation
				displayRotation ^= 0x1;
                break;
            case (SW_C3): // auto disable on LOS
				autoDisableOnLOS ^= 0x1;
                break;
            case (SW_C4): // default input
				defaultInput++;
				if (defaultInput > 3) defaultInput = 0;
                break;
            case (OFF): // exit menu
				loop = 0;
                break;
            default:
                break;
        }
    }
}

int main(void) {
	DDRB = 0b00100000;
	DDRD = 0x00;
	PORTD = 0x00;
	displayInit();
	u8g2_ClearBuffer(&u8g2);
	u8g2_prepare();
	no_input();
	u8g2_SendBuffer(&u8g2);
	uint8_t draw_state = 0xFF;
	uint8_t hold_timer = 0;
	uint8_t sleep_timer = 0;
	while(1){

		if ((~PIND & 0x1F) & 0x4) {
			_delay_ms(20);
			if ((~PIND & 0x1F) & 0x4) hold_timer++;
			else draw_state = OFF;

			if (hold_timer > 40) {
				u8g2_SetPowerSave(&u8g2, 0);
				sleep_timer = 0;
				options_menu();
				draw(SW_C1);
				_delay_ms(500);
				hold_timer = 0;
			}
		} else {
			hold_timer = 0;
			draw_state = readButtons();
		}

		if (draw_state != 0xFF) {
			sleep_timer = 0;
			draw(draw_state);
			draw_state = 0xFF;
		} else {
			if (sleep_timer != 0xFF) {
				sleep_timer++;
				_delay_ms(10);
			} else u8g2_SetPowerSave(&u8g2, 1);
		} 
	}
}