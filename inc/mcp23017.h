#pragma once

#include "pin.h"
#include "i2c.h"
#include "io_extender.h"

using namespace System;

class MCP23017 : public I2CIOExtender {

private:

	// Communication buffer
	uint8_t _tx_buffer[2];

	// Shadow registers
	uint8_t _iodira;	// 0x00
	uint8_t _iodirb;	// 0x01
	uint8_t _gppua;		// 0x0C
	uint8_t _gppub;		// 0x0D
	uint8_t _gpioa;		// 0x12
	uint8_t _gpiob;		// 0x13

public:

	MCP23017 (I2C & i2c, uint8_t slave_address);
	void setDirection (uint32_t pin, Pin::Direction direction);
	void setPullMode (uint32_t pin, Pin::PullMode mode);
	void setOpenDrain (uint32_t pin, bool open_drain);
	void set (uint32_t pin);
	void clear (uint32_t pin);
	void write (uint32_t pin, Pin::Level level);
	Pin::Level read (uint32_t pin);
};
