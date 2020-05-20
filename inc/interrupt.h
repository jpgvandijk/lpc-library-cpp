#pragma once

namespace System::Interrupt {

	// Type definitions
	typedef enum {
		priorities_32_group_1_sub = 2,
		priorities_16_group_2_sub = 3,
		priorities_8_group_4_sub = 4,
		priorities_4_group_8_sub = 5,
		priorities_2_group_16_sub = 6,
		priorities_1_group_32_sub = 7
	} PriorityGrouping;

	// Function prototypes
	void enable (void);
	void disable (void);
	void enable (IRQn_Type interrupt);
	void disable (IRQn_Type interrupt);

	bool isPending (void);
	bool isPending (IRQn_Type interrupt);
	IRQn_Type getPending (void);
	void setPending (IRQn_Type interrupt);
	void clearPending (IRQn_Type interrupt);

	bool isActive (IRQn_Type interrupt);
	IRQn_Type getActive (void);

	void setPriority (IRQn_Type interrupt, uint32_t preempt_priority, uint32_t sub_priority);
	void setPriorityGrouping (PriorityGrouping priority_grouping);

	void trigger (IRQn_Type interrupt);
}
