// Includes
#include "LPC17xx.h"
#include "i2c.h"
#include "clock.h"
#include "pin.h"
#include "interrupt.h"

// Namespaces
using namespace System;

// TODO: register for CPU clock frequency updates!

/************************************
* I2C Interrupt Handlers			*
************************************/

namespace {
	void (*handleInterruptPointer[3]) (void) = {nullptr, nullptr, nullptr};
}

extern "C" {

	void I2C0_IRQHandler (void) {
		if (handleInterruptPointer[0] != nullptr) {
			handleInterruptPointer[0]();
		}
	}

	void I2C1_IRQHandler (void) {
		if (handleInterruptPointer[1] != nullptr) {
			handleInterruptPointer[1]();
		}
	}

	void I2C2_IRQHandler (void) {
		if (handleInterruptPointer[2] != nullptr) {
			handleInterruptPointer[2]();
		}
	}
}

namespace System {

	/************************************
	* I2C Base Implementation			*
	************************************/

	I2C::I2C (uint32_t instance) {
		if (instance == 0) {
			_lpc_i2c = LPC_I2C0;
		} else if (instance == 1) {
			_lpc_i2c = LPC_I2C1;
		} else { // (instance == 2)
			_lpc_i2c = LPC_I2C2;
		}
		busy = false;
	}

	void I2C::initialize (uint32_t pin_sda, Pin::Function function, uint32_t peripheral_frequency, Mode mode) {

		// Init SDA pin
		Pin::setFunction(pin_sda, function);
		Pin::setPullMode(pin_sda, Pin::PullMode::no_pull);
		Pin::setOpenDrain(pin_sda, true);

		// Init SCL pin
		uint32_t pin_scl = pin_sda + 1;
		Pin::setFunction(pin_scl, function);
		Pin::setPullMode(pin_scl, Pin::PullMode::no_pull);
		Pin::setOpenDrain(pin_scl, true);

		// Determine the total clock divider
		uint32_t bus_frequency = 100000;
		if (mode == I2C::Mode::fast_mode) {
			bus_frequency = 400000;
		} else if (mode == I2C::Mode::fast_mode_plus) {
			bus_frequency = 1000000;
		}
		uint32_t sum = peripheral_frequency / bus_frequency;

		// Set according to desired duty cycle
		if (mode == I2C::Mode::standard) {
			_lpc_i2c->I2SCLH = sum / 2;
		} else {
			_lpc_i2c->I2SCLH = sum / 3;
		}
		_lpc_i2c->I2SCLL = sum - _lpc_i2c->I2SCLH;

		// Enable the I2C interface in master transmitter mode
		_lpc_i2c->I2CONCLR = (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);
		_lpc_i2c->I2CONSET = (1 << 6);
	}

	bool I2C::isBusy (void) {
		return (busy || (_lpc_i2c->I2STAT != 0xF8));
	}

	bool I2C::startTransfer (uint8_t slave_address, uint8_t * tx_buffer, uint8_t tx_length, uint8_t * rx_buffer, uint8_t rx_length) {

		// Check if a new transfer can be started
		if (isBusy())
			return false;
		if ((tx_length == 0) && (rx_length == 0))
			return true;

		// Store all settings
		this->slave_address = (slave_address & ~(1 << 0));
		this->tx_buffer = tx_buffer;
		this->tx_length = tx_length;
		this->rx_buffer = rx_buffer;
		this->rx_length = rx_length;

		// Generate the START, the rest is handled in the ISR
		_lpc_i2c->I2CONSET = (1 << 5);

		// The transfer was successfully started
		busy = true;
		return true;
	}

