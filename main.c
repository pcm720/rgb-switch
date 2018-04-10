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
#include "libs/u8g2-hal/hal.h"

#define ENABLE_BITMAPS
#ifdef ENABLE_BITMAPS
    #include "extra/xbm/xbm.h" //optional file, contains xbitmap images for u8g2
#endif

// inputs on PORT D
#define B_SC1 0
#define B_SC2 1
#define B_SC3 3
#define B_SC4 4
#define B_OFF 2
#define SD_LOS 5

// switch control pins on PORT C
#define SW_C1 0
#define SW_C2 1
#define SW_C3 2
#define SW_C4 3

// special inputs
#define OFF 20
#define NOT_SELECTED 0xFF

typedef struct {
    uint8_t currentInput;
    uint8_t autoSwitchingEnabled;
    uint8_t autoDisableOnLOS;
    uint8_t defaultInput;
    uint8_t checksum;
    uint8_t displayRotation;
} switchState;

u8g2_t u8g2;

void init(switchState* state);
void displayInit(uint8_t rotation);
void switchOutput(uint8_t out);
uint8_t readButtons(void);
void manualSwitch(uint8_t* currentInput);
void autoSwitch(uint8_t* currentInput);
void options_menu(switchState* state);
uint8_t options_submenu(uint8_t* option, uint8_t type);
uint8_t eeprom_read_state(switchState* state);
uint8_t eeprom_write_state(switchState* state);
inline uint8_t get_checksum(switchState* state);

int main(void) {
    switchState state;

    init(&state);
    _delay_ms(50);

    while(1) {
        if ((~PIND & ~0xFB) & (1 << B_OFF)) {
            _delay_ms(100); //check if OFF button is held for more than 100 us
            if ((~PIND & ~0xFB) & (1 << B_OFF)) {
                options_menu(&state);
            }
        }

        if (((~PIND & ~0xDF) & (1 << SD_LOS)) && state.autoDisableOnLOS) { // lost sync
            _delay_ms(50);
            if ((~PIND & ~0xDF) & (1 << SD_LOS)) {
                // disable all outputs, display info about lost sync
                DDRC &= ~0x0F;
                state.currentInput = OFF;
            }
        }

        if (state.autoSwitchingEnabled == 1 && state.currentInput == OFF) autoSwitch(&state.currentInput);
        manualSwitch(&state.currentInput);

        _delay_ms(100);
    }
}

void init(switchState* state) {
    DDRD = 0x00; // set PORT D/C direction to input
    DDRC = 0x00;
    PORTC = 0x00; // output low by default
    
    if (eeprom_read_state(state)) {     // read config from EEPROM
        state->currentInput = OFF; state->autoSwitchingEnabled = 1; // reading failed, revert to default parameters
        state->autoDisableOnLOS = 1; state->defaultInput = OFF; 
        state->checksum = 0x0; state->displayRotation = 0;
        eeprom_write_state(state); // try to save defaults
    }

    displayInit(state->displayRotation); // init display
}

void displayInit(uint8_t rotation) {
    switch(rotation) {
        case 1:
            u8g2_Setup_sh1106_i2c_128x64_noname_f(&u8g2, U8G2_R2, u8x8_byte_atmega328p_hw_i2c, u8g2_gpio_and_delay_atmega328p);
            break;
        default:
            u8g2_Setup_sh1106_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_atmega328p_hw_i2c, u8g2_gpio_and_delay_atmega328p);
            break;
    }
	u8g2_InitDisplay(&u8g2); // send initialization code to the display
	u8g2_ClearDisplay(&u8g2);
	u8g2_SetPowerSave(&u8g2, 0);
}

void switchOutput(uint8_t out) {
    if ((DDRC & ~0xF0) != (1 << out)) { // check that selected switch is not already enabled
        DDRC &= ~0x0F; // disable all switches on PORT C
        _delay_ms(2);
        DDRC |= (1 << out);
        _delay_ms(2);
    }
}

uint8_t readButtons(void) {
    uint8_t buttons = 0;
    if ((buttons = (PIND & ~0xE0)) != 0) {
         switch((~buttons) & ~0xE0) {
            case (1 << B_SC1):
                return SW_C1;
                break;
            case (1 << B_SC2):
                return SW_C2;
                break;
            case (1 << B_SC3):
                return SW_C3;
                break;
            case (1 << B_SC4):
                return SW_C4;
                break;
            case (1 << B_OFF):
                return OFF;
                break;
            default:
                break;
        }
    }
    return NOT_SELECTED;
}

void manualSwitch(uint8_t* currentInput) {
    uint8_t pressedButton = readButtons();
    if (pressedButton != OFF && pressedButton != NOT_SELECTED) {
        switchOutput(pressedButton);
        *currentInput = pressedButton;
    } else if (pressedButton == OFF) {
        DDRC &= ~0x0F;
        *currentInput = OFF;
    }
}

void autoSwitch(uint8_t* currentInput) {
    for (uint8_t i = 0; i < 4; i++) {
        switchOutput(i);
        _delay_ms(15);
        if ((PIND & ~0xDF) & (1 << SD_LOS)) {
            *currentInput = SW_C1;
            break;
        }
    }
}

void options_menu(switchState* state) {
    uint8_t pressedButton = 0;
    uint8_t stateChanged = 0;
    uint8_t loop = 1;
    while(loop) {
        //show options screen using state struct as data source (userInterfaceSelectionList?)
        pressedButton = readButtons();
        _delay_ms(20);
        switch(pressedButton) {
            case (1 << B_SC1): // auto switching
                state->autoSwitchingEnabled ^= 0x1;
                stateChanged = 1;
                break;
            case (1 << B_SC2): // set display rotation
                if (options_submenu(&(state->displayRotation), 1)) stateChanged = 1;
                break;
            case (1 << B_SC3): // auto disable on LOS
                state->autoDisableOnLOS ^= 0x1;
                stateChanged = 1;
                break;
            case (1 << B_SC4): // default input
                if (options_submenu(&(state->defaultInput), 2)) stateChanged = 1;
                break;
            case (1 << B_OFF): // exit menu
                loop = 0;
                break;
            default:
                break;
        }
    }
    if (stateChanged) eeprom_write_state(state);
}

uint8_t options_submenu(uint8_t* option, uint8_t type) {
    //show "please press the button" screen
    uint8_t loop = 1;
    uint8_t button = 0;
    while (loop) {
        button = readButtons();
        if ( button != NOT_SELECTED && button != OFF ) {
            *option = button;
            loop = 0;
        } else if (button == OFF) {
            loop = 0;
        }
    }
    if (button == OFF) return 0;
    else {
        switch(type) {
            case 1:
                *option %= 2;
                break;
            default:
                break;
        }
        return 1;
    }
}

uint8_t eeprom_read_state(switchState* state) {
    eeprom_read_block((void*)state, (void*)0, sizeof(*state));
    if (get_checksum(state) == state->checksum) return 0; // read successful
    else return 1; // read failed, need to show that on display
}

uint8_t eeprom_write_state(switchState* state) {
    state->checksum = get_checksum(state);
    eeprom_update_block((const void*)state, (void*)0, sizeof(*state));
    if (eeprom_read_state(state)) return 1; // write failed, need to show that on display
    else return 0; // write successful
}

inline uint8_t get_checksum(switchState* state) {
    return state->currentInput + 
           state->autoSwitchingEnabled + 
           state->autoDisableOnLOS + 
           state->defaultInput +
           state->displayRotation;
}