// Includes
#include "io_extender.h"
#include "pin.h"
#include "spi.h"
#include "i2c.h"

// Namespaces
using namespace System;

IOExtender::IOExtender (void) {
}

SPIIOExtender::SPIIOExtender (SPI & spi, Pin & ss_pin) : _spi(spi), _ss_pin(ss_pin) {
}

I2CIOExtender::I2CIOExtender (I2C & i2c, uint8_t slave_address) : _i2c(i2c), _slave_address(slave_address) {
}

IOExtenderPin::IOExtenderPin (IOExtender & io_extender, uint32_t pin) : Pin(pin), _io_extender(io_extender) {
}

void IOExtenderPin::setDirection (Pin::Direction direction) {
	_io_extender.setDirection(_pin, direction);
}

void IOExtenderPin::setPullMode (Pin::PullMode mode) {
	_io_extender.setPullMode(_pin, mode);
}

void IOExtenderPin::setOpenDrain (bool open_drain) {
	_io_extender.setOpenDrain(_pin, open_drain);
}

void IOExtenderPin::set (void) {
	_io_extender.set(_pin);
}

void IOExtenderPin::clear (void) {
	_io_extender.clear(_pin);
}

void IOExtenderPin::write (Pin::Level level) {
	_io_extender.write(_pin, level);
}

Pin::Level IOExtenderPin::read (void) {
	return _io_extender.read(_pin);
}
