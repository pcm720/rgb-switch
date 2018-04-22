/*
Switching functions
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

#include "switch.h"
#include <util/atomic.h>

void switch_output(uint8_t out) {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { //disable interrupts
        if ((DDRC & ~0xF0) != (1 << out)) { // check that selected switch is not already enabled
            DDRC &= ~0x0F; // disable all switches on PORT C
            _delay_ms(10);
            DDRC |= (1 << out);
            _delay_ms(10);
        }
    }
}

uint8_t auto_switch(void) {
    uint8_t activeInput = NOT_SELECTED;
    for (uint8_t i = 0; i < 4; i++) {
        switch_output(i);
        _delay_ms(10);
        if (PIND & SD_LOS) {
            activeInput = i;
            i = 5;
        }
    }
    return activeInput;
}

void manual_switch(uint8_t currentInput) {
    ATOMIC_BLOCK(ATOMIC_FORCEON) { //disable interrupts
        if (currentInput != OFF) {
            switch_output(currentInput);
        } else {
            DDRC &= ~0x0F;
        }
    }
}