// Includes
#include "LPC17xx.h"
#include "core_cm3.h"
#include "time.h"
#include "clock.h"

// Namespaces
using namespace System;

namespace System::Time {

	namespace {

		const uint32_t _interrupts_per_second = 100;
		volatile uint32_t _tick;

		void _setReloadValue (uint32_t cpu_frequency) {
			SysTick->LOAD = (cpu_frequency / _interrupts_per_second) - 1;
		}
	}

	void _increment_tick (void) {
		_tick++;
	}

	void start (void) {

		// Set the proper reload value and make sure it will get updated if needed
		_setReloadValue(Clock::getCPUFrequency());
		Clock::attachHandler(_setReloadValue);

		// Enable the SysTick clock with interrupt
		SysTick->VAL   = 0;
		SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
						 SysTick_CTRL_TICKINT_Msk   |
						 SysTick_CTRL_ENABLE_Msk;
	}

	uint32_t tic (void) {
		return _tick;
	}

	uint32_t toc (uint32_t tic) {
		return ((_tick - tic) * 1000) / _interrupts_per_second;
	}

	void delay (uint32_t ms) {
		uint32_t timestamp = _tick;
		uint32_t difference = (ms * _interrupts_per_second) / 1000;
		while ((_tick - timestamp) < difference) {}
	}
}

extern "C" {
	void SysTick_Handler (void) {
		Time::_increment_tick();
	}
}