	void I2C::handle (void) {

		// Read the state
		switch (_lpc_i2c->I2STAT) {
		case 0x08:
		case 0x10:

			// (RE)START sent
			if (tx_length != 0) {
				_lpc_i2c->I2DAT = slave_address;
			} else {
				_lpc_i2c->I2DAT = slave_address | 1;
			}
			_lpc_i2c->I2CONCLR = (1 << 5);
			_lpc_i2c->I2CONSET = (1 << 2);
			break;

		case 0x18:
		case 0x28:

			// WRITE or data sent
			if (tx_length != 0) {
				_lpc_i2c->I2DAT = *tx_buffer;
				tx_buffer++;
				tx_length--;
				_lpc_i2c->I2CONSET = (1 << 2);
			} else if (rx_length != 0) {
				_lpc_i2c->I2CONSET = (1 << 2) | (1 << 5);
			} else {
				_lpc_i2c->I2CONSET = (1 << 2) | (1 << 4);
				busy = false;
			}
			break;

		case 0x50:

			// Data received
			*rx_buffer = _lpc_i2c->I2DAT;
			rx_buffer++;
			rx_length--;

		case 0x40:

			// READ sent
			if (rx_length == 1) {
				_lpc_i2c->I2CONCLR = (1 << 2);
			} else {
				_lpc_i2c->I2CONSET = (1 << 2);
			}
			break;

		case 0x58:

			// Data received with No ACK
			*rx_buffer = _lpc_i2c->I2DAT;
			rx_buffer++;
			rx_length--;

		case 0x20:
		case 0x30:
		case 0x48:

			// No ACK received, send STOP
			_lpc_i2c->I2CONSET = (1 << 2) | (1 << 4);
			busy = false;
			break;

		case 0x38:

			// Arbitration lost, resend START
			_lpc_i2c->I2CONCLR = (1 << 2) | (1 << 5);
			busy = false;
			break;

		default:
			break;
		}

		// Continue running the I2C interface
		_lpc_i2c->I2CONCLR = (1 << 3);
	}

	/************************************
	* I2C0 Singleton					*
	************************************/

	I2C0 & I2C0::instance (void) {
		static I2C0 i2c;
		return i2c;
	}

	I2C0::I2C0 (void) : I2C(0) {
		handleInterruptPointer[0] = handleInterrupt;
	}

	void I2C0::handleInterrupt (void) {
		instance().handle();
	}

	void I2C0::initialize (Clock::PeripheralClockSpeed clock, Mode mode) {
		Clock::enablePeripheral(Clock::PeripheralPower::i2c_0_power);
		Clock::setPeripheralClock(Clock::PeripheralClock::i2c_0_clock, clock);
		uint32_t frequency = Clock::getPeripheralClockFrequency(Clock::PeripheralClock::i2c_0_clock);

		// Select the desired operating mode for the pin drivers
		uint32_t pin_sda = PIN(0, 27);
		if (mode == I2C::Mode::fast_mode_plus) {
			LPC_PINCON->I2CPADCFG = 0x05;
		} else {
			LPC_PINCON->I2CPADCFG = 0x00;
		}
		I2C::initialize(pin_sda, Pin::Function::alternate_1, frequency, mode);
		Interrupt::enable(I2C0_IRQn);
	}

	/************************************
	* I2C1 Singleton					*
	************************************/

	I2C1 & I2C1::instance (void) {
		static I2C1 i2c;
		return i2c;
	}

	I2C1::I2C1 (void) : I2C(1) {
		handleInterruptPointer[1] = handleInterrupt;
	}

	void I2C1::handleInterrupt (void) {
		instance().handle();
	}

	void I2C1::initialize (Clock::PeripheralClockSpeed clock, Mode mode, PinSelection pin_selection) {
		Clock::enablePeripheral(Clock::PeripheralPower::i2c_1_power);
		Clock::setPeripheralClock(Clock::PeripheralClock::i2c_1_clock, clock);
		uint32_t frequency = Clock::getPeripheralClockFrequency(Clock::PeripheralClock::i2c_1_clock);

		uint32_t pin_sda = PIN(0, 19);
		if (pin_selection == I2C1::PinSelection::p0_0_and_p0_1) {
			pin_sda = PIN(0, 0);
		}
		I2C::initialize(pin_sda, Pin::Function::alternate_3, frequency, mode);
		Interrupt::enable(I2C1_IRQn);
	}

	/************************************
	* I2C2 Singleton					*
	************************************/

	I2C2 & I2C2::instance (void) {
		static I2C2 i2c;
		return i2c;
	}

	I2C2::I2C2 (void) : I2C(2) {
		handleInterruptPointer[2] = handleInterrupt;
	}

	void I2C2::handleInterrupt (void) {
		instance().handle();
	}

	void I2C2::initialize (Clock::PeripheralClockSpeed clock, Mode mode) {
		Clock::enablePeripheral(Clock::PeripheralPower::i2c_2_power);
		Clock::setPeripheralClock(Clock::PeripheralClock::i2c_2_clock, clock);
		uint32_t frequency = Clock::getPeripheralClockFrequency(Clock::PeripheralClock::i2c_2_clock);

		uint32_t pin_sda = PIN(0, 10);
		I2C::initialize(pin_sda, Pin::Function::alternate_2, frequency, mode);
		Interrupt::enable(I2C2_IRQn);
	}
}
