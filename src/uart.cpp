// Includes
#include "LPC17xx.h"
#include "uart.h"
#include "clock.h"
#include "pin.h"
#include "interrupt.h"
#include "dma.h"

// Namespaces
using namespace System;

// TODO: register for CPU clock frequency updates!

/************************************
* UART Interrupt Handlers			*
************************************/

namespace {
	void (*handleInterruptPointer[4]) (void) = {nullptr, nullptr, nullptr, nullptr};
}

extern "C" {

	void UART0_IRQHandler (void) {
		if (handleInterruptPointer[0] != nullptr) {
			handleInterruptPointer[0]();
		}
	}

	void UART1_IRQHandler (void) {
		if (handleInterruptPointer[1] != nullptr) {
			handleInterruptPointer[1]();
		}
	}

	void UART2_IRQHandler (void) {
		if (handleInterruptPointer[2] != nullptr) {
			handleInterruptPointer[2]();
		}
	}

	void UART3_IRQHandler (void) {
		if (handleInterruptPointer[3] != nullptr) {
			handleInterruptPointer[3]();
		}
	}
}

namespace System {

	/************************************
	* UART Base Implementation			*
	************************************/

	UART::UART (uint32_t instance) {
		if (instance == 0) {
			_lpc_uart = (LPC_UART_TypeDef*) LPC_UART0;
		} else if (instance == 1) {
			_lpc_uart = (LPC_UART_TypeDef*) LPC_UART1;
		} else if (instance == 2) {
			_lpc_uart = LPC_UART2;
		} else { // (instance == 3)
			_lpc_uart = LPC_UART3;
		}

		_tx_dma_handle = nullptr;
		_rx_dma_handle = nullptr;
		_tx_busy = false;
	}

	void UART::initialize (uint32_t pin_txd, Pin::Function function, uint32_t peripheral_frequency, uint32_t baudrate, uint8_t mode) {

		// Init TXD pin
		Pin::setFunction(pin_txd, function);
		Pin::setPullMode(pin_txd, Pin::PullMode::no_pull);
		Pin::setOpenDrain(pin_txd, false);

		// Init SCL pin
		uint32_t pin_rxd = pin_txd + 1;
		Pin::setFunction(pin_rxd, function);
		Pin::setPullMode(pin_rxd, Pin::PullMode::no_pull);
		Pin::setOpenDrain(pin_rxd, false);

		// Set the desired baudrate
		setBaudrate(peripheral_frequency, baudrate);

		// Enable the UART interface
		_lpc_uart->LCR = mode;
		_lpc_uart->FCR = (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0);
		_lpc_uart->IER = 0;
	}

	uint8_t UART::mode (UART::CharacterLength character_length, UART::StopBits stop_bits, UART::Parity parity, bool enable_break_control) {
		uint8_t mode = (((uint8_t) character_length) << 0) | (((uint8_t) stop_bits) << 2) | (((uint8_t) parity) << 3);
		if (enable_break_control) {
			mode |= (1 << 6);
		}
		return mode;
	}

