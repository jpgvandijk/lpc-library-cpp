#pragma once

#include "clock.h"

namespace System {

	/************************************
	* SPI Base Implementations			*
	************************************/
	class SPI {

	private:
		volatile uint32_t * _data_register;
		volatile bool _busy;
		bool _byte;
		void * _tx_buffer;
		void * _rx_buffer;
		uint16_t _length;

	private:
		void _write (void);
		void _read (void);
		bool _transceive (void * tx_buffer, void * rx_buffer, uint16_t length, bool byte);

	protected:
		SPI (volatile uint32_t * data_register);
		void next (void);

	public:
		bool isBusy (void);

		// 8-bit implementations
		bool transmit (uint8_t * tx_buffer, uint16_t length);
		bool receive (uint8_t * rx_buffer, uint16_t length);
		bool transceive (uint8_t * tx_buffer, uint8_t * rx_buffer, uint16_t length);

		// 16-bit implementations
		bool transmit (uint16_t * tx_buffer, uint16_t length);
		bool receive (uint16_t * rx_buffer, uint16_t length);
		bool transceive (uint16_t * tx_buffer, uint16_t * rx_buffer, uint16_t length);
	};

	// LegacySPI implements only minimal SPI functionality
	class LegacySPI : public SPI {

	private:
		static volatile uint32_t * getDataRegister (void);

	protected:
		LegacySPI (void);
		void initialize (uint32_t peripheral_frequency, uint32_t spi_clock_frequency, uint32_t mode);
		uint32_t mode (uint32_t bits, bool clock_phase_2nd_edge, bool clock_polarity_active_low, bool lsb_first);
		void handle (void);
	};

	// SSP extends standard SPI functionality
	class SSP : public SPI {

	private:
		LPC_SSP_TypeDef * _lpc_ssp;

	private:
		static volatile uint32_t * getDataRegister (uint32_t instance);

	protected:
		SSP (uint32_t instance);
		void initialize (uint32_t peripheral_frequency, uint32_t spi_clock_frequency, uint32_t mode);
		uint32_t mode (uint32_t bits, bool clock_phase_2nd_edge, bool clock_polarity_active_low);
		void handle (void);

	public:
		//TODO:void transceive (uint8_t * tx_buffer, uint16_t tx_length, uint8_t * rx_buffer, uint16_t rx_buffer_size, DMA & tx_dma, DMA & rx_dma);
		//TODO:void transceive (uint16_t * tx_buffer, uint16_t tx_length, uint16_t * rx_buffer, uint16_t rx_buffer_size, DMA & tx_dma, DMA & rx_dma);
	};

	/************************************
	* SPI0 Singleton					*
	************************************/
	class SPI0 : public LegacySPI
	{
	private:
		SPI0 (void);
		SPI0 (SPI0 const&) = delete;
		void operator= (SPI0 const&) = delete;
		static void handleInterrupt (void);

	public:
		static SPI0 & instance (void);
		void initialize (System::Clock::PeripheralClockSpeed clock = System::Clock::PeripheralClockSpeed::cpu_divide_by_4,
						uint32_t spi_clock_frequency = 1000000, uint32_t bits = 8, bool clock_phase_2nd_edge = false, bool clock_polarity_active_low = false, bool lsb_first = false);
	};

	/************************************
	* SSP0 Singleton					*
	************************************/
	class SSP0 : public SSP
	{
	private:
		SSP0 (void);
		SSP0 (SSP0 const&) = delete;
		void operator= (SSP0 const&) = delete;
		static void handleInterrupt (void);

	public:
		static SSP0 & instance (void);
		void initialize (System::Clock::PeripheralClockSpeed clock = System::Clock::PeripheralClockSpeed::cpu_divide_by_4,
						uint32_t spi_clock_frequency = 1000000, uint32_t bits = 8, bool clock_phase_2nd_edge = false, bool clock_polarity_active_low = false);
	};

	/************************************
	* SSP1 Singleton					*
	************************************/
	class SSP1 : public SSP
	{
	private:
		SSP1 (void);
		SSP1 (SSP1 const&) = delete;
		void operator= (SSP1 const&) = delete;
		static void handleInterrupt (void);

	public:
		static SSP1 & instance (void);
		void initialize (System::Clock::PeripheralClockSpeed clock = System::Clock::PeripheralClockSpeed::cpu_divide_by_4,
						uint32_t spi_clock_frequency = 1000000, uint32_t bits = 8, bool clock_phase_2nd_edge = false, bool clock_polarity_active_low = false);
	};
}
