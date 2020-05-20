// Includes
#include "LPC17xx.h"
#include "clock.h"

// Namespaces
using namespace System;

// Definitions
#ifndef CLOCK_MAXIMUM_NUMBER_OF_HANDLERS
	#define CLOCK_MAXIMUM_NUMBER_OF_HANDLERS	4
#endif

namespace System::Clock {

	namespace {

		const uint32_t _frequency_rc_oscillator = 4000000;
		uint32_t _frequency_main_oscillator = 0;
		uint32_t _cpu_frequency = _frequency_rc_oscillator;
		uint8_t _number_of_registered_handlers = 0;
		void (*_registered_handlers[CLOCK_MAXIMUM_NUMBER_OF_HANDLERS])(uint32_t cpu_frequency);

		inline void _feedSystemPLL (void) {
			LPC_SC->PLL0FEED = 0xAA;
			LPC_SC->PLL0FEED = 0x55;
		}

		void _disconnectSystemPLL (void) {
			LPC_SC->PLL0CON = 0;
			_feedSystemPLL();
		}

		void _configureSystemPLL (uint32_t multiplier, uint32_t divider) {
			LPC_SC->PLL0CFG = ((multiplier - 1) << 0) + ((divider - 1) << 16);
			_feedSystemPLL();
		}

		void _enableSystemPLL (void) {
			LPC_SC->PLL0CON = (1 << 0);
			_feedSystemPLL();
		}

		void _connectSystemPLL (void) {
			LPC_SC->PLL0CON = (1 << 0) + (1 << 1);
			_feedSystemPLL();
		}

		uint32_t _getSystemFrequency (void) {
			Clock::Source source = (Clock::Source) LPC_SC->CLKSRCSEL;
			if (source == Clock::Source::internal_rc_oscillator)
				return _frequency_rc_oscillator;
			else /* (source == Source::main_oscillator) */
				return _frequency_main_oscillator;
		}

		void _setFlashAccessTime (uint32_t cpu_frequency) {
			uint32_t setting = (cpu_frequency - 1) / 20000000;

			// 5 CPU cycles are sufficient under any conditions for the LPC1769!
			if (setting > 5) {
				setting = 5;
			}

			LPC_SC->FLASHCFG = (setting << 12);
		}

		void _notifyHandlers (uint32_t cpu_frequency) {

			// One local handler!
			_setFlashAccessTime(cpu_frequency);

			for (uint8_t i = 0; i < _number_of_registered_handlers; i++) {
				_registered_handlers[i](cpu_frequency);
			}
		}

	}

	uint32_t getCPUFrequency (void) {
		return _cpu_frequency;
	}

	void enableMainOscillator (uint32_t frequency) {
		_frequency_main_oscillator = frequency;

		// Enable the main oscillator
		if (frequency < 15000000) {
			LPC_SC->SCS = (0 << 4) + (1 << 5);
		} else {
			LPC_SC->SCS = (1 << 4) + (1 << 5);
		}

		// Wait for the main oscillator to be ready
		while ((LPC_SC->SCS & (1 << 6)) == 0);
	}

	void disableMainOscillator (void) {
		_frequency_main_oscillator = 0;

		// Is the main oscillator selected as clock source? Switch back to the internal RC oscillator!
		if (LPC_SC->CLKSRCSEL == (uint32_t) Clock::Source::main_oscillator) {
			useSystemClock(Clock::Source::internal_rc_oscillator);
		}

		// Disable the main oscillator
		LPC_SC->SCS = 0;
	}

	void useSystemClock (Clock::Source source, uint32_t cpu_divider) {

		// Parse the settings
		if ((cpu_divider < 1) || (cpu_divider > 256))
			return;
		if ((source == Clock::Source::main_oscillator) && (_frequency_main_oscillator == 0))
			return;

		// Make sure PLL0 is disconnected
		if (Clock::isSystemPLLConnected()) {
			_disconnectSystemPLL();
		}

		// Select the clock source
		LPC_SC->CLKSRCSEL = (uint32_t) source;

		// Configure the CPU clock divider
		LPC_SC->CCLKCFG = ((cpu_divider - 1) << 0);

		// Calculate the CPU frequency
		_cpu_frequency = _getSystemFrequency() / cpu_divider;
		_notifyHandlers(_cpu_frequency);
	}

