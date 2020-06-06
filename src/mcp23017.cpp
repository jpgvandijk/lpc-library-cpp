// Includes
#include "mcp23017.h"
#include "io_extender.h"
#include "pin.h"
#include "i2c.h"

// Namespaces
using namespace System;

MCP23017::MCP23017 (I2C & i2c, uint8_t slave_address) : I2CIOExtender(i2c, slave_address) {
	_iodira = 0xFF;
	_iodirb = 0xFF;
	_gppua = 0x00;
	_gppub = 0x00;
	_gpioa = 0x00;
	_gpiob = 0x00;
}

void MCP23017::setDirection (uint32_t pin, Pin::Direction direction) {

	// Wait for the previous transfer to finish
	while (_i2c.isBusy()) {}

	// Use a buffer that will not go out of context
	if (pin >> 5) {
		pin = 1 << (pin & 0x7);
		if (direction == Pin::Direction::input) {
			_iodirb |= pin;
		} else {
			_iodirb &= ~pin;
		}
		_tx_buffer[0] = 0x01;
		_tx_buffer[1] = _iodirb;
	} else {
		pin = 1 << (pin & 0x7);
		if (direction == Pin::Direction::input) {
			_iodira |= pin;
		} else {
			_iodira &= ~pin;
		}
		_tx_buffer[0] = 0x00;
		_tx_buffer[1] = _iodira;
	}

	// Start transfer, but don't wait for it to finish!
	_i2c.startTransfer(_slave_address, _tx_buffer, 2, nullptr, 0);
}

void MCP23017::setPullMode (uint32_t pin, Pin::PullMode mode) {

	// Wait for the previous transfer to finish
	while (_i2c.isBusy()) {}

	// Use a buffer that will not go out of context
	if (pin >> 5) {
		pin = 1 << (pin & 0x7);
		if (mode == Pin::PullMode::pull_up) {
			_gppub |= pin;
		} else {

			// Disable pull-up, as other modes not supported
			_gppub &= ~pin;
		}
		_tx_buffer[0] = 0x0D;
		_tx_buffer[1] = _gppub;
	} else {
		pin = 1 << (pin & 0x7);
		if (mode == Pin::PullMode::pull_up) {
			_gppua |= pin;
		} else {

			// Disable pull-up, as other modes not supported
			_gppua &= ~pin;
		}
		_tx_buffer[0] = 0x0C;
		_tx_buffer[1] = _gppua;
	}

	// Start transfer, but don't wait for it to finish!
	_i2c.startTransfer(_slave_address, _tx_buffer, 2, nullptr, 0);
}

void MCP23017::setOpenDrain (uint32_t pin, bool open_drain) {
	// Not supported, return
}

void MCP23017::set (uint32_t pin) {
	write(pin, Pin::Level::high);
}

void MCP23017::clear (uint32_t pin) {
	write(pin, Pin::Level::low);
}

void MCP23017::write (uint32_t pin, Pin::Level level) {

	// Wait for the previous transfer to finish
	while (_i2c.isBusy()) {}

	// Use a buffer that will not go out of context
	if (pin >> 5) {
		pin = 1 << (pin & 0x7);
		if (level == Pin::Level::high) {
			_gpiob |= pin;
		} else {

			// Disable pull-up, as other modes not supported
			_gpiob &= ~pin;
		}
		_tx_buffer[0] = 0x13;
		_tx_buffer[1] = _gpiob;
	} else {
		pin = 1 << (pin & 0x7);
		if (level == Pin::Level::high) {
			_gpioa |= pin;
		} else {

			// Disable pull-up, as other modes not supported
			_gpioa &= ~pin;
		}
		_tx_buffer[0] = 0x12;
		_tx_buffer[1] = _gpioa;
	}

	// Start transfer, but don't wait for it to finish!
	_i2c.startTransfer(_slave_address, _tx_buffer, 2, nullptr, 0);
}

Pin::Level MCP23017::read (uint32_t pin) {

	// Wait for the previous transfer to finish
	while (_i2c.isBusy()) {}

	// Use a buffer that will not go out of context
	if (pin >> 5) {
		_tx_buffer[0] = 0x13;
	} else {
		_tx_buffer[0] = 0x12;
	}

	// Start transfer
	_i2c.startTransfer(_slave_address, _tx_buffer, 1, _tx_buffer + 1, 1);

	// Wait for the transfer to finish
	while (_i2c.isBusy()) {}

	// Read the result
	pin = 1 << (pin & 0x7);
	if (_tx_buffer[1] & pin) {
		return Pin::Level::high;
	} else {
		return Pin::Level::low;
	}
}
