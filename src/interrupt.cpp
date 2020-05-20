// Includes
#include "LPC17xx.h"
#include "core_cm3.h"
#include "interrupt.h"

// Namespaces
using namespace System;

namespace System::Interrupt {

	namespace {
		uint8_t _priority_grouping = (uint8_t) Interrupt::PriorityGrouping::priorities_32_group_1_sub;
	}

	void enable (void) {
		__enable_irq();
	}

	void disable (void) {
		__disable_irq();
	}

	void enable (IRQn_Type interrupt) {
		if (interrupt >= 0) {
			NVIC_EnableIRQ(interrupt);
		} else if (interrupt == UsageFault_IRQn) {
			SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk;
		} else if (interrupt == BusFault_IRQn) {
			SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk;
		} else if (interrupt == MemoryManagement_IRQn) {
			SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
		} else if (interrupt == SysTick_IRQn) {
			SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
		}
	}

	void disable (IRQn_Type interrupt) {
		if (interrupt >= 0) {
			NVIC_DisableIRQ(interrupt);
		} else if (interrupt == UsageFault_IRQn) {
			SCB->SHCSR &= ~SCB_SHCSR_USGFAULTENA_Msk;
		} else if (interrupt == BusFault_IRQn) {
			SCB->SHCSR &= ~SCB_SHCSR_BUSFAULTENA_Msk;
		} else if (interrupt == MemoryManagement_IRQn) {
			SCB->SHCSR &= ~SCB_SHCSR_MEMFAULTENA_Msk;
		} else if (interrupt == SysTick_IRQn) {
			SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
		}
	}

	bool isPending (void) {
		return ((SCB->ICSR & SCB_ICSR_ISRPENDING_Msk) != 0);
	}

	bool isPending (IRQn_Type interrupt) {
		if (interrupt >= 0) {
			return (NVIC_GetPendingIRQ(interrupt) != 0);
		} else if (interrupt == NonMaskableInt_IRQn) {
			return ((SCB->ICSR & SCB_ICSR_NMIPENDSET_Msk) != 0);
		} else if (interrupt == PendSV_IRQn) {
			return ((SCB->ICSR & SCB_ICSR_PENDSVSET_Msk) != 0);
		} else if (interrupt == SysTick_IRQn) {
			return ((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) != 0);
		} else if (interrupt == SVCall_IRQn) {
			return ((SCB->SHCSR & SCB_SHCSR_SVCALLPENDED_Msk) != 0);
		} else if (interrupt == BusFault_IRQn) {
			return ((SCB->SHCSR & SCB_SHCSR_BUSFAULTPENDED_Msk) != 0);
		} else if (interrupt == MemoryManagement_IRQn) {
			return ((SCB->SHCSR & SCB_SHCSR_MEMFAULTPENDED_Msk) != 0);
		} else if (interrupt == UsageFault_IRQn) {
			return ((SCB->SHCSR & SCB_SHCSR_USGFAULTPENDED_Msk) != 0);
		}
		return false;
	}

	IRQn_Type getPending (void) {
		uint32_t exception = (SCB->ICSR & SCB_ICSR_VECTPENDING_Msk) >> SCB_ICSR_VECTPENDING_Pos;
		int32_t irq = exception - 16;
		return ((IRQn_Type) irq);
	}

	void setPending (IRQn_Type interrupt) {
		if (interrupt >= 0) {
			NVIC_SetPendingIRQ(interrupt);
		} else if (interrupt == NonMaskableInt_IRQn) {
			SCB->ICSR |= SCB_ICSR_NMIPENDSET_Msk;
		} else if (interrupt == PendSV_IRQn) {
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		} else if (interrupt == SysTick_IRQn) {
			SCB->ICSR |= SCB_ICSR_PENDSTSET_Msk;
		} else if (interrupt == SVCall_IRQn) {
			SCB->SHCSR |= SCB_SHCSR_SVCALLPENDED_Msk;
		} else if (interrupt == BusFault_IRQn) {
			SCB->SHCSR |= SCB_SHCSR_BUSFAULTPENDED_Msk;
		} else if (interrupt == MemoryManagement_IRQn) {
			SCB->SHCSR |= SCB_SHCSR_MEMFAULTPENDED_Msk;
		} else if (interrupt == UsageFault_IRQn) {
			SCB->SHCSR |= SCB_SHCSR_USGFAULTPENDED_Msk;
		}
	}

