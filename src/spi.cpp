// Includes
#include "LPC17xx.h"
#include "spi.h"
#include "clock.h"
#include "pin.h"
#include "interrupt.h"

// Namespaces
using namespace System;

// TODO: register for CPU clock frequency updates!

/************************************
* UART Interrupt Handlers			*
************************************/

namespace {
	void (*handleInterruptPointer[3]) (void) = {nullptr, nullptr, nullptr};
}

extern "C" {

	void SPI_IRQHandler (void) {
		if (handleInterruptPointer[0] != nullptr) {
			handleInterruptPointer[0]();
		}
	}

	void SSP0_IRQHandler (void) {
		if (handleInterruptPointer[1] != nullptr) {
			handleInterruptPointer[1]();
		}
	}

	void SSP1_IRQHandler (void) {
		if (handleInterruptPointer[2] != nullptr) {
			handleInterruptPointer[2]();
		}
	}
}

/************************************
* SPI Base Implementation			*
************************************/

SPI::SPI (volatile uint32_t * data_register) {
	_busy = false;
	_tx_buffer = nullptr;
	_rx_buffer = nullptr;

	// Cache the address to make the ISR faster
	// Alternative 1 would be to call getDataRegister() every time in _write() and _read()
	// Alternative 2 would be to implement the _write() and _read() in the derived classes
	_data_register = data_register;
}

bool SPI::isBusy (void) {
	return _busy;
}

bool SPI::transmit (uint8_t * tx_buffer, uint16_t length) {
	return _transceive (tx_buffer, nullptr, length, true);
}

bool SPI::receive (uint8_t * rx_buffer, uint16_t length) {
	return _transceive (nullptr, rx_buffer, length, true);
}

bool SPI::transceive (uint8_t * tx_buffer, uint8_t * rx_buffer, uint16_t length) {
	return _transceive (tx_buffer, rx_buffer, length, true);
}

bool SPI::transmit (uint16_t * tx_buffer, uint16_t length) {
	return _transceive (tx_buffer, nullptr, length, false);
}

bool SPI::receive (uint16_t * rx_buffer, uint16_t length) {
	return _transceive (nullptr, rx_buffer, length, false);
}

bool SPI::transceive (uint16_t * tx_buffer, uint16_t * rx_buffer, uint16_t length) {
	return _transceive (tx_buffer, rx_buffer, length, false);
}

bool SPI::_transceive (void * tx_buffer, void * rx_buffer, uint16_t length, bool byte) {

	// Check if a new transfer can be started
	if (isBusy())
		return false;
	if (length == 0)
		return true;

	// Store all settings
	_byte = byte;
	_tx_buffer = tx_buffer;
	_rx_buffer = rx_buffer;
	_length = length;

	// Start the transfer by writing the first byte, the rest is handled in the ISR
	_busy = true; // FIXME: place before, also in other peripherals!
	_write();

	// The transfer was successfully started
	return true;
}

void SPI::_write (void) {

	// Write a single byte
	if (_tx_buffer != nullptr) {
		if (_byte) {
			*_data_register = *((uint8_t *)_tx_buffer);
			_tx_buffer = ((uint8_t *)_tx_buffer) + 1;
		} else {
			*_data_register = *((uint16_t *)_tx_buffer);
			_tx_buffer = ((uint16_t *)_tx_buffer) + 1;
		}
	} else {
		// Dummy write
		*_data_register = 0;
	}
}

void SPI::_read (void) {

	// Read a single byte
	if (_rx_buffer != nullptr) {
		if (_byte) {
			*((uint8_t *)_rx_buffer) = *_data_register;
			_rx_buffer = ((uint8_t *)_rx_buffer) + 1;
		} else {
			*((uint16_t *)_rx_buffer) = *_data_register;
			_rx_buffer = ((uint16_t *)_rx_buffer) + 1;
		}
	} else {
		// Dummy read
		volatile uint32_t data = *_data_register;
	}
}

void SPI::next (void) {

	// Read and store the received byte
	_read();

	// Decrement the transfer count, and terminate if this was the last
	_length--;
	if (_length == 0) {
		_busy = false;
		return;
	}

	// Send the next byte
	_write();
}