	void UART::setBaudrate (uint32_t peripheral_frequency, uint32_t baudrate) {

		/********************************************************
		* Baudrate Calculation									*
		* q = (256 * UnDLM + UnDLL) * (MULVAL + DIVADDVAL)		*
		********************************************************/
		baudrate = 16 * baudrate;

		// Try all fractional multiplier settings to determine the closest match
		uint32_t min_error = 0xFFFFFFFF;
		uint32_t min_error_multiplier = 1;
		for (uint32_t multiplier = 1; multiplier < 16; multiplier++) {
			uint32_t multiplied_frequency = peripheral_frequency * multiplier;
			uint32_t q = (multiplied_frequency + (baudrate >> 1)) / baudrate;

			q = q * baudrate;
			uint32_t error;
			if (q > multiplied_frequency) {
				error = q - multiplied_frequency;
			} else {
				error = multiplied_frequency - q;
			}

			if (error < min_error) {
				min_error = error;
				min_error_multiplier = multiplier;
				if (error == 0) {
					break;
				}
			}
		}
		uint32_t multiplied_frequency = peripheral_frequency * min_error_multiplier;
		uint32_t q = (multiplied_frequency + (baudrate >> 1)) / baudrate;

		// Now find the best divider setting
		min_error = 0xFFFFFFFF;
		uint32_t min_error_divider = 0;
		for (uint32_t divider = 0; divider < min_error_multiplier; divider++) {
			uint32_t sum = min_error_multiplier + divider;
			uint32_t dl = (q + (sum >> 1)) / sum;

			sum = sum * dl;
			uint32_t error;
			if (sum > q) {
				error = sum - q;
			} else {
				error = q - sum;
			}

			if (error < min_error) {
				min_error = error;
				min_error_divider = divider;
				if (error == 0) {
					break;
				}
			}
		}

		// Fraction not required?
		_lpc_uart->LCR |= (1 << 7);
		if (min_error_divider == 0) {
			uint32_t dl = (peripheral_frequency + (baudrate >> 1)) / baudrate;
			_lpc_uart->FDR = (1 << 4) | (0 << 0);
			_lpc_uart->DLL = (dl >> 0) & 0xFF;
			_lpc_uart->DLM = (dl >> 8) & 0xFF;
		} else {
			_lpc_uart->FDR = (min_error_multiplier << 4) | (min_error_divider << 0);
			uint32_t sum = min_error_multiplier + min_error_divider;
			uint32_t dl = (q + (sum >> 1)) / sum;
			_lpc_uart->DLL = (dl >> 0) & 0xFF;
			_lpc_uart->DLM = (dl >> 8) & 0xFF;
		}
		_lpc_uart->LCR &= ~(1 << 7);
	}

	void UART::handle (void) {

		// Handle each pending interrupt
		uint32_t interrupt_status = _lpc_uart->IIR;
		while ((interrupt_status & (1 << 0)) == 0) {

			// Line status error
			if ((interrupt_status & (7 << 1)) == (3 << 1)) {
				volatile uint32_t error = _lpc_uart->LSR;
			}

			// Receive data available or character time-out
			if (((interrupt_status & (7 << 1)) == (2 << 1)) || ((interrupt_status & (7 << 1)) == (6 << 1))) {
				_rx_buffer[_rx_write_index] = _lpc_uart->RBR;
				_rx_write_index = (_rx_write_index + 1) % _rx_buffer_size;
			}

			// Transmit buffer empty
			if ((interrupt_status & (7 << 1)) == (1 << 1)) {
				if (_tx_length > 0) {
					_lpc_uart->THR = *_tx_buffer;
					_tx_buffer++;
					_tx_length--;
				} else {
					_tx_busy = false;
				}
			}

			// Check for more interrupts
			interrupt_status = _lpc_uart->IIR;
		}
	}

	void UART::receive (uint8_t * rx_buffer, uint16_t rx_buffer_size) {
		_rx_buffer = rx_buffer;
		_rx_buffer_size = rx_buffer_size;
		_rx_read_index = 0;
		_rx_write_index = 0;

		// Enable interrupts, no DMA
		_lpc_uart->IER |= (1 << 0);
		_rx_dma_handle = nullptr;
	}

	void UART::receive (uint8_t * rx_buffer, uint16_t rx_buffer_size, DMA & dma) {
		_rx_dma_handle = &dma;
		_rx_buffer = rx_buffer;
		_rx_buffer_size = rx_buffer_size;
		_rx_read_index = 0;

		// Disable interrupts
		_lpc_uart->IER &= ~(1 << 0);

		// Set up the DMA
		_rx_dma_handle->configure(
				DMA::TransferType::peripheral_to_memory,
				DMA::Peripheral::uart0_rx, DMA::Peripheral::unused,  // FIXME: UART 0 hardcoded
				DMA::BurstSize::transfer_1, DMA::BurstSize::transfer_1,
				DMA::TransferWidth::byte, DMA::TransferWidth::byte,
				false, true);
		_rx_dma_handle->transfer(&(_lpc_uart->RBR), _rx_buffer, _rx_buffer_size, true);
	}

