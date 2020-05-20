#pragma once

#include "pin.h"
#include "clock.h"

namespace System {

	/************************************
	* I2C Base Implementation			*
	************************************/
	class I2C {

	public:
		typedef enum {
			standard,
			fast_mode,
			fast_mode_plus
		} ModeSelection;

	private:
		LPC_I2C_TypeDef * _lpc_i2c;
		volatile bool busy;
		uint8_t slave_address;
		volatile uint8_t * tx_buffer;
		volatile uint8_t tx_length;
		volatile uint8_t * rx_buffer;
		volatile uint8_t rx_length;

	protected:
		I2C (uint32_t instance);
		void initialize (uint32_t pin_sda, uint32_t pin_scl, System::Pin::Function function, uint32_t peripheral_frequency, ModeSelection mode_selection);
		void handle (void);

	public:
		bool isBusy (void);
		bool startTransfer (uint8_t slave_address, uint8_t * tx_buffer, uint8_t tx_length, uint8_t * rx_buffer, uint8_t rx_length);
	};

	/************************************
	* I2C0 Singleton					*
	************************************/
	class I2C0 : public I2C
	{
	private:
		I2C0 (void);
		I2C0 (I2C0 const&) = delete;
		void operator= (I2C0 const&) = delete;
		static void handleInterrupt (void);
	public:
		static I2C0 & instance (void);
		void initialize (System::Clock::PeripheralClockSpeed clock = System::Clock::PeripheralClockSpeed::cpu_divide_by_4,
						ModeSelection mode_selection = standard);
	};

	/************************************
	* I2C1 Singleton					*
	************************************/
	class I2C1 : public I2C
	{
	public:
		typedef enum {
			p0_0_and_p0_1,
			p0_19_and_p0_20,
		} PinSelection;

	private:
		I2C1 (void);
		I2C1 (I2C1 const&) = delete;
		void operator= (I2C1 const&) = delete;
		static void handleInterrupt (void);
	public:
		static I2C1 & instance (void);
		void initialize (System::Clock::PeripheralClockSpeed clock = System::Clock::PeripheralClockSpeed::cpu_divide_by_4,
						ModeSelection mode_selection = standard,
						PinSelection pin_selection = p0_0_and_p0_1);
	};

	/************************************
	* I2C2 Singleton					*
	************************************/
	class I2C2 : public I2C
	{
	private:
		I2C2 (void);
		I2C2 (I2C2 const&) = delete;
		void operator= (I2C2 const&) = delete;
		static void handleInterrupt (void);
	public:
		static I2C2 & instance (void);
		void initialize (System::Clock::PeripheralClockSpeed clock = System::Clock::PeripheralClockSpeed::cpu_divide_by_4,
						ModeSelection mode_selection = standard);
	};
}
