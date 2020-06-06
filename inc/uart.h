#pragma once

#include "pin.h"
#include "clock.h"
#include "dma.h"

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

		// UART RX
		DMA * _rx_dma_handle;
		uint8_t * _rx_buffer;
		uint16_t _rx_buffer_size;
		uint16_t _rx_read_index;
		uint16_t _rx_write_index;

		// UART TX
		DMA * _tx_dma_handle;
		uint8_t * _tx_buffer;
		volatile uint16_t _tx_length;
		volatile bool _tx_busy;

	private:
		virtual void configureReceiveDMA (DMA * dma) {};
		virtual void configureTransmitDMA (DMA * dma) {};

	protected:
		UART (uint32_t instance);
		void initialize (uint32_t pin_txd_index, GPIO::Function function, uint32_t peripheral_frequency, uint32_t baudrate, uint8_t mode);
		uint8_t mode (CharacterLength character_length, StopBits stop_bits, Parity parity, bool enable_break_control);
		void setBaudrate (uint32_t peripheral_frequency, uint32_t baudrate);
		void handle (void);

	public:

		// UART RX (optionally with DMA)
		void receive (uint8_t * rx_buffer, uint16_t rx_buffer_size);
		void receive (uint8_t * rx_buffer, uint16_t rx_buffer_size, DMA & dma);
		uint16_t bytesAvailable (void);
		uint8_t getChar (void);

		// UART TX (optionally with DMA)
		bool transmit (uint8_t * tx_buffer, uint16_t tx_length);
		bool transmit (uint8_t * tx_buffer, uint16_t tx_length, DMA & dma);
		bool isTransmitting (void);
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
		void configureReceiveDMA (DMA * dma);
		void configureTransmitDMA (DMA * dma);

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
	public:
		typedef enum {
			p0_15_and_p0_16,
			p2_0_and_p2_1,
		} PinSelection;

	private:
		UART1 (void);
		UART1 (UART1 const&) = delete;
		void operator= (UART1 const&) = delete;
		static void handleInterrupt (void);
		void configureReceiveDMA (DMA * dma);
		void configureTransmitDMA (DMA * dma);

	public:
		static UART1 & instance (void);
		void initialize (System::Clock::PeripheralClockSpeed clock = System::Clock::PeripheralClockSpeed::cpu_divide_by_4,
				uint32_t baudrate = 9600,
				UART::CharacterLength character_length = UART::CharacterLength::char_8b,
				UART::StopBits stop_bits = UART::StopBits::stop_1,
				UART::Parity parity = UART::Parity::none,
				bool enable_break_control = false,
				PinSelection pin_selection = p0_15_and_p0_16);
	};

	/************************************
	* UART2 Singleton					*
	************************************/
	class UART2 : public UART
	{
	public:
		typedef enum {
			p0_10_and_p0_11,
			p2_8_and_p2_9,
		} PinSelection;

	private:
		UART2 (void);
		UART2 (UART2 const&) = delete;
		void operator= (UART2 const&) = delete;
		static void handleInterrupt (void);
		void configureReceiveDMA (DMA * dma);
		void configureTransmitDMA (DMA * dma);

	public:
		static UART2 & instance (void);
		void initialize (System::Clock::PeripheralClockSpeed clock = System::Clock::PeripheralClockSpeed::cpu_divide_by_4,
				uint32_t baudrate = 9600,
				UART::CharacterLength character_length = UART::CharacterLength::char_8b,
				UART::StopBits stop_bits = UART::StopBits::stop_1,
				UART::Parity parity = UART::Parity::none,
				bool enable_break_control = false,
				PinSelection pin_selection = p0_10_and_p0_11);
	};

	/************************************
	* UART3 Singleton					*
	************************************/
	class UART3 : public UART
	{
	public:
		typedef enum {
			p0_0_and_p0_1,
			p0_25_and_p0_26,
			p4_28_and_p4_29,
		} PinSelection;

	private:
		UART3 (void);
		UART3 (UART3 const&) = delete;
		void operator= (UART3 const&) = delete;
		static void handleInterrupt (void);
		void configureReceiveDMA (DMA * dma);
		void configureTransmitDMA (DMA * dma);

	public:
		static UART3 & instance (void);
		void initialize (System::Clock::PeripheralClockSpeed clock = System::Clock::PeripheralClockSpeed::cpu_divide_by_4,
				uint32_t baudrate = 9600,
				UART::CharacterLength character_length = UART::CharacterLength::char_8b,
				UART::StopBits stop_bits = UART::StopBits::stop_1,
				UART::Parity parity = UART::Parity::none,
				bool enable_break_control = false,
				PinSelection pin_selection = p0_0_and_p0_1);
	};
}
