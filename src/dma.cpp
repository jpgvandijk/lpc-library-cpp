// Includes
#include "LPC17xx.h"
#include "dma.h"
#include "clock.h"
#include "interrupt.h"

// Namespaces
using namespace System;

// Properly define the registers
typedef struct
{
  __IO uint32_t DMACCSrcAddr;
  __IO uint32_t DMACCDestAddr;
  __IO uint32_t DMACCLLI;
  __IO uint32_t DMACCControl;
  __IO uint32_t DMACCConfig;
       uint32_t RESERVED0[3];  // NOTE: added for proper array indexing
} LPC_GPDMACHN_TypeDef;

#define LPC_GPDMACH ((LPC_GPDMACHN_TypeDef *) LPC_GPDMACH0_BASE)

/************************************
* DMA Interrupt Handlers			*
************************************/

void (*_handleInterruptPointerDMA[8]) (void) = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

extern "C" {

	void DMA_IRQHandler (void) {
		for (uint32_t channel = 0; channel < 8; channel++) {
			if (LPC_GPDMA->DMACIntStat & (1 << channel)) {
				if (_handleInterruptPointerDMA[channel] != nullptr) {
					_handleInterruptPointerDMA[channel]();
				}
			}
		}
	}

}

namespace System {

	/************************************
	* DMA Base Implementation			*
	************************************/

	DMA::DMA (DMA::Channel channel) {
		_channel = (uint8_t) channel;
		_control = 0;
		_config = 0;
	}

	void DMA::handle (void) {

		if (LPC_GPDMA->DMACIntTCStat & (1 << _channel)) {

			// Reset source/destination addresses if auto-increment was enabled (correct for data size)
			uint32_t number_of_transfers_to_reset = getTotalNumberOfTransfers() - 1;
			if (_control & (1 << 26)) {
				LPC_GPDMACH[_channel].DMACCSrcAddr -= number_of_transfers_to_reset << ((_control >> 18) & 0x03);
			}
			if (_control & (1 << 27)) {
				LPC_GPDMACH[_channel].DMACCDestAddr -= number_of_transfers_to_reset << ((_control >> 21) & 0x03);
			}

			// Re-enable the channel (_control contains the size, _config the enable bit)
			LPC_GPDMACH[_channel].DMACCControl = _control;
			LPC_GPDMACH[_channel].DMACCConfig = _config;

			// Clear the terminal count interrupt
			LPC_GPDMA->DMACIntTCClear = (1 << _channel);
		}

		if (LPC_GPDMA->DMACIntErrStat & (1 << _channel)) {

			// NOTE: this should never happen, as the interrupt is masked
			// Clear the error status interrupt
			LPC_GPDMA->DMACIntErrClr = (1 << _channel);
		}
	}

	void DMA::enable (void) {

		// Enable peripheral power
		Clock::enablePeripheral(Clock::PeripheralPower::dma_power);

		// Enable the DMA controller
		LPC_GPDMA->DMACConfig = 1;

		// Enable DMA interrupts after clearing any pending interrupts
		LPC_GPDMA->DMACIntTCClear = 0xFF;
		LPC_GPDMA->DMACIntErrClr = 0xFF;
		Interrupt::enable(DMA_IRQn);
	}

	void DMA::disable (void) {

		// FIXME: Disable all DMA channels
		//for (uint32_t channel = 0; channel < 8; channel++) {
		//	LPC_GPDMACH[_channel].DMACCConfig = 0;
		//}

		// Disable DMA interrupts
		Interrupt::disable(DMA_IRQn);

		// Disable the DMA controller
		LPC_GPDMA->DMACConfig = 0;

		// Disable the peripheral power
		Clock::disablePeripheral(Clock::PeripheralPower::dma_power);
	}

	void DMA::configure (DMA::TransferType transfer_type,
			DMA::Peripheral source_peripheral,
			DMA::Peripheral destination_peripheral,
			DMA::BurstSize source_burst_size,
			DMA::BurstSize destination_burst_size,
			DMA::TransferWidth source_transfer_width,
			DMA::TransferWidth destination_transfer_width,
			bool source_increment,
			bool destination_increment) {

		// Prepare the control word
		this->_control = ((((uint32_t) source_burst_size) << 12) |
				(((uint32_t) destination_burst_size) << 15) |
				(((uint32_t) source_transfer_width) << 18) |
				(((uint32_t) destination_transfer_width) << 21) |
				(((uint32_t) source_increment) << 26) |
				(((uint32_t) destination_increment) << 27));

		// Prepare the configuration word
		if (source_peripheral & (1 << 4)) {
			LPC_SC->DMAREQSEL |= (1 << (((uint32_t) source_peripheral) & 0x07));
		} else {
			LPC_SC->DMAREQSEL &= ~(1 << (((uint32_t) source_peripheral) & 0x07));
		}
		if (destination_peripheral & (1 << 4)) {
			LPC_SC->DMAREQSEL |= (1 << (((uint32_t) destination_peripheral) & 0x07));
		} else {
			LPC_SC->DMAREQSEL &= ~(1 << (((uint32_t) destination_peripheral) & 0x07));
		}
		this->_config = ((((uint32_t) source_peripheral) & 0x0F) << 1) |
				((((uint32_t) destination_peripheral) & 0x0F) << 6) |
				(((uint32_t) transfer_type) << 11) | (1 << 0) | (1 << 15);
	}

	void DMA::transfer (volatile void * source, volatile void * destination, uint32_t number_of_transfers, bool auto_re_enable) {

		// Make sure the channel is disabled
		LPC_GPDMACH[_channel].DMACCConfig = 0;

		// Clear any pending interrupts
		LPC_GPDMA->DMACIntTCClear = (1 << _channel);
		LPC_GPDMA->DMACIntErrClr = (1 << _channel);

		// Setup the source and destination addresses
		LPC_GPDMACH[_channel].DMACCSrcAddr = (uint32_t) source;
		LPC_GPDMACH[_channel].DMACCDestAddr = (uint32_t) destination;
		LPC_GPDMACH[_channel].DMACCLLI = 0;

		// Use the previously prepared control and configuration words
		_control = (_control & ~(0x0FFF)) | number_of_transfers;
		if (auto_re_enable) {
			_control |= (1 << 31);
		} else {
			_control &= ~(1 << 31);
		}
		LPC_GPDMACH[_channel].DMACCControl = _control;
		LPC_GPDMACH[_channel].DMACCConfig = _config;
	}

	uint32_t DMA::getTotalNumberOfTransfers (void) {
		return _control & 0xFFF;
	}

	uint32_t DMA::getNumberOfTransfersLeft (void) {
		return LPC_GPDMACH[_channel].DMACCControl & 0xFFF;
	}

	uint32_t DMA::numberTransferred (void) {
		return getTotalNumberOfTransfers() - getNumberOfTransfersLeft();
	}
}