	bool isSystemPLLConnected (void) {
		return (LPC_SC->PLL0CON & (1 << 1));
	}

	void disconnectSystemPLL (uint32_t cpu_divider) {
		_disconnectSystemPLL();

		// Configure the CPU clock divider
		LPC_SC->CCLKCFG = ((cpu_divider - 1) << 0);

		// Recalculate the CPU frequency
		_cpu_frequency = _getSystemFrequency() / cpu_divider;
		_notifyHandlers(_cpu_frequency);
	}

	void connectSystemPLL (uint32_t multiplier, uint32_t divider, uint32_t cpu_divider) {

		// Parse the settings
		if ((multiplier < 6) || (multiplier > 512))
			return;
		if ((divider < 1) || (divider > 32))
			return;
		if ((cpu_divider < 1) || (cpu_divider > 256))
			return;

		// Make sure PLL0 is disconnected
		if (isSystemPLLConnected()) {
			_disconnectSystemPLL();
		}

		// Configure and enable PLL0
		_configureSystemPLL(multiplier, divider);
		_enableSystemPLL();

		// Configure the CPU clock divider
		LPC_SC->CCLKCFG = ((cpu_divider - 1) << 0);

		// Calculate the CPU frequency
		uint32_t pll_frequency = (2 * _getSystemFrequency()) / (divider * cpu_divider);
		_cpu_frequency = pll_frequency * multiplier;
		_notifyHandlers(_cpu_frequency);

		// Wait for PLL0 to be stable, then connect it
		while ((LPC_SC->PLL0STAT & (1 << 26)) == 0);
		_connectSystemPLL();
	}

	void attachHandler (void (*handler)(uint32_t cpu_frequency)) {
		if (_number_of_registered_handlers < CLOCK_MAXIMUM_NUMBER_OF_HANDLERS) {
			_registered_handlers[_number_of_registered_handlers] = handler;
			_number_of_registered_handlers++;
		}
	}

	void setPeripheralClock (PeripheralClock peripheral, PeripheralClockSpeed clock) {
		uint32_t index = ((uint32_t) peripheral) << 1;
		if (index >= 32) {
			index -= 32;
			LPC_SC->PCLKSEL1 = (LPC_SC->PCLKSEL1 & ~(3 << index)) | (((uint32_t) clock) << index);
		} else {
			LPC_SC->PCLKSEL0 = (LPC_SC->PCLKSEL0 & ~(3 << index)) | (((uint32_t) clock) << index);
		}
	}

	uint32_t getPeripheralClockFrequency (PeripheralClock peripheral) {
		uint32_t index = ((uint32_t) peripheral) << 1;
		PeripheralClockSpeed clock = Clock::PeripheralClockSpeed::cpu_divide_by_4;
		if (index >= 32) {
			index -= 32;
			clock = (Clock::PeripheralClockSpeed)((LPC_SC->PCLKSEL1 >> index) & 3);
		} else {
			clock = (Clock::PeripheralClockSpeed)((LPC_SC->PCLKSEL0 >> index) & 3);
		}

		uint32_t cpu_frequency = getCPUFrequency();
		if (clock == Clock::PeripheralClockSpeed::cpu_divide_by_1) {
			return cpu_frequency / 1;
		} else if (clock == Clock::PeripheralClockSpeed::cpu_divide_by_2) {
			return cpu_frequency / 2;
		} else if (clock == Clock::PeripheralClockSpeed::cpu_divide_by_4) {
			return cpu_frequency / 4;
		} else /* (clock == Clock::PeripheralClockSpeed::cpu_divide_by_6_or_8) */ {
			if ((peripheral == Clock::PeripheralClock::can_1_clock) ||
				(peripheral == Clock::PeripheralClock::can_2_clock) ||
				(peripheral == Clock::PeripheralClock::can_filter_clock)) {
				return cpu_frequency / 6;
			} else {
				return cpu_frequency / 8;
			}
		}
	}

	void enablePeripheral (PeripheralPower peripheral) {
		LPC_SC->PCONP |= (1 << ((uint32_t) peripheral));
	}

	void disablePeripheral (PeripheralPower peripheral) {
		LPC_SC->PCONP &= ~(1 << ((uint32_t) peripheral));
	}
}
