#pragma once

#include "pin.h"
#include "spi.h"
#include "i2c.h"

using namespace System;

class IOExtender {

public:
	IOExtender (void);
	virtual void setDirection (uint32_t pin, Pin::Direction direction) = 0;
	virtual void setPullMode (uint32_t pin, Pin::PullMode mode) = 0;
	virtual void setOpenDrain (uint32_t pin, bool open_drain) = 0;
	virtual void set (uint32_t pin) = 0;
	virtual void clear (uint32_t pin) = 0;
	virtual void write (uint32_t pin, Pin::Level level) = 0;
	virtual Pin::Level read (uint32_t pin) = 0;
};

class SPIIOExtender : public IOExtender {

protected:
	SPI & _spi;
	Pin & _ss_pin;

public:
	SPIIOExtender (SPI & spi, Pin & ss_pin);
};

class I2CIOExtender : public IOExtender {

protected:
	I2C & _i2c;
	uint8_t _slave_address;

public:
	I2CIOExtender (I2C & i2c, uint8_t slave_address);
};

class IOExtenderPin : public Pin
{
private:
	IOExtender & _io_extender;

public:
	IOExtenderPin (IOExtender & io_extender, uint32_t pin);
	void setDirection (Direction direction);
	void setPullMode (PullMode mode);
	void setOpenDrain (bool open_drain);
	void set (void);
	void clear (void);
	void write (Pin::Level level);
	Pin::Level read (void);
};