	uint16_t UART::bytesAvailable (void) {

		// Using DMA?
		if (_rx_dma_handle != nullptr) {
			_rx_write_index = 0 - _rx_dma_handle->getNumberOfTransfersLeft();
		}
		return (_rx_buffer_size + _rx_write_index - _rx_read_index) % _rx_buffer_size;
	}

	uint8_t UART::getChar (void) {
		if (bytesAvailable() == 0) {
			return 0;
		}

		uint8_t data = _rx_buffer[_rx_read_index];
		_rx_read_index = (_rx_read_index + 1) % _rx_buffer_size;
		return data;
	}

	bool UART::transmit (uint8_t * tx_buffer, uint16_t tx_length) {

		// Check if a new transfer can be started
		if (isTransmitting())
			return false;
		if (tx_length == 0)
			return true;

		// Enable interrupts, no DMA
		_lpc_uart->IER |= (1 << 1);
		_tx_dma_handle = nullptr;

		// Set the first character, the rest is handled in the ISR
		_lpc_uart->THR = *tx_buffer;
		tx_buffer++;
		tx_length--;

		// Store all settings
		_tx_buffer = tx_buffer;
		_tx_length = tx_length;

		// The transfer was successfully started
		_tx_busy = true;
		return true;
	}

	bool UART::transmit (uint8_t * tx_buffer, uint16_t tx_length, DMA & dma) {

		// Check if a new transfer can be started
		if (isTransmitting())
			return false;
		if (tx_length == 0)
			return true;

		// Disable interrupts, use DMA
		_lpc_uart->IER &= ~(1 << 1);
		_tx_dma_handle = &dma;

		// Start DMA transfer
		_tx_dma_handle->configure(
				DMA::TransferType::memory_to_peripheral,
				DMA::Peripheral::unused, DMA::Peripheral::uart0_tx,  // FIXME: UART 0 hardcoded
				DMA::BurstSize::transfer_1, DMA::BurstSize::transfer_1,
				DMA::TransferWidth::byte, DMA::TransferWidth::byte,
				true, false);
		_tx_dma_handle->transfer(tx_buffer, &(_lpc_uart->THR), tx_length, false);

		// The transfer was successfully started
		return true;
	}

	bool UART::isTransmitting (void) {

		// Using DMA?
		if (_tx_dma_handle != nullptr) {
			_tx_busy = (_tx_dma_handle->getNumberOfTransfersLeft() != 0);
		}
		return (_tx_busy || ((_lpc_uart->LSR & (1 << 5)) == 0));
	}
}

/************************************
* UART0 Singleton					*
************************************/

UART0 & UART0::instance (void) {
	static UART0 uart;
	return uart;
}

UART0::UART0 (void) : UART(0) {
	handleInterruptPointer[0] = handleInterrupt;
}

void UART0::handleInterrupt (void) {
	instance().handle();
}

void UART0::initialize (Clock::PeripheralClockSpeed clock, uint32_t baudrate, UART::CharacterLength character_length, UART::StopBits stop_bits, UART::Parity parity, bool enable_break_control) {
	Clock::enablePeripheral(Clock::PeripheralPower::uart_0_power);
	Clock::setPeripheralClock(Clock::PeripheralClock::uart_0_clock, clock);
	uint32_t frequency = Clock::getPeripheralClockFrequency(Clock::PeripheralClock::uart_0_clock);

	uint32_t pin_txd = PIN(0, 2);
	uint8_t mode = UART::mode(character_length, stop_bits, parity, enable_break_control);
	UART::initialize(pin_txd, Pin::Function::alternate_1, frequency, baudrate, mode);
	System::Interrupt::enable(UART0_IRQn);
}



// alt 2 or alt 3 or alt 3
// TXD3: P0.0 or 0.25 or 4.28
// RXD3: P0.1 or 0.26 or 4.29

// alt 1 or alt 2
// TXD2: P0.10 or 2.8
// RXD2: P0.11 or 2.9

// alt 1 or alt 2
// TXD1: P0.15 or 2.0
// RXD1: P0.16 or 2.1
