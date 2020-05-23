#pragma once

#include "pin.h"
#include "clock.h"

namespace System {

	/************************************
	* UART Base Implementation			*
	************************************/
	class UART {

	public:
		typedef enum {
			char_5b = 0,
			char_6b = 1,
			char_7b = 2,
			char_8b = 3
		} CharacterLength;

		typedef enum {
			stop_1 = 0,
			stop_1_5 = 1,
			stop_2 = 1
		} StopBits;

		typedef enum {
			none = 0,
			odd = 1,
			even = 3,
			high = 5,
			low = 7,
		} Parity;

	private:
		LPC_UART_TypeDef * _lpc_uart;
		volatile bool busy;
		volatile uint8_t * tx_buffer;
		volatile uint8_t tx_length;

	protected:
		UART (uint32_t instance);
		void initialize (uint32_t pin_txd, Pin::Function function, uint32_t peripheral_frequency, uint32_t baudrate, uint8_t mode);
		void handle (void);
		uint8_t mode (CharacterLength character_length, StopBits stop_bits, Parity parity, bool enable_break_control);
		void setBaudrate (uint32_t peripheral_frequency, uint32_t baudrate);

	public:
		bool isBusy (void);
		bool write (uint8_t * tx_buffer, uint8_t tx_length);
	};

	/************************************
	* UART0 Singleton					*
	************************************/
	class UART0 : public UART
	{
	private:
		UART0 (void);
		UART0 (UART0 const&) = delete;
		void operator= (UART0 const&) = delete;
		static void handleInterrupt (void);

	public:
		static UART0 & instance (void);
		void initialize (System::Clock::PeripheralClockSpeed clock = System::Clock::PeripheralClockSpeed::cpu_divide_by_4,
						uint32_t baudrate = 9600,
						UART::CharacterLength character_length = UART::CharacterLength::char_8b,
						UART::StopBits stop_bits = UART::StopBits::stop_1,
						UART::Parity parity = UART::Parity::none,
						bool enable_break_control = false);
	};

	/************************************
	* UART1 Singleton					*
	************************************/
	class UART1 : public UART
	{
	private:
		UART1 (void);
		UART1 (UART1 const&) = delete;
		void operator= (UART1 const&) = delete;
		static void handleInterrupt (void);
	public:
		static UART1 & instance (void);
		void initialize (System::Clock::PeripheralClockSpeed clock = System::Clock::PeripheralClockSpeed::cpu_divide_by_4);
	};

	/************************************
	* UART2 Singleton					*
	************************************/
	class UART2 : public UART
	{
	private:
		UART2 (void);
		UART2 (UART2 const&) = delete;
		void operator= (UART2 const&) = delete;
		static void handleInterrupt (void);
	public:
		static UART2 & instance (void);
		void initialize (System::Clock::PeripheralClockSpeed clock = System::Clock::PeripheralClockSpeed::cpu_divide_by_4);
	};

	/************************************
	* UART3 Singleton					*
	************************************/
	class UART3 : public UART
	{
	private:
		UART3 (void);
		UART3 (UART3 const&) = delete;
		void operator= (UART3 const&) = delete;
		static void handleInterrupt (void);
	public:
		static UART3 & instance (void);
		void initialize (System::Clock::PeripheralClockSpeed clock = System::Clock::PeripheralClockSpeed::cpu_divide_by_4);
	};
}