/************************************
* LegacySPI							*
************************************/

volatile uint32_t * LegacySPI::getDataRegister (void) {
	return &(LPC_SPI->SPDR);
}

LegacySPI::LegacySPI (void) : SPI(getDataRegister()) {

}

void LegacySPI::initialize (uint32_t peripheral_frequency, uint32_t spi_clock_frequency, uint32_t mode) {

	// Calculate the clock divider
	uint32_t divider = peripheral_frequency / spi_clock_frequency;
	if (divider < 8) {
		divider = 8;
	} else if (divider > 255) {
		divider = 255;
	}

	// Setup all registers
	LPC_SPI->SPCCR = divider;
	LPC_SPI->SPCR = mode;
}

uint32_t LegacySPI::mode (uint32_t bits, bool clock_phase_2nd_edge, bool clock_polarity_active_low, bool lsb_first) {
	if ((bits < 8) || (bits > 16)) {
		return 0;
	}

	// Prepare the SPI control register
	uint32_t control = (((uint32_t) clock_phase_2nd_edge) << 3) |
						(((uint32_t) clock_polarity_active_low) << 4) |
						(1 << 5) | (((uint32_t) lsb_first) << 6) |	(1 << 7);
	if (bits > 8) {
		control |= (1 << 2) | ((bits & 0xF) << 8);
	}
	return control;
}

void LegacySPI::handle (void) {

	if (LPC_SPI->SPINT & (1 << 0)) {

		// Read the SPI status register
		uint32_t status = LPC_SPI->SPSR;

		// SPI transfer complete?
		if (status & (1 << 7)) {
			next();
		}

		// Clear the interrupt flag
		LPC_SPI->SPINT = (1 << 0);
	}
}

/************************************
* SSP								*
************************************/

volatile uint32_t * SSP::getDataRegister (uint32_t instance) {
	if (instance == 0) {
		return &(LPC_SSP0->DR);
	} else { // (instance == 1)
		return &(LPC_SSP1->DR);
	}
}

SSP::SSP (uint32_t instance) : SPI(getDataRegister(instance)) {
	if (instance == 0) {
		_lpc_ssp = LPC_SSP0;
	} else { // (instance == 1)
		_lpc_ssp = LPC_SSP1;
	}
}

void SSP::initialize (uint32_t peripheral_frequency, uint32_t spi_clock_frequency, uint32_t mode) {

	// Calculate the clock divider
	uint32_t divider = peripheral_frequency / spi_clock_frequency / 2;
	if (divider < 6) {
		divider = 6;
	} else if (divider > 65536) {
		divider = 65536;
	}
	divider = divider - 1;

	// Setup all registers
	_lpc_ssp->CPSR = 2;
	_lpc_ssp->IMSC = (1 << 1);
	_lpc_ssp->CR0 = mode | (divider << 8);
	_lpc_ssp->CR1 = (1 << 1);

}

uint32_t SSP::mode (uint32_t bits, bool clock_phase_2nd_edge, bool clock_polarity_active_low) {
	if ((bits < 4) || (bits > 16)) {
		return 0;
	}
	bits = bits - 1;

	// Prepare the SPI control register
	uint32_t control = (((uint32_t) clock_phase_2nd_edge) << 7) |
						(((uint32_t) clock_polarity_active_low) << 6) |
						(0 << 4) | ((bits & 0xF) << 0);

	return control;
}

void SSP::handle (void) {

	if (_lpc_ssp->MIS & (1 << 1)) {

		// Read the SPI status register
		uint32_t status = _lpc_ssp->SR;

		// SPI transfer complete?
		if (status & (1 << 2)) {
			next();
		}

		// Clear the interrupt flag
		_lpc_ssp->ICR = (1 << 1);
	}
}

/************************************
* SPI0 Singleton					*
************************************/

SPI0 & SPI0::instance (void) {
	static SPI0 spi;
	return spi;
}

SPI0::SPI0 (void) : LegacySPI() {
	handleInterruptPointer[0] = handleInterrupt;
}

