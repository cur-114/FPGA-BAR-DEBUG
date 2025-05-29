#pragma once
#include "structs.hpp"
#include <stdint.h>
#include <Windows.h>
#include <map>

#define CC_SUCCESS          1
#define CC_TRB_ERROR        5
#define CC_CONTEXT_STATE_ERROR 19

#define SLOT_CONTEXT_ENTRIES_MASK 0x1f
#define SLOT_CONTEXT_ENTRIES_SHIFT 27

#define SLOT_STATE_MASK     0x1f
#define SLOT_STATE_SHIFT        27
#define SLOT_STATE(s)       (((s)>>SLOT_STATE_SHIFT)&SLOT_STATE_MASK)
#define SLOT_ENABLED        0
#define SLOT_DEFAULT        1
#define SLOT_ADDRESSED      2
#define SLOT_CONFIGURED     3

#define EP_TYPE_MASK        0x7
#define EP_TYPE_SHIFT           3

#define EP_STATE_MASK       0x7
#define EP_DISABLED         (0<<0)
#define EP_RUNNING          (1<<0)
#define EP_HALTED           (2<<0)
#define EP_STOPPED          (3<<0)
#define EP_ERROR            (4<<0)

enum InternalState {
	START,
	ENABLE_SLOT,
	ENABLE_SLOT_RESETED
};

namespace xhci {

	inline HostControllerCapabilityRegisters* capReg = nullptr;
	inline HostControllerOperationalRegisters* opReg = nullptr;
	inline HostControllerRuntimeRegisters* rtReg = nullptr;

	inline int command_ring_done_index = 0;

	inline std::map<int, int> event_ring_cycle_state;
	inline std::map<int, uint64_t> event_ring_enqueue_pointer;

	//inline std::map<int, int> ep0_cycle_state;
	//inline std::map<int, uint64_t> ep0_dequeue_pointer;

	inline std::map<int, InternalSlotContext> internal_slot_contexts;
	inline InternalState internal_state = InternalState::START;

	void init_pointers();
	void interrupter_enable(int index);

	void xhci_run();
	void xhci_stop();
	void xhci_reset();
	void portsc_reset(int port_index);
	void send_event_ring(int interrupter_index, PBYTE trb);
	void send_command_completion_event(uint64_t cmd_trb_ptr, uint8_t Slot_ID, uint32_t completion_code);
	void send_port_status_change_event();

	void consume_command_ring(BARLogEntry* entry);
	void handle_transfer_ring(uint32_t offset, uint8_t ep_id, uint16_t stream_id);

	void set_ep_state(uint32_t slot_id, uint32_t ep_id, uint32_t state);

	void init_ep_ctx(uint32_t slot_id, uint32_t ep_id, uint32_t* ep_ctx);
	void enable_ep(uint32_t slot_id, uint32_t ep_id, uint64_t p_ctx, uint32_t *ep_ctx);
	void disable_ep(uint32_t slot_id, uint32_t ep_id);
}