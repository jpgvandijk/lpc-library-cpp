#pragma once

extern void (*_handleInterruptPointerDMA[]) (void);

namespace System {

	class DMA {

	public:
		typedef enum {
			ch_0 = 0,
			ch_1 = 1,
			ch_2 = 2,
			ch_3 = 3,
			ch_4 = 4,
			ch_5 = 5,
			ch_6 = 6,
			ch_7 = 7,
		} Channel;

		typedef enum {
			memory_to_memory = 0,
			memory_to_peripheral = 1,
			peripheral_to_memory = 2,
			peripheral_to_peripheral = 3
		} TransferType;

		typedef enum {
			unused = 0,
			ssp0_tx = 0,
			ssp0_rx = 1,
			ssp1_tx = 2,
			ssp1_rx = 3,
			adc = 4,
			i2s_ch0 = 5,
			i2s_ch1 = 6,
			dac = 7,
			uart0_tx = 8,
			uart0_rx = 9,
			uart1_tx = 10,
			uart1_rx = 11,
			uart2_tx = 12,
			uart2_rx = 13,
			uart3_tx = 14,
			uart3_rx = 15,
			mat0_0 = 24,
			mat0_1 = 25,
			mat1_0 = 26,
			mat1_1 = 27,
			mat2_0 = 28,
			mat2_1 = 29,
			mat3_0 = 30,
			mat3_1 = 31,
		} Peripheral;

		typedef enum {
			transfer_1 = 0,
			transfer_4 = 1,
			transfer_8 = 2,
			transfer_16 = 3,
			transfer_32 = 4,
			transfer_64 = 5,
			transfer_128 = 6,
			transfer_256 = 7
		} BurstSize;

		typedef enum {
			byte = 0,
			uint8 = 0,
			int8 = 0,
			halfword = 1,
			uint16 = 1,
			int16 = 1,
			word = 2,
			uint32 = 2,
			int32 = 2
		} TransferWidth;

	private:
		uint8_t _channel;
		volatile uint32_t _control;
		volatile uint32_t _config;

	protected:
		DMA (DMA::Channel channel);
		void handle (void);

	public:
		static void enable (void);
		static void disable (void);
		void configure (DMA::TransferType transfer_type,
					DMA::Peripheral source_peripheral,
					DMA::Peripheral destination_peripheral,
					DMA::BurstSize source_burst_size,
					DMA::BurstSize destination_burst_size,
					DMA::TransferWidth source_transfer_width,
					DMA::TransferWidth destination_transfer_width,
					bool source_increment,
					bool destination_increment);
		void transfer (volatile void * source, volatile void * destination, uint32_t number_of_transfers, bool auto_re_enable = false);
		uint32_t getTotalNumberOfTransfers (void);
		uint32_t getNumberOfTransfersLeft (void);
		uint32_t numberTransferred (void);
	};

	/************************************
	* DMAChannel<> Singleton			*
	************************************/

	template<DMA::Channel channel>
	class DMAChannel : public DMA {
	private:
		DMAChannel (void) : DMA(channel) {
			_handleInterruptPointerDMA[channel] = handleInterrupt;
		}
		DMAChannel (DMAChannel const&) = delete;
		void operator= (DMA const&) = delete;
		static void handleInterrupt (void) {
			instance().handle();
		}
	public:
		static DMAChannel & instance (void) {
			static DMAChannel<channel> dma_channel;
			return dma_channel;
		}
	};
}
