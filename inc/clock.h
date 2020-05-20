#pragma once

#include <cstdint>

namespace System::Clock {

	// Type definitions
	typedef enum {
		internal_rc_oscillator = 0,
		main_oscillator = 1
	} Source;

	typedef enum {
		watchdog_timer_clock = 0,
		timer_0_clock = 1,
		timer_1_clock = 2,
		uart_0_clock = 3,
		uart_1_clock = 4,
		pwm_1_clock = 6,
		i2c_0_clock = 7,
		spi_clock = 8,
		ssp_1_clock = 10,
		dac_clock = 11,
		adc_clock = 12,
		can_1_clock = 13,
		can_2_clock = 14,
		can_filter_clock = 15,
		quadrature_encoder_clock = 16,
		gpio_interrupt_clock = 17,
		pin_connect_block_clock = 18,
		i2c_1_clock = 19,
		ssp_0_clock = 21,
		timer_2_clock = 22,
		timer_3_clock = 23,
		uart_2_clock = 24,
		uart_3_clock = 25,
		i2c_2_clock = 26,
		i2s_clock = 27,
		repetitive_interrupt_timer_clock = 29,
		system_control_block_clock = 30,
		motor_control_pwm_clock = 31
	} PeripheralClock;

	typedef enum {
		timer_0_power = 1,
		timer_1_power = 2,
		uart_0_power = 3,
		uart_1_power = 4,
		pwm_1_power = 6,
		i2c_0_power = 7,
		spi_power = 8,
		rtc_power = 9,
		ssp_1_power = 10,
		adc_power = 12,
		can_1_power = 13,
		can_2_power = 14,
		gpio_power = 15,
		repetitive_interrupt_timer_power = 16,
		motor_control_pwm_power = 17,
		quadrature_encoder_power = 18,
		i2c_1_power = 19,
		ssp_0_power = 21,
		timer_2_power = 22,
		timer_3_power = 23,
		uart_2_power = 24,
		uart_3_power = 25,
		i2c_2_power = 26,
		i2s_power = 27,
		dma_power = 29,
		ethernet_power = 30,
		usb_power = 31
	} PeripheralPower;

	typedef enum {
		cpu_divide_by_4 = 0,
		cpu_divide_by_1 = 1,
		cpu_divide_by_2 = 2,
		cpu_divide_by_8 = 3,
		cpu_divide_by_6_can = 3
	} PeripheralClockSpeed;

	// Function prototypes
	uint32_t getCPUFrequency (void);
	void enableMainOscillator (uint32_t frequency);
	void disableMainOscillator (void);
	void useSystemClock (Source source, uint32_t cpu_divider = 1);
	void connectSystemPLL (uint32_t multiplier, uint32_t divider, uint32_t cpu_divider);
	void disconnectSystemPLL (uint32_t cpu_divider = 1);
	bool isSystemPLLConnected (void);
	void attachHandler (void (*handler)(uint32_t cpu_frequency));
	void setPeripheralClock (PeripheralClock peripheral, PeripheralClockSpeed clock);
	uint32_t getPeripheralClockFrequency (PeripheralClock peripheral);
	void enablePeripheral (PeripheralPower peripheral);
	void disablePeripheral (PeripheralPower peripheral);
}
