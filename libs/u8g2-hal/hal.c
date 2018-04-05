#include <avr/io.h>
#include <util/delay.h>
#include "hal.h"

uint8_t u8x8_byte_atmega328p_hw_i2c(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr)
{
  switch(msg)
  {
    case U8X8_MSG_BYTE_SEND:
      i2c_write_array((uint8_t *)arg_ptr, (int)arg_int);
      break;
    case U8X8_MSG_BYTE_INIT:
      i2c_init();
      break;
    case U8X8_MSG_BYTE_SET_DC:
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      i2c_start(u8x8_GetI2CAddress(u8x8)>>1, I2C_WRITE);
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
			_delay_ms(arg_int);
			break;
		case U8X8_MSG_DELAY_10MICRO:
			_delay_us(10);
			break;
		default:
			return 0;
		}							
	return 1;
}