void SPI0::handleInterrupt (void) {
	instance().handle();
}

void SPI0::initialize (Clock::PeripheralClockSpeed clock, uint32_t spi_clock_frequency, uint32_t bits, bool clock_phase_2nd_edge, bool clock_polarity_active_low, bool lsb_first) {
	Clock::enablePeripheral(Clock::PeripheralPower::spi_power);
	Clock::setPeripheralClock(Clock::PeripheralClock::spi_clock, clock);
	uint32_t frequency = Clock::getPeripheralClockFrequency(Clock::PeripheralClock::spi_clock);

	// FIXME: make generic in SPI base method? (+2,+3, function)

	// Init SCK pin
	GPIOPin pin_sck(PIN(0, 15));
	pin_sck.setFunction(GPIO::Function::alternate_3);
	pin_sck.setPullMode(Pin::PullMode::no_pull);
	pin_sck.setOpenDrain(false);

	// Init MISO pin
	GPIOPin pin_miso(PIN(0, 17));
	pin_miso.setFunction(GPIO::Function::alternate_3);
	pin_miso.setPullMode(Pin::PullMode::no_pull);
	pin_miso.setOpenDrain(false);

	// Init MOSI pin
	GPIOPin pin_mosi(PIN(0, 18));
	pin_mosi.setFunction(GPIO::Function::alternate_3);
	pin_mosi.setPullMode(Pin::PullMode::no_pull);
	pin_mosi.setOpenDrain(false);

	uint32_t mode = LegacySPI::mode(bits, clock_phase_2nd_edge, clock_polarity_active_low, lsb_first);
	LegacySPI::initialize(frequency, spi_clock_frequency, mode);
	System::Interrupt::enable(SPI_IRQn);
}

/************************************
* SSP0 Singleton					*
************************************/

SSP0 & SSP0::instance (void) {
	static SSP0 ssp;
	return ssp;
}

SSP0::SSP0 (void) : SSP(0) {
	handleInterruptPointer[1] = handleInterrupt;
}

void SSP0::handleInterrupt (void) {
	instance().handle();
}

void SSP0::initialize (Clock::PeripheralClockSpeed clock, uint32_t spi_clock_frequency, uint32_t bits, bool clock_phase_2nd_edge, bool clock_polarity_active_low) {
	Clock::enablePeripheral(Clock::PeripheralPower::ssp_0_power);
	Clock::setPeripheralClock(Clock::PeripheralClock::ssp_0_clock, clock);
	uint32_t frequency = Clock::getPeripheralClockFrequency(Clock::PeripheralClock::ssp_0_clock);

	// FIXME: implement pin initialization!

	uint32_t mode = SSP::mode(bits, clock_phase_2nd_edge, clock_polarity_active_low);
	SSP::initialize(frequency, spi_clock_frequency, mode);
	System::Interrupt::enable(SSP0_IRQn);
}

/************************************
* SSP1 Singleton					*
************************************/

SSP1 & SSP1::instance (void) {
	static SSP1 ssp;
	return ssp;
}

SSP1::SSP1 (void) : SSP(1) {
	handleInterruptPointer[2] = handleInterrupt;
}

void SSP1::handleInterrupt (void) {
	instance().handle();
}

void SSP1::initialize (Clock::PeripheralClockSpeed clock, uint32_t spi_clock_frequency, uint32_t bits, bool clock_phase_2nd_edge, bool clock_polarity_active_low) {
	Clock::enablePeripheral(Clock::PeripheralPower::ssp_1_power);
	Clock::setPeripheralClock(Clock::PeripheralClock::ssp_1_clock, clock);
	uint32_t frequency = Clock::getPeripheralClockFrequency(Clock::PeripheralClock::ssp_1_clock);

	// FIXME: implement pin initialization!

	uint32_t mode = SSP::mode(bits, clock_phase_2nd_edge, clock_polarity_active_low);
	SSP::initialize(frequency, spi_clock_frequency, mode);
	System::Interrupt::enable(SSP1_IRQn);
}
