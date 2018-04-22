/*
EEPROM operations
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

#include "eeprom.h"
#include "display.h"
#include <avr/eeprom.h>

inline uint8_t get_checksum(switchOptions* options);

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