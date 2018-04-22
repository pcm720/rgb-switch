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

#include <avr/sleep.h>
#include "common.h"
#include "display.h"
#include "switch.h"
#include "eeprom.h"

volatile uint8_t activeButton = NOT_SELECTED;
volatile uint8_t offCounter = 0;
volatile uint8_t powerSave = 0;

void init(switchOptions* options);
void mcu_sleep(void);
void options_menu(switchOptions* options);

int main(void) {
    
    switchOptions options;
    init(&options);

    uint8_t sleep_timer = 0;
    uint8_t currentInput = options.defaultInput;
    uint8_t autoSwitchingState = options.autoSwitchingEnabled;
    uint8_t syncState = 0;

    activeButton = NOT_SELECTED;
    if (autoSwitchingState == 0) manual_switch(currentInput);
    else draw(SYNC_SEARCH);

    while(1) {
        while (!powerSave) {
            sleep_timer++;

            if (sleep_timer == 0xFF) {
                toggle_powerSave(); // disable display
                sleep_timer = 0;
            }

            if (offCounter == 0xFF) { // OFF button was held, switch to options menu
                sleep_timer = 0;
                options_menu(&options);
                if (autoSwitchingState && !options.autoSwitchingEnabled) currentInput = options.defaultInput;
            }
            
            offCounter = 0;

            if (!(PIND & SD_LOS)) {
                if (options.autoSwitchingEnabled && autoSwitchingState) {
                    currentInput = auto_switch();
                    if (currentInput != NOT_SELECTED) autoSwitchingState = 0;
                    else {
                        sleep_timer = 0;
                        currentInput = SYNC_SEARCH;
                    }
                }
                if ((options.DisableOnLOS || options.autoSwitchingEnabled) && syncState) {
                        autoSwitchingState = 1;
                        syncState = 0;
                        sleep_timer = 0;
                        currentInput = OFF;
                        switch_output(currentInput);
                        draw(SYNC_LOST);
                        _delay_ms(1000);
                }
            } else syncState = 1;

            if ((activeButton != NOT_SELECTED) && (offCounter == 0)) { // button press happened, switch to selected input
                sleep_timer = 0;
                syncState = 0;
                
                currentInput = activeButton;
                if (currentInput == OFF) autoSwitchingState = 1;
                else autoSwitchingState = 0;
                
                manual_switch(currentInput);
                draw(currentInput);
                 _delay_ms(500);
                activeButton = NOT_SELECTED;
            }
            draw(currentInput); // update screen
        }
        mcu_sleep(); // put ATmega to sleep
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
        options->DisableOnLOS = 0;
        options->defaultInput = OFF; 
        options->checksum = 0x0;
        eeprom_write_state(options); // try to save defaults
    }

    PCMSK2 = 0x3B; // prepare interrupts for every input pin except PD2
    EICRA |= 0x00; // set INT0 to trigger when PD2 is low
    PCICR = 0x4; // enable PCINT for PORTD
    EIMSK |= 0x01; // enable INT0
}

void mcu_sleep(void) {
    set_sleep_mode(SLEEP_MODE_IDLE); // set sleep mode
    cli();
    PORTB ^= 0x20;
    _delay_ms(500);
    PORTB ^= 0x20;
    _delay_ms(500);
    PORTB ^= 0x20;
    _delay_ms(500);
    PORTB ^= 0x20;
    sleep_enable();
    sei();
    sleep_cpu();
    sleep_disable();
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
                Interrupt vectors
-------------------------------------------------*/

ISR(INT0_vect) { // OFF button interrupt
    if (powerSave) {
        toggle_powerSave();
    } else {
        offCounter++;
        activeButton = NOT_SELECTED;
        if (offCounter > 6) {
            offCounter = 0xFF;
            EIMSK = 0x00; // disable INT0, will be enabled later by options_menu draw functions
        } else if (offCounter < 3) activeButton = OFF;
    }
    _delay_ms(150);
}

ISR(PCINT2_vect) {
    uint8_t inputs = PIND;
    if (powerSave) toggle_powerSave();
    else {
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
    }
       _delay_ms(100);
}