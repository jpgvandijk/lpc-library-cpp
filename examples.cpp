// https://stackoverflow.com/questions/5793334/c-static-vs-namespace-vs-singleton

// Includes
#include "LPC17xx.h"
#include "clock.h"
#include "time.h"
#include "pin.h"
#include "uart.h"
#include "dma.h"
#include "i2c.h"

// Namespaces
using namespace System;

/***************************************************
* Clock, Time, Pin
***************************************************/

void init (void) {
	Clock::enableMainOscillator(12000000);
	Clock::useSystemClock(Clock::Source::main_oscillator);
	Clock::connectSystemPLL(20, 1, 4);
	Time::start();
}

void blink (void) {

	// Quickly blink 5 times, then pause
	for (uint32_t i = 0; i < 5; i++) {
		Pin::set(PIN(0, 22));
		Time::delay(25);
		Pin::clear(PIN(0, 22));
		Time::delay(25);
	}
	Time::delay(200);
}

void test_clock_time_pin (void) {

	// Simple GPIO blinky
	Pin::setDirection(PIN(0, 22), Pin::Direction::output);

	// Verify time/delay while changing the CPU frequency
	Time::start();
	volatile uint32_t timestamp = Time::tic();

	// Enable/disable the oscillator and PLL
	Clock::enableMainOscillator(12000000);
	blink();
	Clock::useSystemClock(Clock::Source::main_oscillator);
	blink();
	Clock::connectSystemPLL(20, 1, 4);
	blink();
	Clock::disconnectSystemPLL();
	blink();
	Clock::disableMainOscillator();
	blink();

	// Record the time passed
	timestamp = Time::toc(timestamp);
}

/***************************************************
* UART, DMA
***************************************************/

void test_uart0_dma (void) {
	init();

	// The use of DMA is optional (the last argument in receive() and transmit() should be removed to not use the DMA)
	DMA::enable();
	UART0::instance().initialize();

	// Receive
	uint8_t rx_buffer[16];
	UART0::instance().receive(rx_buffer, sizeof(rx_buffer), DMAChannel<DMA::ch_0>::instance());

	// Transmit
	uint8_t tx_buffer[] = "Hello World!\r\n";
	UART0::instance().transmit(tx_buffer, sizeof(tx_buffer), DMAChannel<DMA::ch_1>::instance());

	while (true) {

		// Make sure not to overwrite the TX buffer while still transmitting
		while (UART0::instance().isTransmitting()) {}

		// Data available?
		uint32_t bytes_available = UART0::instance().bytesAvailable();
		if (bytes_available != 0) {

			// Echo
			for (uint32_t i = 0; i < bytes_available; i++) {
				tx_buffer[i] = UART0::instance().getChar();
			}
			UART0::instance().transmit(tx_buffer, bytes_available, DMAChannel<DMA::ch_1>::instance());
		}

		// Pause, so we can accumulate data in the receive ring buffer
		Time::delay(200);
	}
}

void test_uart1 (void) {
	init();
	UART1::instance().initialize(System::Clock::PeripheralClockSpeed::cpu_divide_by_1,
			115200, UART::CharacterLength::char_8b, UART::StopBits::stop_1, UART::Parity::none, false);

	// Receive
	uint8_t rx_buffer[16];
	UART1::instance().receive(rx_buffer, sizeof(rx_buffer));

	// Transmit
	uint8_t tx_buffer[] = "Hello World!\r\n";
	UART1::instance().transmit(tx_buffer, sizeof(tx_buffer));

	while (true) {

		// Make sure not to overwrite the TX buffer while still transmitting
		while (UART1::instance().isTransmitting()) {}

		// Data available?
		uint32_t bytes_available = UART1::instance().bytesAvailable();
		if (bytes_available != 0) {

			// Echo
			for (uint32_t i = 0; i < bytes_available; i++) {
				tx_buffer[i] = UART1::instance().getChar();
			}
			UART1::instance().transmit(tx_buffer, bytes_available);
		}

		// Pause, so we can accumulate data in the receive ring buffer
		Time::delay(200);
	}
}

/***************************************************
* I2C
***************************************************/

void test_i2c0 (void) {
	init();

	// I/O Extender MCP23017 over I2C0 at 1 MHz
	I2C0::instance().initialize(Clock::PeripheralClockSpeed::cpu_divide_by_2, I2C::Mode::fast_mode_plus);
	uint8_t slave_address = 0x4E;

	// IODIRA = 0
	while (I2C0::instance().isBusy()) {}
	uint8_t tx_buffer[] = {0x00, 0x00};
	I2C0::instance().startTransfer(slave_address, tx_buffer, 2, nullptr, 0);

	while (true) {

		// Pause
		Time::delay(25);

		// OLATA = 0x00
		while (I2C0::instance().isBusy()) {}
		tx_buffer[0] = 0x14;
		tx_buffer[1] = 0x00;
		I2C0::instance().startTransfer(slave_address, tx_buffer, 2, nullptr, 0);

		// Pause
		Time::delay(25);

		// OLATA = 0xFF
		while (I2C0::instance().isBusy()) {}
		tx_buffer[0] = 0x14;
		tx_buffer[1] = 0xFF;
		I2C0::instance().startTransfer(slave_address, tx_buffer, 2, nullptr, 0);
	}
}

void test_i2c1 (void) {
	init();

	// EEPROM 24LC64 over I2C1 at 100 kHz
	I2C1::instance().initialize(Clock::PeripheralClockSpeed::cpu_divide_by_2, I2C::Mode::standard, I2C1::PinSelection::p0_19_and_p0_20);
	uint8_t slave_address = 0xA0;

	// Write 4 bytes at address 0
	while (I2C1::instance().isBusy()) {}
	uint8_t tx_buffer[] = {0x00, 0x00, 0x31, 0x32, 0x33, 0x34};
	I2C1::instance().startTransfer(slave_address, tx_buffer, 6, nullptr, 0);

	// Give it plenty time to finish programming
	Time::delay(100);

	// Read back the 4 bytes from address 0
	while (I2C1::instance().isBusy()) {}
	uint8_t rx_buffer[] = {0, 0, 0, 0};
	I2C1::instance().startTransfer(slave_address, tx_buffer, 2, rx_buffer, 4);

	// Wait for the I2C transaction to finish
	while (I2C1::instance().isBusy()) {}
}

int main(void) {

	//test_clock_time_pin();
	//test_uart0_dma();
	test_uart1();
	//test_i2c0();
	//test_i2c1();

    return 0;
}