	void clearPending (IRQn_Type interrupt) {
		if (interrupt >= 0) {
			NVIC_ClearPendingIRQ(interrupt);
		} else if (interrupt == PendSV_IRQn) {
			SCB->ICSR |= SCB_ICSR_PENDSVCLR_Msk;
		} else if (interrupt == SysTick_IRQn) {
			SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;
		} else if (interrupt == SVCall_IRQn) {
			SCB->SHCSR &= ~SCB_SHCSR_SVCALLPENDED_Msk;
		} else if (interrupt == BusFault_IRQn) {
			SCB->SHCSR &= ~SCB_SHCSR_BUSFAULTPENDED_Msk;
		} else if (interrupt == MemoryManagement_IRQn) {
			SCB->SHCSR &= ~SCB_SHCSR_MEMFAULTPENDED_Msk;
		} else if (interrupt == UsageFault_IRQn) {
			SCB->SHCSR &= ~SCB_SHCSR_USGFAULTPENDED_Msk;
		}
	}

	bool isActive (void) {
		return ((SCB->ICSR & SCB_ICSR_RETTOBASE_Msk) == 0);
	}

	bool isActive (IRQn_Type interrupt) {
		if (interrupt >= 0) {
			return (NVIC_GetActive(interrupt) != 0);
		} else if (interrupt == SysTick_IRQn) {
			return ((SCB->SHCSR & SCB_SHCSR_SYSTICKACT_Msk) != 0);
		} else if (interrupt == PendSV_IRQn) {
			return ((SCB->SHCSR & SCB_SHCSR_PENDSVACT_Msk) != 0);
		} else if (interrupt == DebugMonitor_IRQn) {
			return ((SCB->SHCSR & SCB_SHCSR_MONITORACT_Msk) != 0);
		} else if (interrupt == SVCall_IRQn) {
			return ((SCB->SHCSR & SCB_SHCSR_SVCALLACT_Msk) != 0);
		} else if (interrupt == UsageFault_IRQn) {
			return ((SCB->SHCSR & SCB_SHCSR_USGFAULTACT_Msk) != 0);
		} else if (interrupt == BusFault_IRQn) {
			return ((SCB->SHCSR & SCB_SHCSR_BUSFAULTACT_Msk) != 0);
		} else if (interrupt == MemoryManagement_IRQn) {
			return ((SCB->SHCSR & SCB_SHCSR_MEMFAULTACT_Msk) != 0);
		}
		return false;
	}

	IRQn_Type getActive (void) {
		uint32_t exception = (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) >> SCB_ICSR_VECTACTIVE_Pos;
		int32_t irq = exception - 16;
		return ((IRQn_Type) irq);
	}

	void setPriority (IRQn_Type interrupt, uint32_t preempt_priority, uint32_t sub_priority) {
		NVIC_SetPriority(interrupt, NVIC_EncodePriority((uint32_t) _priority_grouping, preempt_priority, sub_priority));
	}

	void setPriorityGrouping (PriorityGrouping priority_grouping) {
		_priority_grouping = (uint8_t) priority_grouping;
		NVIC_SetPriorityGrouping((uint32_t) _priority_grouping);
	}

	void trigger (IRQn_Type interrupt) {
		if ((interrupt >= 0) && (interrupt < 112)) {
			NVIC->STIR = interrupt;
		}
	}
}
