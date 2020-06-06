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

	void UART::initialize (uint32_t pin_txd_index, GPIO::Function function, uint32_t peripheral_frequency, uint32_t baudrate, uint8_t mode) {

		// Init TXD pin
		GPIOPin pin_txd(pin_txd_index);
		pin_txd.setFunction(function);
		pin_txd.setPullMode(Pin::PullMode::no_pull);
		pin_txd.setOpenDrain(false);

		// Init SCL pin
		GPIOPin pin_rxd(pin_txd_index + 1);
		pin_rxd.setFunction(function);
		pin_rxd.setPullMode(Pin::PullMode::no_pull);
		pin_rxd.setOpenDrain(false);

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
		configureReceiveDMA(_rx_dma_handle);
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
		configureTransmitDMA(_tx_dma_handle);
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

void UART0::configureReceiveDMA (DMA * dma) {
	dma->configure(
			DMA::TransferType::peripheral_to_memory,
			DMA::Peripheral::uart0_rx, DMA::Peripheral::unused,
			DMA::BurstSize::transfer_1, DMA::BurstSize::transfer_1,
			DMA::TransferWidth::byte, DMA::TransferWidth::byte,
			false, true);
}

void UART0::configureTransmitDMA (DMA * dma) {
	dma->configure(
			DMA::TransferType::memory_to_peripheral,
			DMA::Peripheral::unused, DMA::Peripheral::uart0_tx,
			DMA::BurstSize::transfer_1, DMA::BurstSize::transfer_1,
			DMA::TransferWidth::byte, DMA::TransferWidth::byte,
			true, false);
}

void UART0::initialize (Clock::PeripheralClockSpeed clock, uint32_t baudrate, UART::CharacterLength character_length, UART::StopBits stop_bits, UART::Parity parity, bool enable_break_control) {
	Clock::enablePeripheral(Clock::PeripheralPower::uart_0_power);
	Clock::setPeripheralClock(Clock::PeripheralClock::uart_0_clock, clock);
	uint32_t frequency = Clock::getPeripheralClockFrequency(Clock::PeripheralClock::uart_0_clock);

	uint32_t pin_txd = PIN(0, 2);
	uint8_t mode = UART::mode(character_length, stop_bits, parity, enable_break_control);
	UART::initialize(pin_txd, GPIO::Function::alternate_1, frequency, baudrate, mode);
	System::Interrupt::enable(UART0_IRQn);
}

/************************************
* UART1 Singleton					*
************************************/

UART1 & UART1::instance (void) {
	static UART1 uart;
	return uart;
}

UART1::UART1 (void) : UART(1) {
	handleInterruptPointer[1] = handleInterrupt;
}

void UART1::handleInterrupt (void) {
	instance().handle();
}

void UART1::configureReceiveDMA (DMA * dma) {
	dma->configure(
			DMA::TransferType::peripheral_to_memory,
			DMA::Peripheral::uart1_rx, DMA::Peripheral::unused,
			DMA::BurstSize::transfer_1, DMA::BurstSize::transfer_1,
			DMA::TransferWidth::byte, DMA::TransferWidth::byte,
			false, true);
}

void UART1::configureTransmitDMA (DMA * dma) {
	dma->configure(
			DMA::TransferType::memory_to_peripheral,
			DMA::Peripheral::unused, DMA::Peripheral::uart1_tx,
			DMA::BurstSize::transfer_1, DMA::BurstSize::transfer_1,
			DMA::TransferWidth::byte, DMA::TransferWidth::byte,
			true, false);
}

void UART1::initialize (Clock::PeripheralClockSpeed clock, uint32_t baudrate, UART::CharacterLength character_length, UART::StopBits stop_bits, UART::Parity parity, bool enable_break_control, PinSelection pin_selection) {
	Clock::enablePeripheral(Clock::PeripheralPower::uart_1_power);
	Clock::setPeripheralClock(Clock::PeripheralClock::uart_1_clock, clock);
	uint32_t frequency = Clock::getPeripheralClockFrequency(Clock::PeripheralClock::uart_1_clock);

	uint32_t pin_txd = PIN(0, 15);
	GPIO::Function pin_function = GPIO::Function::alternate_1;
	if (pin_selection == UART1::PinSelection::p2_0_and_p2_1) {
		pin_txd = PIN(2, 0);
		pin_function = GPIO::Function::alternate_2;
	}

	uint8_t mode = UART::mode(character_length, stop_bits, parity, enable_break_control);
	UART::initialize(pin_txd, pin_function, frequency, baudrate, mode);
	System::Interrupt::enable(UART1_IRQn);
}

/************************************
* UART2 Singleton					*
************************************/

UART2 & UART2::instance (void) {
	static UART2 uart;
	return uart;
}

UART2::UART2 (void) : UART(2) {
	handleInterruptPointer[2] = handleInterrupt;
}

void UART2::handleInterrupt (void) {
	instance().handle();
}

void UART2::configureReceiveDMA (DMA * dma) {
	dma->configure(
			DMA::TransferType::peripheral_to_memory,
			DMA::Peripheral::uart2_rx, DMA::Peripheral::unused,
			DMA::BurstSize::transfer_1, DMA::BurstSize::transfer_1,
			DMA::TransferWidth::byte, DMA::TransferWidth::byte,
			false, true);
}

void UART2::configureTransmitDMA (DMA * dma) {
	dma->configure(
			DMA::TransferType::memory_to_peripheral,
			DMA::Peripheral::unused, DMA::Peripheral::uart2_tx,
			DMA::BurstSize::transfer_1, DMA::BurstSize::transfer_1,
			DMA::TransferWidth::byte, DMA::TransferWidth::byte,
			true, false);
}

void UART2::initialize (Clock::PeripheralClockSpeed clock, uint32_t baudrate, UART::CharacterLength character_length, UART::StopBits stop_bits, UART::Parity parity, bool enable_break_control, PinSelection pin_selection) {
	Clock::enablePeripheral(Clock::PeripheralPower::uart_2_power);
	Clock::setPeripheralClock(Clock::PeripheralClock::uart_2_clock, clock);
	uint32_t frequency = Clock::getPeripheralClockFrequency(Clock::PeripheralClock::uart_2_clock);

	uint32_t pin_txd = PIN(0, 10);
	GPIO::Function pin_function = GPIO::Function::alternate_1;
	if (pin_selection == UART2::PinSelection::p2_8_and_p2_9) {
		pin_txd = PIN(2, 8);
		pin_function = GPIO::Function::alternate_2;
	}

	uint8_t mode = UART::mode(character_length, stop_bits, parity, enable_break_control);
	UART::initialize(pin_txd, pin_function, frequency, baudrate, mode);
	System::Interrupt::enable(UART2_IRQn);
}

/************************************
* UART3 Singleton					*
************************************/

UART3 & UART3::instance (void) {
	static UART3 uart;
	return uart;
}

UART3::UART3 (void) : UART(3) {
	handleInterruptPointer[3] = handleInterrupt;
}

void UART3::handleInterrupt (void) {
	instance().handle();
}

void UART3::configureReceiveDMA (DMA * dma) {
	dma->configure(
			DMA::TransferType::peripheral_to_memory,
			DMA::Peripheral::uart3_rx, DMA::Peripheral::unused,
			DMA::BurstSize::transfer_1, DMA::BurstSize::transfer_1,
			DMA::TransferWidth::byte, DMA::TransferWidth::byte,
			false, true);
}

void UART3::configureTransmitDMA (DMA * dma) {
	dma->configure(
			DMA::TransferType::memory_to_peripheral,
			DMA::Peripheral::unused, DMA::Peripheral::uart3_tx,
			DMA::BurstSize::transfer_1, DMA::BurstSize::transfer_1,
			DMA::TransferWidth::byte, DMA::TransferWidth::byte,
			true, false);
}

void UART3::initialize (Clock::PeripheralClockSpeed clock, uint32_t baudrate, UART::CharacterLength character_length, UART::StopBits stop_bits, UART::Parity parity, bool enable_break_control, PinSelection pin_selection) {
	Clock::enablePeripheral(Clock::PeripheralPower::uart_3_power);
	Clock::setPeripheralClock(Clock::PeripheralClock::uart_3_clock, clock);
	uint32_t frequency = Clock::getPeripheralClockFrequency(Clock::PeripheralClock::uart_3_clock);

	uint32_t pin_txd = PIN(0, 0);
	GPIO::Function pin_function = GPIO::Function::alternate_2;
	if (pin_selection == UART3::PinSelection::p0_25_and_p0_26) {
		pin_txd = PIN(0, 25);
		pin_function = GPIO::Function::alternate_3;
	} else if (pin_selection == UART3::PinSelection::p4_28_and_p4_29) {
		pin_txd = PIN(4, 28);
		pin_function = GPIO::Function::alternate_3;
	}

	uint8_t mode = UART::mode(character_length, stop_bits, parity, enable_break_control);
	UART::initialize(pin_txd, pin_function, frequency, baudrate, mode);
	System::Interrupt::enable(UART3_IRQn);
}
