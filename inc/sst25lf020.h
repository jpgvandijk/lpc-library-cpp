#pragma once

#include "spi.h"
#include "pin.h"

using namespace System;

class SST25LF020 {

private:
	SPI & _spi;
	Pin & _pin_ss;

public:

	SST25LF020 (SPI & spi, Pin & pin_ss) : _spi(spi), _pin_ss(pin_ss) {
		// Note: the SPI must already be initialized, as we cannot determine which initializer method is available!
		// Moreover, the SPI can be shared with other chips, and should not be initialized each time!

		// Initialize the SS pin, and de-select the chip
		_pin_ss.setDirection(Pin::Direction::output);
		deselect();
	}

	void select (void) {
		_pin_ss.clear();
	}

	void deselect (void) {
		_pin_ss.set();
	}

	static uint32_t getMaximumClockFrequency (void) {
		return 33000000;
	}

	static bool isCPOL (void) {
		return false;
	}

	static bool isCPHA (void) {
		return false;
	}

	static bool isLSBFirst (void) {
		return false;
	}

	uint16_t readID (void) {
		uint8_t tx_buffer[4] = {0x90, 0x00, 0x00, 0x00};
		uint8_t rx_buffer[2];

		while (_spi.isBusy()) {}
		select();
		_spi.transmit(tx_buffer, 4);
		while (_spi.isBusy()) {}
		_spi.receive(rx_buffer, 2);
		while (_spi.isBusy()) {}
		deselect();

		return (rx_buffer[0] << 8) | rx_buffer[1];
	}


};
