#include <thread>
#include <string>
#include <format>

#include "stdint.h"
#include "xhci.hpp"
#include "memory.hpp"
#include "utils.hpp"


void xhci::init_pointers() {
	capReg = (HostControllerCapabilityRegisters*)(memory::bar0.data());
	xhci::opReg = (HostControllerOperationalRegisters*)(memory::bar0.data() + capReg->CAPLENGTH);
	xhci::rtReg = (HostControllerRuntimeRegisters*)(memory::bar0.data() + capReg->RTSOFF);

	for (int i = 0; i < 8; i++) {
		//Internal Slot Context
		InternalSlotContext isc = {};
		isc.slot_id = i + 1;
		isc.slot_state = SlotState::DISABLE;
		isc.device_context = 0;

		internal_slot_contexts.insert(std::make_pair(i + 1, isc));
	}
}

void xhci::interrupter_enable(int index) {
	printf("interrupter %d enabled\n", index);
	xhci::event_ring_cycle_state[index] = 1;
	xhci::event_ring_enqueue_pointer[index] = 0;
}

void xhci::xhci_run() {
	printf("xhci::run()\n");
	opReg->USBSTS.HCHalted = 0;
	opReg->USBCMD.RunStop = 1;
}

void xhci::xhci_stop() {
	printf("xhci::stop()\n");
	opReg->USBSTS.HCHalted = 1;
	opReg->USBCMD.RunStop = 0;
}

void xhci::xhci_reset() {
	printf("xhci::reset()\n");

	HostControllerCapabilityRegisters* capRegister = (HostControllerCapabilityRegisters*)(memory::bar0.data());
	HostControllerOperationalRegisters* opRegister = (HostControllerOperationalRegisters*)(memory::bar0.data() + capRegister->CAPLENGTH);
	HostControllerRuntimeRegisters* rtRegister = (HostControllerRuntimeRegisters*)(memory::bar0.data() + capRegister->RTSOFF);

	opRegister->USBCMD_RAW = 0;

	opRegister->USBSTS_RAW = 0;
	(&opRegister->USBSTS)->HCHalted = 1;
	/*
	opRegister->USBSTS.HCHalted = 1;
	opRegister->USBSTS.Host_System_Error = 0;
	opRegister->USBSTS.Event_Interrupt = 0;
	opRegister->USBSTS.Port_Change_Detect = 0;
	opRegister->USBSTS.Save_State_Status = 0;
	opRegister->USBSTS.Restore_State_Status = 0;
	opRegister->USBSTS.Host_Controller_Error = 0;*/

	opRegister->DNCTRL = 0;

	opRegister->CRCR.Ring_Cycle_State = 0;
	opRegister->CRCR.Command_Stop = 0;
	opRegister->CRCR.Command_Abort = 0;
	opRegister->CRCR.Command_Ring_Running = 0;
	opRegister->CRCR.Command_Ring_Pointer = 0;

	opRegister->DCBAAP = 0;

	opRegister->CONFIG.Max_Device_Slots_Enabled = 0;
	opRegister->CONFIG.U3_Entry_Enable = 0;
	opRegister->CONFIG.Configuration_Information_Enable = 0;

	for (int i = 0; i < 8; i++) {
		opRegister->PortRegisters[i].PORTSC.Current_Connect_Status = 0;
		opRegister->PortRegisters[i].PORTSC.Port_Enable_Disable = 0;
		opRegister->PortRegisters[i].PORTSC.Over_Current_Active = 0;
		opRegister->PortRegisters[i].PORTSC.Port_Reset = 0;

		opRegister->PortRegisters[i].PORTSC.Port_Link_State = 5;
		opRegister->PortRegisters[i].PORTSC.Port_Speed = 0;
		//if (i < 4) { //USB2
		//	opRegister->PortRegisters[i].PORTSC.Port_Link_State = 0;
		//	opRegister->PortRegisters[i].PORTSC.Port_Speed = 1;
		//}
		//else { //USB3
		//	opRegister->PortRegisters[i].PORTSC.Port_Link_State = 5;
		//	opRegister->PortRegisters[i].PORTSC.Port_Speed = 2;
		//}

		opRegister->PortRegisters[i].PORTSC.Port_Power = 1;
		//opRegister->PortRegisters[i].PORTSC.Port_Speed = 0;
		opRegister->PortRegisters[i].PORTSC.Port_Indicator_Control = 0;
		opRegister->PortRegisters[i].PORTSC.Port_Link_State_Write_Strobe = 0;
		opRegister->PortRegisters[i].PORTSC.Connect_Status_Change = 0;
		opRegister->PortRegisters[i].PORTSC.Port_Enabled_Disabled_Change = 0;
		opRegister->PortRegisters[i].PORTSC.Warm_Port_Reset_Change = 0;
		opRegister->PortRegisters[i].PORTSC.Over_Current_Change = 0;
		opRegister->PortRegisters[i].PORTSC.Port_Reset_Change = 0;
		opRegister->PortRegisters[i].PORTSC.Port_Link_State_Change = 0;
		opRegister->PortRegisters[i].PORTSC.Port_Config_Error_Change = 0;
		opRegister->PortRegisters[i].PORTSC.Cold_Attach_Status = 0;
		opRegister->PortRegisters[i].PORTSC.Wake_on_Connect_Enable = 0;
		opRegister->PortRegisters[i].PORTSC.Wake_on_Disconnect_Enable = 0;
		opRegister->PortRegisters[i].PORTSC.Wake_on_Over_current_Enable = 0;
		opRegister->PortRegisters[i].PORTSC.Device_Removable = 0;
		opRegister->PortRegisters[i].PORTSC.Warm_Port_Reset = 0;

		*((uint32_t*)&opRegister->PortRegisters[i].PORTPMSC_USB3) = 0;

		//if (i == 5) {
		//	opRegister->PortRegisters[i].PORTSC.Port_Enable_Disable = 1;
		//	opRegister->PortRegisters[i].PORTSC.Port_Link_State = 7;
		//	opRegister->PortRegisters[i].PORTSC.Port_Power = 1;
		//	opRegister->PortRegisters[i].PORTSC.Port_Speed = 1;
		//	opRegister->PortRegisters[i].PORTSC.Connect_Status_Change = 1;
		//}

		opRegister->PortRegisters[i].PORTLI_USB3.Link_Error_Count = 0;
		opRegister->PortRegisters[i].PORTLI_USB3.Rx_Lane_Count = 0;
		opRegister->PortRegisters[i].PORTLI_USB3.Tx_Lane_Count = 0;

		opRegister->PortRegisters[i].PORTHLPMC = 0;
	}

	rtRegister->MFINDEX = 0;

	for (int i = 0; i < 0x8; i++) {
		rtRegister->InterrupterRegisters[i].IMAN.Interrupt_Pending = 0;
		rtRegister->InterrupterRegisters[i].IMAN.Interrupt_Enable = 0;

		rtRegister->InterrupterRegisters[i].IMOD.Interrupt_Moderation_Interval = 0;
		rtRegister->InterrupterRegisters[i].IMOD.Interrupt_Moderation_Count = 0;

		rtRegister->InterrupterRegisters[i].ERSTSZ.Event_Ring_Segment_Table_Size = 0;

		rtRegister->InterrupterRegisters[i].ERDP.Dequeue_ERST_Segment_Index = 0;
		rtRegister->InterrupterRegisters[i].ERDP.Event_Handler_Busy = 0;
		rtRegister->InterrupterRegisters[i].ERDP.Event_Ring_Dequeue_Pointer = 0;
	}

	opRegister->USBCMD.Host_Controller_Reset = 0;
}

void xhci::portsc_reset(int port_index) {
	printf("%d PORTSC RESET\n", port_index);
	PortRegisterSet_PORTSC* pPORTSC = &xhci::opReg->PortRegisters[port_index].PORTSC;
	uint32_t raw = *(uint32_t*)pPORTSC;
	printf("PORTSC: 0x%X\n", raw);
	pPORTSC->Port_Reset = 0;
	pPORTSC->Port_Reset_Change = 1;

	if (xhci::internal_state == InternalState::ENABLE_SLOT) {
		printf("XDXDXDDXXD\n");
		xhci::internal_state = InternalState::ENABLE_SLOT_RESETED;
		pPORTSC->Port_Enable_Disable = 1;
		//pPORTSC->Port_Enabled_Disabled_Change = 1;
		pPORTSC->Port_Link_State = 0;
	}

	send_port_status_change_event();
}

void xhci::send_event_ring(int interrupter_index, PBYTE trb) {
	TRB_Unknown* uTRB = (TRB_Unknown*)trb;

	int ERSTE_count = xhci::rtReg->InterrupterRegisters[interrupter_index].ERSTSZ.Event_Ring_Segment_Table_Size;
	std::vector<Event_Ring_Segment_Table_Entry> ERSTEs(ERSTE_count);
	if (!LcRead(memory::hLC, rtReg->InterrupterRegisters[0].ERSTBA.Event_Ring_Segment_Table_Base_Address_Register << 6, sizeof(Event_Ring_Segment_Table_Entry) * ERSTE_count, (PBYTE)ERSTEs.data())) {
		printf("[! Event Ring] Failed to read EventRingSegmentTableEntries!\n");
		return;
	}

	uint64_t enqueue_pointer = xhci::event_ring_enqueue_pointer[interrupter_index];
	if (enqueue_pointer == 0) {
		//enqueue_pointer = ERSTEs[0].Ring_Segment_Base_Address << 6;
		enqueue_pointer = rtReg->InterrupterRegisters[interrupter_index].ERDP.Event_Ring_Dequeue_Pointer << 4;
	}

	uTRB->Cycle_Bit = xhci::event_ring_cycle_state[interrupter_index];
	printf("[+ Event Ring] Write TRB(Type: %d, TypeName: %s) to 0x%llX\n", uTRB->TRB_Type, utils::get_trb_type_name(uTRB->TRB_Type).c_str(), enqueue_pointer);
	if (!LcWrite(memory::hLC, enqueue_pointer, 0x10, trb)) {
		printf("[! Event Ring] Failed to write TRB to Event Ring!\n");
		return;
	}

	uint64_t msix_addr = *(uint64_t*)(memory::bar2.data() + 0x10 * interrupter_index);
	uint32_t msix_data = *(uint32_t*)(memory::bar2.data() + 0x10 * interrupter_index + 0x8);

	//printf("[+ Event Ring] MSI-X addr: 0x%llX, data: 0x%llX\n", msix_addr, msix_data);
	xhci::opReg->USBSTS.Event_Interrupt = 1;

	if (!LcWrite(memory::hLC, msix_addr, 4, (PBYTE)&msix_data)) {
		printf("[! Event Ring] Failed to Write MSI-X!\n");
		return;
	}
	//printf("[+ Event Ring] SENT MSI-X Intrrupt!\n");

	int current_erste_index = -1;
	for (int i = 0; i < ERSTEs.size(); i++) {
		uint64_t ring_seg_base = ERSTEs[i].Ring_Segment_Base_Address << 6;
		uint64_t ring_seg_end = ring_seg_base + (ERSTEs[i].Ring_Segment_Size - 1) * 0x10;
		if (ring_seg_base <= enqueue_pointer && enqueue_pointer <= ring_seg_end) {
			current_erste_index = i;
			break;
		}
	}
	if (current_erste_index == -1) {
		printf("[! Event Ring] Unknown ERSTE index!\n");
		return;
	}

	uint64_t new_enqueue_pointer = enqueue_pointer + 0x10;
	if (enqueue_pointer == (ERSTEs[current_erste_index].Ring_Segment_Base_Address << 6) + (ERSTEs[current_erste_index].Ring_Segment_Size - 1) * 0x10) {
		//next event ring segment
		int next_index = current_erste_index + 1;
		if (next_index == ERSTEs.size()) {
			//Ring Cycle Bit update
			next_index = 0;
			xhci::event_ring_cycle_state[interrupter_index] = xhci::event_ring_cycle_state[interrupter_index] == 0 ? 1 : 0;
		}
		new_enqueue_pointer = ERSTEs[next_index].Ring_Segment_Base_Address << 6;
	}

	xhci::event_ring_enqueue_pointer[interrupter_index] = new_enqueue_pointer;
	//printf("[+ Event Ring] new enqueue pointer = 0x%llX\n", new_enqueue_pointer);
}

void xhci::send_command_completion_event(uint64_t cmd_trb_ptr, uint8_t Slot_ID, uint32_t completion_code = 1) {
	printf("[+] Command Completion TRB => pointer: 0x%llX\n", cmd_trb_ptr << 4);
	TRB_Command_Completion_Event TRB = {};
	TRB.TRB_Type = 33;
	TRB.Command_TRB_Pointer = cmd_trb_ptr;
	TRB.Slot_ID = Slot_ID;
	TRB.Completion_Code = completion_code;
	xhci::send_event_ring(0, (PBYTE)&TRB);
}

void xhci::send_port_status_change_event() {
	HostControllerCapabilityRegisters* capRegister = (HostControllerCapabilityRegisters*)(memory::bar0.data());
	HostControllerOperationalRegisters* opRegister = (HostControllerOperationalRegisters*)(memory::bar0.data() + capRegister->CAPLENGTH);

	TRB_Port_Status_Change_Event TRB = {};
	TRB.Cycle_Bit = 1;
	TRB.Port_ID = 1;
	TRB.TRB_Type = 0x22;
	TRB.Completion_Code = 1;
	opRegister->USBSTS.Event_Interrupt = 1;
	xhci::send_event_ring(0, (PBYTE)&TRB);
}

void xhci::set_ep_state(uint32_t slot_id, uint32_t ep_id, uint32_t state) {
	InternalEndpointContext* i_ep_ctx = &xhci::internal_slot_contexts[slot_id].internal_endpoint_contexts[ep_id - 1];

	uint32_t ctx[5];

	memory::read_raw(i_ep_ctx->p_ctx, sizeof(ctx), (PBYTE)&ctx);

	ctx[0] &= ~EP_STATE_MASK;
	ctx[0] |= state;

	uint64_t ring;

	if (i_ep_ctx->nr_pstreams) {
		//NOT IMPLEMENTED
		printf("SET EP STATE NOT IMPLEMENTED FOR STREAMS!\n");
	}
	else {
		ring = i_ep_ctx->tr_dequeue_pointer;
	}

	if (ring) {
		ctx[2] = ring | (i_ep_ctx->cycle_state & 1);
		ctx[3] = ring >> 32;
		printf("EP%d CTX state=%d dequeue = %08X%08X\n", ep_id, state, ctx[3], ctx[2]);
	}

	memory::write_raw(i_ep_ctx->p_ctx, sizeof(ctx), (PBYTE)&ctx);
	i_ep_ctx->state = state;
}

void xhci::init_ep_ctx(uint32_t slot_id, uint32_t ep_id, uint32_t* ctx) {
	InternalEndpointContext* i_ep_ctx = &xhci::internal_slot_contexts[slot_id].internal_endpoint_contexts[ep_id - 1];

	uint64_t dequeue = (static_cast<uint64_t>(ctx[3]) << 32) | (ctx[2] & ~0xf);

	i_ep_ctx->type = static_cast<EPType>((ctx[1] >> EP_TYPE_SHIFT) & EP_TYPE_MASK);
	i_ep_ctx->max_psize = ctx[1] >> 16;
	i_ep_ctx->max_psize *= 1 + ((ctx[1] >> 8) & 0xff);
	i_ep_ctx->max_pstreams = (ctx[0] >> 10) & 0x7;
	i_ep_ctx->lsa = (ctx[0] >> 15) & 1;

	printf("ENDPOINT MAX STREAMS: %d\n", i_ep_ctx->max_pstreams);
	if (i_ep_ctx->max_pstreams) {
		printf("NOT IMPLEMENTED STREAM INITIALIZATION!\n");
		return;
	}
	else {
		i_ep_ctx->tr_dequeue_pointer = dequeue;
		i_ep_ctx->cycle_state = ctx[2] & 1;
	}
}

void xhci::enable_ep(uint32_t slot_id, uint32_t ep_id, uint64_t p_ctx, uint32_t* ep_ctx) {
	InternalEndpointContext* i_ep_ctx = &xhci::internal_slot_contexts[slot_id].internal_endpoint_contexts[ep_id - 1];
	i_ep_ctx->p_ctx = p_ctx;
	i_ep_ctx->state = EP_RUNNING;

	xhci::init_ep_ctx(slot_id, ep_id, ep_ctx);

	ep_ctx[0] &= ~EP_STATE_MASK;
	ep_ctx[0] |= EP_RUNNING;
}

void xhci::disable_ep(uint32_t slot_id, uint32_t ep_id) {
	InternalEndpointContext* i_ep_ctx = &xhci::internal_slot_contexts[slot_id].internal_endpoint_contexts[ep_id - 1];

	if (i_ep_ctx->state == EP_DISABLED) {
		printf("Slot %d EP %d already disabled!\n", slot_id, ep_id);
		return;
	}
}

uint32_t handle_disable_slot_command(TRB_Disable_Slot_Command* dsTRB) {
	xhci::internal_slot_contexts[dsTRB->Slot_ID].slot_state = SlotState::DISABLE;

	return 1;
}

uint32_t handle_address_device_command(TRB_Address_Device_Command* adTRB) {
	char log[200];
	sprintf_s(log, "[CMD] Address Device Command => Slot_ID: %d, Input Context Pointer: 0x%llX", adTRB->Slot_ID, adTRB->Input_Context_Pointer);
	printf("%s\n", log);

	printf("[CMD] Address Device Command\n");
	uint64_t p_input_context = adTRB->Input_Context_Pointer << 4;
	uint64_t p_output_context = memory::readt_raw<uint64_t>(xhci::opReg->DCBAAP + 8 * adTRB->Slot_ID);
	uint64_t p_ep0_ctx = p_output_context + 0x20;

	printf(" Input Context Pointer: 0x%llX\n", adTRB->Input_Context_Pointer << 4);
	printf(" Output Context Pointer: 0x%llX\n", p_output_context);
	printf(" Slot ID: %d\n", adTRB->Slot_ID);
	printf(" Block Set Address Request: %d\n", adTRB->Block_Set_Address_Request);

	//uint32_t input_control_context[2];
	uint32_t slot_context[4];
	uint32_t ep0_context[5];

	//memory::read_raw(p_input_context, sizeof(input_control_context), (PBYTE)&input_control_context);
	memory::read_raw(p_input_context + 0x8 * 4, sizeof(slot_context), (PBYTE)&slot_context);
	memory::read_raw(p_input_context + 0x8 * 8, sizeof(ep0_context), (PBYTE)&ep0_context);

	//printf(" Input Context Control: %08X %08X\n", input_control_context[0], input_control_context[1]);
	printf(" Input Slot Context:    %08X %08X %08X %08X\n", slot_context[0], slot_context[1], slot_context[2], slot_context[3]);
	printf(" Input EP0 Context:     %08X %08X %08X %08X %08X\n", ep0_context[0], ep0_context[1], ep0_context[2], ep0_context[3], ep0_context[4]);

	if (adTRB->Block_Set_Address_Request) {
		slot_context[3] = 1 << 27;// SLOT_DEFAULT << SLOT_STATE_SHIFT
		xhci::internal_slot_contexts[adTRB->Slot_ID].slot_state = SlotState::DEFAULT;
	}
	else {
		printf("NOT IMPLEMENTED SET ADDRESS REQUEST\n");
		xhci::internal_slot_contexts[adTRB->Slot_ID].slot_state = SlotState::ADDRESSED;
	}
	xhci::internal_slot_contexts[adTRB->Slot_ID].device_context = p_output_context;
	xhci::internal_slot_contexts[adTRB->Slot_ID].intr = (slot_context[2] >> 22) & 0x3FF;

	xhci::enable_ep(adTRB->Slot_ID, 1, p_ep0_ctx, ep0_context);

	memory::write_raw(p_output_context, sizeof(slot_context), (PBYTE)&slot_context);
	memory::write_raw(p_ep0_ctx, sizeof(ep0_context), (PBYTE)&ep0_context);

	return 1;
}

uint32_t handle_configure_endpoint_command(TRB_Configure_Endpoint_Command* ceTRB) {
	InternalSlotContext* isc = &xhci::internal_slot_contexts[ceTRB->Slot_ID];
	uint64_t p_input_ctx = ceTRB->Input_Context_Pointer << 4;
	uint64_t p_output_ctx = isc->device_context;

	uint32_t ictl_ctx[2];
	uint32_t islot_ctx[4];
	uint32_t slot_ctx[4];

	memory::read_raw(p_input_ctx + 0x20, sizeof(islot_ctx), (PBYTE)&islot_ctx);
	memory::read_raw(p_output_ctx, sizeof(slot_ctx), (PBYTE)&slot_ctx);

	if (ceTRB->Deconfigure) {
		slot_ctx[3] &= ~(SLOT_STATE_MASK << SLOT_STATE_SHIFT);
		slot_ctx[3] |= SLOT_ADDRESSED << SLOT_STATE_SHIFT;
		isc->slot_state = SlotState::ADDRESSED;
		memory::write_raw(p_output_ctx, sizeof(slot_ctx), (PBYTE)&slot_ctx);
		return CC_SUCCESS;
	}

	memory::read_raw(ceTRB->Input_Context_Pointer << 4, sizeof(ictl_ctx), (PBYTE)&ictl_ctx);

	if ((ictl_ctx[0] & 0x3) != 0x0 || (ictl_ctx[1] & 0x3) != 0x1) {
		printf("xhci: invalid input context control %08x %08x\n", ictl_ctx[0], ictl_ctx[1]);
		return CC_TRB_ERROR;
	}

	if (SLOT_STATE(slot_ctx[3]) < SLOT_ADDRESSED) {
		printf("xhci: invalid slot state 0x%llX: %08x\n", p_output_ctx, slot_ctx[3]);
		//return CC_CONTEXT_STATE_ERROR;
	}
	
	//TODO: FREE DEVICE STREAMS
	printf("CFG EP CMD:  FREE DEVICE STREAMS NOT IMPLEMENTED\n");

	for (int i = 2; i <= 31; i++) {
		if (ictl_ctx[0] & (1 << i)) {
			xhci::disable_ep(ceTRB->Slot_ID, i);
		}
		uint32_t ep_ctx[5];
		if (ictl_ctx[1] & (1 << i)) {
			memory::read_raw(p_input_ctx + 0x20 + (0x20 * i), sizeof(ep_ctx), (PBYTE)&ep_ctx);

			xhci::disable_ep(ceTRB->Slot_ID, i);
			xhci::enable_ep(ceTRB->Slot_ID, i, p_output_ctx + 0x20 * i, ep_ctx);
			printf("xhci: output ep%d.%d context: %08x %08x %08x %08x %08x\n",
				i / 2, i % 2, ep_ctx[0], ep_ctx[1], ep_ctx[2],
				ep_ctx[3], ep_ctx[4]);
			memory::write_raw(p_output_ctx + 0x20 * i, sizeof(ep_ctx), (PBYTE)&ep_ctx);
		}
	}

	//TODO: ALLOC DEVICE STREAMS
	printf("CFG EP CMD:  ALLOC DEVICE STREAMS NOT IMPLEMENTED\n");

	slot_ctx[3] &= ~(SLOT_STATE_MASK << SLOT_STATE_SHIFT);
	slot_ctx[3] |= SLOT_CONFIGURED << SLOT_STATE_SHIFT;
	slot_ctx[0] &= ~(SLOT_CONTEXT_ENTRIES_MASK << SLOT_CONTEXT_ENTRIES_SHIFT);
	slot_ctx[0] |= islot_ctx[0] & (SLOT_CONTEXT_ENTRIES_MASK << SLOT_CONTEXT_ENTRIES_SHIFT);
	memory::write_raw(p_output_ctx, sizeof(slot_ctx), (PBYTE)&slot_ctx);

	return CC_SUCCESS;
}

uint32_t handle_reset_endpoint_command(TRB_Reset_Endpoint_Command* reTRB) {
	printf("Reset Endpoint Command Slot ID: %d, EP ID: %d\n", reTRB->Slot_ID, reTRB->Endpoint_ID);

	return 1;
}

uint32_t handle_stop_endpoint_command(TRB_Stop_Endpoint_Command_TRB* seTRB) {
	xhci::set_ep_state(seTRB->Slot_ID, seTRB->Endpoint_ID, EP_STOPPED);

	if (xhci::internal_slot_contexts[seTRB->Slot_ID].internal_endpoint_contexts[seTRB->Endpoint_ID - 1].nr_pstreams) {
		//IMPLEMENT RESET STREAMS
		printf(" HANDLE STOP ENDOINT NOT IMPLEMENTED RESET STREAMS!\n");
	}

	return 1;
}

uint32_t handle_set_tr_dequeue_pointer_command(TRB_Set_TR_Dequeue_Pointer_Command* stTRB) {
	xhci::internal_slot_contexts[stTRB->Slot_ID].internal_endpoint_contexts[stTRB->Endpoint_ID - 1].tr_dequeue_pointer = stTRB->New_TR_Dequeue_Pointer << 4;
	xhci::internal_slot_contexts[stTRB->Slot_ID].internal_endpoint_contexts[stTRB->Endpoint_ID - 1].cycle_state = stTRB->Dequeue_Cycle_state;

	printf(" Set TR dequeue = 0x%llX cycle = %d\n", stTRB->New_TR_Dequeue_Pointer << 4, stTRB->Dequeue_Cycle_state);

	return 1;
}

uint32_t handle_reset_device_command(TRB_Reset_Device_Command* rdTRB) {
	uint32_t completion_code = 1;

	for (int i = 2; i <= 31; i++)
		if (xhci::internal_slot_contexts[rdTRB->Slot_ID].internal_endpoint_contexts[i - 1].state != EP_DISABLED)
			xhci::disable_ep(rdTRB->Slot_ID, i);
	

	if (xhci::internal_slot_contexts[rdTRB->Slot_ID].slot_state == SlotState::DEFAULT) {
		printf("SLOT_ID: %d, Slot State is already DEFAULT!\n", rdTRB->Slot_ID);
		completion_code = 19; //Context State Error
	}
	else {
		printf("SLOT_ID: %d, Slot State is updated to DEFAULT!\n", rdTRB->Slot_ID);
		xhci::internal_slot_contexts[rdTRB->Slot_ID].slot_state = SlotState::DEFAULT;

		uint32_t slot_ctx[4];
		uint64_t p_slot_ctx = xhci::internal_slot_contexts[rdTRB->Slot_ID].device_context;

		memory::read_raw(p_slot_ctx, sizeof(slot_ctx), (PBYTE)&slot_ctx);

		slot_ctx[3] &= ~(SLOT_STATE_MASK << SLOT_STATE_SHIFT);
		slot_ctx[3] |= SLOT_DEFAULT << SLOT_STATE_SHIFT;

		memory::write_raw(p_slot_ctx, sizeof(slot_ctx), (PBYTE)&slot_ctx);
	}

	return completion_code;
}

void xhci::consume_command_ring(BARLogEntry* entry) {
	printf("[+] Processing Command Ring!\n");
	bool next = true;
	while (next) {
		uint64_t pRingEntry = (xhci::opReg->CRCR.Command_Ring_Pointer << 6) + 0x10 * xhci::command_ring_done_index;
		void* ring_entry = malloc(0x10);
		if (ring_entry == 0) {
			printf("[!] failed to allocate command ring buffer!\n");
			return;
		}
		if (!LcRead(memory::hLC, pRingEntry, 0x10, (PBYTE)ring_entry)) {
			printf("[!] Failed to LcRead for Command Ring TRB Entry!\n");
			return;
		}
		TRB_Unknown* uTRB = (TRB_Unknown*)ring_entry;
		
		printf("[+] Command Entry: 0x%llX, Type: %d(%s), CB: %d\n", pRingEntry, uTRB->TRB_Type, utils::get_trb_type_name(uTRB->TRB_Type).c_str(), uTRB->Cycle_Bit);
		if (uTRB->Cycle_Bit != xhci::opReg->CRCR.Ring_Cycle_State) {
			next = false;
			break;
		}

		uint32_t completion_code = 0;
		uint32_t cmd_slot_id = 0;

		//run TRB process
		if (uTRB->TRB_Type == 6) {//Link TRB
			TRB_Link_TRB* link_trb = (TRB_Link_TRB*)ring_entry;
			uint64_t new_pointer = link_trb->Ring_Segment_Pointer << 4;
			xhci::opReg->CRCR.Command_Ring_Pointer = new_pointer >> 6;
			xhci::command_ring_done_index = 0;
			if (link_trb->Toggle_cycle) {
				if (xhci::opReg->CRCR.Ring_Cycle_State == 1) {
					xhci::opReg->CRCR.Ring_Cycle_State = 0;
				}
				else {
					xhci::opReg->CRCR.Ring_Cycle_State = 1;
				}
			}
			continue;
		}
		else if (uTRB->TRB_Type == 9) {//Enable Slot Command TRB
			TRB_Enable_Slot_Command* iTRB = (TRB_Enable_Slot_Command*)ring_entry;
			std::string log_enable_slot_cmd = std::format("[CMD] Enable Slot Command => SlotType: {:d} {:d}", (int)iTRB->Slot_Type, (int)iTRB->TRB_Type);
			printf("%s\n", log_enable_slot_cmd.c_str());
			entry->descs.push_back(log_enable_slot_cmd);

			int selected_slot_id = -1;

			for (auto& [slot_id, isc] : xhci::internal_slot_contexts) {
				if (isc.slot_state == SlotState::DISABLE) {
					selected_slot_id = slot_id;
					isc.slot_state = SlotState::ENABLE;
					memset(isc.internal_endpoint_contexts, 0, sizeof(InternalEndpointContext) * 31);
					break;
				}
			}

			if (selected_slot_id == -1) {
				printf("THERE IS NO UN USED SLOT ID!\n");
				completion_code = 9;
			}
			else {
				printf("ASSIGNING SLOT ID: %d\n", selected_slot_id);

				if (xhci::internal_state == InternalState::START)
					xhci::internal_state = InternalState::ENABLE_SLOT;

				completion_code = 1;
				cmd_slot_id = selected_slot_id;
			}
		}
		else if (uTRB->TRB_Type == 10) {//Disable Slot Command TRB
			TRB_Disable_Slot_Command* dsTRB = (TRB_Disable_Slot_Command*)ring_entry;

			cmd_slot_id = dsTRB->Slot_ID;
			completion_code = handle_disable_slot_command(dsTRB);
		}
		else if (uTRB->TRB_Type == 11) {//Address Device Command TRB
			TRB_Address_Device_Command* adTRB = (TRB_Address_Device_Command*)ring_entry;
			
			cmd_slot_id = adTRB->Slot_ID;
			completion_code = handle_address_device_command(adTRB);
		}
		else if (uTRB->TRB_Type == 12) {//Configure Endpoint Command TRB
			TRB_Configure_Endpoint_Command* ceTRB = (TRB_Configure_Endpoint_Command*)ring_entry;
			
			cmd_slot_id = ceTRB->Slot_ID;
			completion_code = handle_configure_endpoint_command(ceTRB);
		}
		else if (uTRB->TRB_Type == 14) {//Reset Endpoint Command TRB
			TRB_Reset_Endpoint_Command* reTRB = (TRB_Reset_Endpoint_Command*)ring_entry;

			cmd_slot_id = reTRB->Slot_ID;
			completion_code = handle_reset_endpoint_command(reTRB);
		}
		else if (uTRB->TRB_Type == 15) {//Stop Endpoint Command TRB
			TRB_Stop_Endpoint_Command_TRB* seTRB = (TRB_Stop_Endpoint_Command_TRB*)ring_entry;

			cmd_slot_id = seTRB->Slot_ID;
			completion_code = handle_stop_endpoint_command(seTRB);
		}
		else if (uTRB->TRB_Type == 16) {//Set TR Dequeue Pointer Command
			TRB_Set_TR_Dequeue_Pointer_Command* stTRB = (TRB_Set_TR_Dequeue_Pointer_Command*)ring_entry;
			
			cmd_slot_id = stTRB->Slot_ID;
			completion_code = handle_set_tr_dequeue_pointer_command(stTRB);
		}
		else if (uTRB->TRB_Type == 17) {//Reset Device Command TRB
			TRB_Reset_Device_Command* rdTRB = (TRB_Reset_Device_Command*)ring_entry;

			cmd_slot_id = rdTRB->Slot_ID;
			completion_code = handle_reset_device_command(rdTRB);
		}

		if (completion_code != 0) {
			xhci::send_command_completion_event(pRingEntry >> 4, cmd_slot_id, completion_code);
		}

		//update next TRB
		xhci::command_ring_done_index += 1;
	}
}

uint64_t last_data_stage_trb_ptr = 0x0;
uint64_t last_status_stage_trb_ptr = 0x0;

TRB_Setup_Stage  last_setup_trb;
TRB_Data_Stage   last_data_trb;
TRB_Status_Stage last_status_trb;

/*
    1: bmRequestType
    2: bRequest
    3: w_value
	4: w_index
*/

//Status : 2
unsigned char data_80_00_0000_0000[] = { 0x0, 0x0 };

//Device Descriptor : 18
unsigned char data_80_06_0100_0000[] = { 0x12, 0x1, 0x0, 0x2, 0xef, 0x2, 0x1, 0x40, 0x4d, 0x53, 0x9, 0x21, 0x0, 0x21, 0x1, 0x2, 0x0, 0x1 };
//Configuration Descriptor : 1211
unsigned char data_80_06_0200_0000[] = { 0x9, 0x2, 0xbb, 0x4, 0x5, 0x1, 0x0, 0x80, 0xfa, 0x8, 0xb, 0x0, 0x2, 0xe, 0x3, 0x0, 0x2, 0x9, 0x4, 0x0, 0x0, 0x0, 0xe, 0x1, 0x0, 0x2, 0xd, 0x24, 0x1, 0x0, 0x1, 0x33, 0x0, 0x0, 0x6c, 0xdc, 0x2, 0x1, 0x1, 0x12, 0x24, 0x2, 0x1, 0x1, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x0, 0xb, 0x24, 0x5, 0x2, 0x1, 0x0, 0x0, 0x2, 0xf, 0x0, 0x0, 0x9, 0x24, 0x3, 0x3, 0x1, 0x1, 0x0, 0x2, 0x0, 0x9, 0x4, 0x1, 0x0, 0x0, 0xe, 0x2, 0x0, 0x0, 0xf, 0x24, 0x1, 0x2, 0xb1, 0x3, 0x83, 0x0, 0x3, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0xb, 0x24, 0x6, 0x1, 0xb, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x2e, 0x24, 0x7, 0x1, 0x0, 0x80, 0x7, 0x38, 0x4, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x48, 0x3f, 0x0, 0xa, 0x8b, 0x2, 0x0, 0x5, 0xa, 0x8b, 0x2, 0x0, 0x15, 0x16, 0x5, 0x0, 0x80, 0x1a, 0x6, 0x0, 0x20, 0xa1, 0x7, 0x0, 0x40, 0x42, 0xf, 0x0, 0x2e, 0x24, 0x7, 0x2, 0x0, 0x40, 0x6, 0xb0, 0x4, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x98, 0x3a, 0x0, 0xa, 0x8b, 0x2, 0x0, 0x5, 0xa, 0x8b, 0x2, 0x0, 0x15, 0x16, 0x5, 0x0, 0x80, 0x1a, 0x6, 0x0, 0x20, 0xa1, 0x7, 0x0, 0x40, 0x42, 0xf, 0x0, 0x2e, 0x24, 0x7, 0x3, 0x0, 0x50, 0x5, 0x0, 0x3, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0xe0, 0x1f, 0x0, 0xa, 0x8b, 0x2, 0x0, 0x5, 0xa, 0x8b, 0x2, 0x0, 0x15, 0x16, 0x5, 0x0, 0x80, 0x1a, 0x6, 0x0, 0x20, 0xa1, 0x7, 0x0, 0x40, 0x42, 0xf, 0x0, 0x2e, 0x24, 0x7, 0x4, 0x0, 0x0, 0x5, 0x0, 0x4, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x28, 0x0, 0xa, 0x8b, 0x2, 0x0, 0x5, 0xa, 0x8b, 0x2, 0x0, 0x15, 0x16, 0x5, 0x0, 0x80, 0x1a, 0x6, 0x0, 0x20, 0xa1, 0x7, 0x0, 0x40, 0x42, 0xf, 0x0, 0x2e, 0x24, 0x7, 0x5, 0x0, 0x0, 0x5, 0xc0, 0x3, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x80, 0x25, 0x0, 0xa, 0x8b, 0x2, 0x0, 0x5, 0xa, 0x8b, 0x2, 0x0, 0x15, 0x16, 0x5, 0x0, 0x80, 0x1a, 0x6, 0x0, 0x20, 0xa1, 0x7, 0x0, 0x40, 0x42, 0xf, 0x0, 0x2e, 0x24, 0x7, 0x6, 0x0, 0x0, 0x5, 0xd0, 0x2, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x20, 0x1c, 0x0, 0xa, 0x8b, 0x2, 0x0, 0x5, 0xa, 0x8b, 0x2, 0x0, 0x40, 0xd, 0x3, 0x0, 0x15, 0x16, 0x5, 0x0, 0x20, 0xa1, 0x7, 0x0, 0x40, 0x42, 0xf, 0x0, 0x2e, 0x24, 0x7, 0x7, 0x0, 0x0, 0x4, 0x0, 0x3, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x18, 0x0, 0xa, 0x8b, 0x2, 0x0, 0x5, 0xa, 0x8b, 0x2, 0x0, 0x40, 0xd, 0x3, 0x0, 0x15, 0x16, 0x5, 0x0, 0x20, 0xa1, 0x7, 0x0, 0x40, 0x42, 0xf, 0x0, 0x2e, 0x24, 0x7, 0x8, 0x0, 0x20, 0x3, 0x58, 0x2, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0xa6, 0xe, 0x0, 0xa, 0x8b, 0x2, 0x0, 0x5, 0xa, 0x8b, 0x2, 0x0, 0x40, 0xd, 0x3, 0x0, 0x15, 0x16, 0x5, 0x0, 0x20, 0xa1, 0x7, 0x0, 0x40, 0x42, 0xf, 0x0, 0x2e, 0x24, 0x7, 0x9, 0x0, 0xd0, 0x2, 0x40, 0x2, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0xa8, 0xc, 0x0, 0xa, 0x8b, 0x2, 0x0, 0x5, 0xa, 0x8b, 0x2, 0x0, 0x40, 0xd, 0x3, 0x0, 0x15, 0x16, 0x5, 0x0, 0x20, 0xa1, 0x7, 0x0, 0x40, 0x42, 0xf, 0x0, 0x2e, 0x24, 0x7, 0xa, 0x0, 0xd0, 0x2, 0xe0, 0x1, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x8c, 0xa, 0x0, 0xa, 0x8b, 0x2, 0x0, 0x5, 0xa, 0x8b, 0x2, 0x0, 0x40, 0xd, 0x3, 0x0, 0x15, 0x16, 0x5, 0x0, 0x20, 0xa1, 0x7, 0x0, 0x40, 0x42, 0xf, 0x0, 0x2e, 0x24, 0x7, 0xb, 0x0, 0x80, 0x2, 0xe0, 0x1, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x60, 0x9, 0x0, 0xa, 0x8b, 0x2, 0x0, 0x5, 0xa, 0x8b, 0x2, 0x0, 0x40, 0xd, 0x3, 0x0, 0x15, 0x16, 0x5, 0x0, 0x20, 0xa1, 0x7, 0x0, 0x40, 0x42, 0xf, 0x0, 0x6, 0x24, 0xd, 0x1, 0x1, 0x4, 0x1b, 0x24, 0x4, 0x2, 0xb, 0x59, 0x55, 0x59, 0x32, 0x0, 0x0, 0x10, 0x0, 0x80, 0x0, 0x0, 0xaa, 0x0, 0x38, 0x9b, 0x71, 0x10, 0x6, 0x0, 0x0, 0x0, 0x0, 0x1e, 0x24, 0x5, 0x1, 0x0, 0x80, 0x7, 0x38, 0x4, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x48, 0x3f, 0x0, 0x80, 0x84, 0x1e, 0x0, 0x1, 0x80, 0x84, 0x1e, 0x0, 0x1e, 0x24, 0x5, 0x2, 0x0, 0x40, 0x6, 0xb0, 0x4, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x98, 0x3a, 0x0, 0x80, 0x84, 0x1e, 0x0, 0x1, 0x80, 0x84, 0x1e, 0x0, 0x1e, 0x24, 0x5, 0x3, 0x0, 0x50, 0x5, 0x0, 0x3, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0xe0, 0x1f, 0x0, 0xd0, 0x12, 0x13, 0x0, 0x1, 0xd0, 0x12, 0x13, 0x0, 0x1e, 0x24, 0x5, 0x4, 0x0, 0x0, 0x5, 0x0, 0x4, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x28, 0x0, 0xd0, 0x12, 0x13, 0x0, 0x1, 0xd0, 0x12, 0x13, 0x0, 0x1e, 0x24, 0x5, 0x5, 0x0, 0x0, 0x5, 0xc0, 0x3, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x80, 0x25, 0x0, 0xd0, 0x12, 0x13, 0x0, 0x1, 0xd0, 0x12, 0x13, 0x0, 0x1e, 0x24, 0x5, 0x6, 0x0, 0x0, 0x5, 0xd0, 0x2, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x20, 0x1c, 0x0, 0x40, 0x42, 0xf, 0x0, 0x1, 0x40, 0x42, 0xf, 0x0, 0x1e, 0x24, 0x5, 0x7, 0x0, 0x0, 0x4, 0x0, 0x3, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x18, 0x0, 0x40, 0x42, 0xf, 0x0, 0x1, 0x40, 0x42, 0xf, 0x0, 0x26, 0x24, 0x5, 0x8, 0x0, 0x20, 0x3, 0x58, 0x2, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0xa6, 0xe, 0x0, 0x20, 0xa1, 0x7, 0x0, 0x3, 0x20, 0xa1, 0x7, 0x0, 0x40, 0x42, 0xf, 0x0, 0x80, 0x84, 0x1e, 0x0, 0x2a, 0x24, 0x5, 0x9, 0x0, 0xd0, 0x2, 0x40, 0x2, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0xa8, 0xc, 0x0, 0x80, 0x1a, 0x6, 0x0, 0x4, 0x80, 0x1a, 0x6, 0x0, 0x20, 0xa1, 0x7, 0x0, 0x40, 0x42, 0xf, 0x0, 0x80, 0x84, 0x1e, 0x0, 0x2a, 0x24, 0x5, 0xa, 0x0, 0xd0, 0x2, 0xe0, 0x1, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x8c, 0xa, 0x0, 0x15, 0x16, 0x5, 0x0, 0x4, 0x15, 0x16, 0x5, 0x0, 0x20, 0xa1, 0x7, 0x0, 0x40, 0x42, 0xf, 0x0, 0x80, 0x84, 0x1e, 0x0, 0x2a, 0x24, 0x5, 0xb, 0x0, 0x80, 0x2, 0xe0, 0x1, 0x0, 0xb8, 0xb, 0x0, 0x0, 0x0, 0xb8, 0xb, 0x0, 0x60, 0x9, 0x0, 0x15, 0x16, 0x5, 0x0, 0x4, 0x15, 0x16, 0x5, 0x0, 0x20, 0xa1, 0x7, 0x0, 0x40, 0x42, 0xf, 0x0, 0x80, 0x84, 0x1e, 0x0, 0x6, 0x24, 0xd, 0x1, 0x1, 0x4, 0x9, 0x4, 0x1, 0x1, 0x1, 0xe, 0x2, 0x0, 0x0, 0x7, 0x5, 0x83, 0x5, 0x20, 0x3, 0x1, 0x9, 0x4, 0x1, 0x2, 0x1, 0xe, 0x2, 0x0, 0x0, 0x7, 0x5, 0x83, 0x5, 0x0, 0xc, 0x1, 0x9, 0x4, 0x1, 0x3, 0x1, 0xe, 0x2, 0x0, 0x0, 0x7, 0x5, 0x83, 0x5, 0x0, 0x14, 0x1, 0x8, 0xb, 0x2, 0x2, 0x1, 0x1, 0x0, 0x4, 0x9, 0x4, 0x2, 0x0, 0x0, 0x1, 0x1, 0x0, 0x4, 0x9, 0x24, 0x1, 0x0, 0x1, 0x26, 0x0, 0x1, 0x3, 0xc, 0x24, 0x2, 0x1, 0x2, 0x6, 0x0, 0x2, 0x3, 0x0, 0x0, 0x0, 0x8, 0x24, 0x6, 0x2, 0x1, 0x1, 0x1, 0x0, 0x9, 0x24, 0x3, 0x3, 0x1, 0x1, 0x0, 0x2, 0x0, 0x9, 0x4, 0x3, 0x0, 0x0, 0x1, 0x2, 0x0, 0x4, 0x9, 0x4, 0x3, 0x1, 0x1, 0x1, 0x2, 0x0, 0x4, 0x7, 0x24, 0x1, 0x3, 0x0, 0x1, 0x0, 0xb, 0x24, 0x2, 0x1, 0x1, 0x2, 0x10, 0x1, 0x0, 0x77, 0x1, 0x9, 0x5, 0x82, 0x5, 0x0, 0x1, 0x4, 0x0, 0x0, 0x7, 0x25, 0x1, 0x0, 0x0, 0x0, 0x0, 0x9, 0x4, 0x4, 0x0, 0x1, 0x3, 0x0, 0x0, 0x0, 0x9, 0x21, 0x10, 0x1, 0x21, 0x1, 0x22, 0x17, 0x0, 0x7, 0x5, 0x84, 0x3, 0x4, 0x0, 0x10 };
//String Descriptor Index: 0x00, lang: 0x0000 : 4
unsigned char data_80_06_0300_0000[] = { 0x4, 0x3, 0x9, 0x4 };
//String Descriptor Index: 0x01, lang: 0x0409 : 26
unsigned char data_80_06_0301_0409[] = { 0x1a, 0x3, 0x4d, 0x0, 0x41, 0x0, 0x43, 0x0, 0x52, 0x0, 0x4f, 0x0, 0x53, 0x0, 0x49, 0x0, 0x4c, 0x0, 0x49, 0x0, 0x43, 0x0, 0x4f, 0x0, 0x4e, 0x0 };
//String Descriptor Index: 0x02, lang: 0x0409 : 32
unsigned char data_80_06_0302_0409[] = { 0x20, 0x3, 0x55, 0x0, 0x53, 0x0, 0x42, 0x0, 0x33, 0x0, 0x2e, 0x0, 0x20, 0x0, 0x30, 0x0, 0x20, 0x0, 0x63, 0x0, 0x61, 0x0, 0x70, 0x0, 0x74, 0x0, 0x75, 0x0, 0x72, 0x0, 0x65, 0x0 };
//String Descriptor Index: 0x04, lang: 0x0409 : 32
unsigned char data_80_06_0304_0409[] = { 0x55, 0x0, 0x53, 0x0, 0x42, 0x0, 0x33, 0x0, 0x2e, 0x0, 0x20, 0x0, 0x30, 0x0, 0x20, 0x0, 0x63, 0x0, 0x61, 0x0, 0x70, 0x0, 0x74, 0x0, 0x75, 0x0, 0x72, 0x0, 0x65, 0x0 };
//HID Report Desciptor : 23
unsigned char data_81_06_2200_0004[] = { 0x6, 0x0, 0xff, 0x9, 0x1, 0xa1, 0x1, 0x15, 0x0, 0x26, 0xff, 0x0, 0x19, 0x1, 0x29, 0x2, 0x75, 0x8, 0x95, 0x8, 0xb1, 0x2, 0xc0 };

//USBVIDEO
//GET INFO [BRIGHTNESS] : 1
unsigned char data_A1_86_0200_0200[] = { 0x7 };
//GET INFO [CONTRAST]   : 1
unsigned char data_A1_86_0300_0200[] = { 0x7 };
//GET INFO [HUE] : 1
unsigned char data_A1_86_0600_0200[] = { 0x7 };
//GET INFO [Saturation] : 1
unsigned char data_A1_86_0700_0200[] = { 0x7 };

//GET MIN  [BRIGHTNESS] : 2
unsigned char data_A1_82_0200_0200[] = { 0x80, 0xff };
//GET MIN  [CONSTRAST]  : 2
unsigned char data_A1_82_0300_0200[] = { 0x0, 0x0 };
//GET MIN  [HUE] : 2
unsigned char data_A1_82_0600_0200[] = { 0x80, 0xff };
//GET MIN  [Saturation] : 2
unsigned char data_A1_82_0700_0200[] = { 0x0, 0x0 };

//GET MAX  [BRIGHTNESS] : 2
unsigned char data_A1_83_0200_0200[] = { 0x7f, 0x0 };
//GET MAX  [CONTRAST]   : 2
unsigned char data_A1_83_0300_0200[] = { 0xff, 0x0 };
//GET MAX  [HUE] :2
unsigned char data_A1_83_0600_0200[] = { 0x7f, 0x0 };
//GET MAX  [Saturation] :2
unsigned char data_A1_83_0700_0200[] = { 0xff, 0x0 };

//GET RES  [BRIGHTNESS] : 2
unsigned char data_A1_84_0200_0200[] = { 0x1, 0x0 };
//GET RES  [CONTRAST]   : 2
unsigned char data_A1_84_0300_0200[] = { 0x1, 0x0 };
//GET RES  [HUE] : 2
unsigned char data_A1_84_0600_0200[] = { 0x1, 0x0 };
//GET RES  [Saturation] : 2
unsigned char data_A1_84_0700_0200[] = { 0x1, 0x0 };

//GET DEF  [BRIGHTNESS] : 2
unsigned char data_A1_87_0200_0200[] = { 0xf5, 0xff };
//GET DEF  [CONTRAST]   : 2
unsigned char data_A1_87_0300_0200[] = { 0x94, 0x0 };
//GET DEF  [HUE] : 2
unsigned char data_A1_87_0600_0200[] = { 0x0, 0x0 };
//GET DEF  [Saturation] : 2
unsigned char data_A1_87_0700_0200[] = { 0xb4, 0x0 };

//GET_CUR MUTE_CONTROL : 1
unsigned char data_A1_81_0100_0202[] = { 0x0 };

//INTERFACE : 0
//01_0B_0000_0001
//01_0B_0000_0003

//SET IDLE : 0
//21_0A_0000_0004

//SET CONFIGURATION : 0
//00_09_0001_0000


void xhci::handle_transfer_ring(uint32_t offset, uint8_t ep_id, uint16_t stream_id) {
	printf("[+] Processing Transfer Ring!\n");
	printf(" DOORBELL HIT: 0x%X, ep_id: %d, stream_id: %d\n", offset, ep_id, stream_id);
	int slot_id = offset / 0x4;
	uint64_t p_dev_ctx = memory::readt_raw<uint64_t>(opReg->DCBAAP + 8 * slot_id);
	uint64_t p_ep_ctx = p_dev_ctx + 0x20;
	if (ep_id != 1) {
		printf("!!!!!!!!!! ERROR EP ID != 1 \n");
		return;
	}

	bool next = true;
	
	/*if (xhci::ep0_dequeue_pointer.find(slot_id - 1) == xhci::ep0_dequeue_pointer.end()) {
		EndpointContext ep_ctx = memory::readt_raw<EndpointContext>(p_ep_ctx);
		xhci::ep0_dequeue_pointer[slot_id - 1] = ep_ctx.TR_Dequeue_Pointer << 4;
		xhci::ep0_cycle_state[slot_id - 1] = ep_ctx.Dequeue_Cycle_State;
	}*/

	InternalEndpointContext* iec = &xhci::internal_slot_contexts[slot_id].internal_endpoint_contexts[ep_id - 1];

	uint64_t tr_dequeue_pointer = iec->tr_dequeue_pointer;//xhci::ep0_dequeue_pointer[slot_id - 1];
	uint64_t cycle_state = iec->cycle_state;//xhci::ep0_cycle_state[slot_id - 1];

	if (tr_dequeue_pointer == 0) {
		EndpointContext ep_ctx = memory::readt_raw<EndpointContext>(p_ep_ctx);
		tr_dequeue_pointer = ep_ctx.TR_Dequeue_Pointer << 4;
		cycle_state = ep_ctx.Dequeue_Cycle_State;
	}

	printf(" TR Dequeue Pointer: 0x%llX\n", tr_dequeue_pointer);
	printf(" TR Dequeue Cycle State: %lld\n", cycle_state);

	TRB_Unknown uTRB;

	while (next) {
		if (!LcRead(memory::hLC, tr_dequeue_pointer, sizeof(TRB_Unknown), (PBYTE)&uTRB)) {
			printf("FAILED TO READ TRANSFER RING ENTRY: 0x%llX\n", tr_dequeue_pointer);
			return;
		}

		if (uTRB.Cycle_Bit != cycle_state) {
			next = false;
			break;
		}

		printf("[+] Transfer Entry: 0x%llX, Type: %d(%s), CB: %d\n", tr_dequeue_pointer, uTRB.TRB_Type, utils::get_trb_type_name(uTRB.TRB_Type).c_str(), uTRB.Cycle_Bit);

		if (uTRB.TRB_Type == 2) {
			TRB_Setup_Stage* trb = (TRB_Setup_Stage*)&uTRB;
			printf("==== Setup Stage TRB ====\n");
			printf(" bm Request Type: %d\n", trb->bm_Request_Type);
			printf(" b Request: %d\n", trb->b_Request);
			printf(" w Value: %d\n", trb->w_Value);
			printf(" w Index: %d\n", trb->w_Index);
			printf(" w Length: %d\n", trb->w_Length);
			printf(" TRB Trasnfer Length: %d\n", trb->TRB_Transfer_Length);
			printf(" Interrupter Target: %d\n", trb->Interrupter_Target);
			printf(" Interrupt In Completion: %d\n", trb->Interrupt_On_Completion);
			printf(" Immediate Data: %d\n", trb->Immediate_Data);
			std::string transfer_type = "unknown";
			if (trb->Transfer_Type == 0)
				transfer_type = "No Data Stage";
			else if (trb->Transfer_Type == 1)
				transfer_type = "Reserved";
			else if (trb->Transfer_Type == 2)
				transfer_type = "OUT Data Stage";
			else if (trb->Transfer_Type == 3)
				transfer_type = "IN Data Stage";
			printf(" Transfer Type: %s\n", transfer_type.c_str());
			printf("========== END ==========\n");

			last_setup_trb = *trb;
		}
		else if (uTRB.TRB_Type == 3) {
			TRB_Data_Stage* trb = (TRB_Data_Stage*)&uTRB;
			printf("==== Data Stage TRB ====\n");
			printf(" Data Buffer Pointer: 0x%llX\n", trb->Data_Buffer_Pointer);
			printf(" TRB Transfer Length: %d\n", trb->TRB_Transfer_Length);
			printf(" Interrupter Target: %d\n", trb->Interrupter_Target);
			printf(" Evaluate Next TRB: %d\n", trb->Evaluate_Next_TRB);
			printf(" Interrupt on Short Packet: %d\n", trb->Interrupt_on_Short_Packet);
			printf(" No Snoop: %d\n", trb->No_Snoop);
			printf(" Chain bit: %d\n", trb->Chain_bit);
			printf(" Interrupt On Completion: %d\n", trb->Interrupt_On_Completion);
			printf(" Immediate Data: %d\n", trb->Immediate_data);
			printf(" Direction: %d\n", trb->Direction);
			printf("========= END ==========\n");
			last_data_stage_trb_ptr = tr_dequeue_pointer;
			last_data_trb  = *trb;
		}
		else if (uTRB.TRB_Type == 4) {
			TRB_Status_Stage* trb = (TRB_Status_Stage*)&uTRB;
			printf("==== Status Stage TRB ====\n");
			printf(" Interrupter Target: %d\n", trb->Interrupter_Target);
			printf(" Evaluate Next TRB: %d\n", trb->Evaluate_Next_TRB);
			printf(" Chain bit: %d\n", trb->Chain_bit);
			printf(" Interrupt On Completion: %d\n", trb->Interrupt_On_Completion);
			printf(" Direction: %d\n", trb->Direction);
			printf("========== END ==========\n");
			last_status_stage_trb_ptr = tr_dequeue_pointer;

			last_status_trb = *trb;

			TRB_Transfer_Event transfer_event = {};
			transfer_event.TRB_Pointer = last_status_stage_trb_ptr;
			transfer_event.TRB_Type = 32;
			transfer_event.Endpoint_ID = ep_id;
			transfer_event.Slot_ID = slot_id;
			transfer_event.Event_Data = 0;

			int length = -1;
			void *data_response = nullptr;

			uint8_t  bm_request_type = last_setup_trb.bm_Request_Type;
			uint8_t  b_request       = last_setup_trb.b_Request;
			uint16_t w_value         = last_setup_trb.w_Value;
			uint16_t w_index         = last_setup_trb.w_Index;
			// GET STATUS
			if (bm_request_type == 0x80 && b_request == 0x00) {
				if (w_value == 0x00 && w_index == 0x00) {
					length = 2;
					data_response = data_80_00_0000_0000;
				}
			}
			// GET DESCRIPTOR
			else if (bm_request_type == 0x80 && b_request == 0x06) {
				if (w_value == 0x0100 && w_index == 0x0000) {
					length = 18;
					data_response = data_80_06_0100_0000;
				}
				//else if (w_value == 0x0200 && w_index == 0x0000) {
				else if (w_value == 0x0200) {
					length = 1211;
					data_response = data_80_06_0200_0000;
				}
				else if (w_value == 0x0300 && w_index == 0x0000) {
					length = 4;
					data_response = data_80_06_0300_0000;
				}
				else if (w_value == 0x0301 && w_index == 0x0409) {
					length = 26;
					data_response = data_80_06_0301_0409;
				}
				else if (w_value == 0x0302 && w_index == 0x0409) {
					length = 32;
					data_response = data_80_06_0302_0409;
				}
				else if (w_value == 0x0304 && w_index == 0x0409) {
					length = 32;
					data_response = data_80_06_0304_0409;
				}
			}
			else if (bm_request_type == 0x81 && b_request == 0x06) {
				if (w_value == 0x2200 && w_index == 0x0004) {
					length = 23;
					data_response = data_81_06_2200_0004;
				}
			}
			else if (bm_request_type == 0xA1 && b_request == 0x86 && w_index == 0x0200) {
				if (w_value == 0x0200) {
					length = 1;
					data_response = data_A1_86_0200_0200;
				}
				else if (w_value == 0x0300) {
					length = 1;
					data_response = data_A1_86_0300_0200;
				}
				else if (w_value == 0x0600) {
					length = 1;
					data_response = data_A1_86_0600_0200;
				}
				else if (w_value == 0x0700) {
					length = 1;
					data_response = data_A1_86_0700_0200;
				}
			}
			else if (bm_request_type == 0xA1 && b_request == 0x82 && w_index == 0x0200) {
				if (w_value == 0x0200) {
					length = 2;
					data_response = data_A1_82_0200_0200;
				}
				else if (w_value == 0x0300) {
					length = 2;
					data_response = data_A1_82_0300_0200;
				}
				else if (w_value == 0x0600) {
					length = 2;
					data_response = data_A1_82_0600_0200;
				}
				else if (w_value == 0x0700) {
					length = 2;
					data_response = data_A1_82_0700_0200;
				}
			}
			else if (bm_request_type == 0xA1 && b_request == 0x83 && w_index == 0x0200) {
				if (w_value == 0x0200) {
					length = 2;
					data_response = data_A1_83_0200_0200;
				}
				else if (w_value == 0x0300) {
					length = 2;
					data_response = data_A1_83_0300_0200;
				}
				else if (w_value == 0x0600) {
					length = 2;
					data_response = data_A1_83_0600_0200;
				}
				else if (w_value == 0x0700) {
					length = 2;
					data_response = data_A1_83_0700_0200;
				}
			}
			else if (bm_request_type == 0xA1 && b_request == 0x84 && w_index == 0x0200) {
				if (w_value == 0x0200) {
					length = 2;
					data_response = data_A1_84_0200_0200;
				}
				else if (w_value == 0x0300) {
					length = 2;
					data_response = data_A1_84_0300_0200;
				}
				else if (w_value == 0x0600) {
					length = 2;
					data_response = data_A1_84_0600_0200;
				}
				else if (w_value == 0x0700) {
					length = 2;
					data_response = data_A1_84_0700_0200;
				}
			}
			else if (bm_request_type == 0xA1 && b_request == 0x87 && w_index == 0x0200) {
				if (w_value == 0x0200) {
					length = 2;
					data_response = data_A1_87_0200_0200;
				}
				else if (w_value == 0x0300) {
					length = 2;
					data_response = data_A1_87_0300_0200;
				}
				else if (w_value == 0x0600) {
					length = 2;
					data_response = data_A1_87_0600_0200;
				}
				else if (w_value == 0x0700) {
					length = 2;
					data_response = data_A1_87_0700_0200;
				}
			}
			else if (bm_request_type == 0xA1 && b_request == 0x81 && w_value == 0x0100 && w_index == 0x0202) {
				length = 1;
				data_response = data_A1_81_0100_0202;
			}
			else if (bm_request_type == 0x01 && b_request == 0x0B && w_value == 0x0000) {
				if (w_index == 0x0001 || w_index == 0x0003) {
					length = 0;
				}
			}
			else if (bm_request_type == 0x21 && b_request == 0x0A && w_value == 0x0000 && w_index == 0x0004) {
				length = 0;
			}
			else if (bm_request_type == 0x00 && b_request == 0x09 && w_value == 0x0001 && w_index == 0x0000) {
				length = 0;
			}
			

			if (length != -1) {
				if (length > last_setup_trb.w_Length)
					length = last_setup_trb.w_Length;

				if (length != 0)
					memory::write_raw(last_data_trb.Data_Buffer_Pointer, length, (PBYTE)data_response);

				transfer_event.TRB_Transfer_Length = 0;

				// Interrupt on Short Packet
				if (last_data_trb.Interrupt_on_Short_Packet && last_setup_trb.w_Length > length) {
					//send cplt to data packet
					//completion code = 0xD(13)

					TRB_Transfer_Event short_transfer_event = {};
					short_transfer_event.TRB_Pointer = last_data_stage_trb_ptr;
					short_transfer_event.TRB_Type = 32;
					short_transfer_event.Endpoint_ID = ep_id;
					short_transfer_event.Slot_ID = slot_id;
					short_transfer_event.Event_Data = 0;
					short_transfer_event.Completion_Code = 0xD;
					short_transfer_event.TRB_Transfer_Length = last_setup_trb.w_Length - length;
					xhci::send_event_ring(0, (PBYTE)&short_transfer_event);
				}
				else {
					transfer_event.TRB_Transfer_Length = last_setup_trb.w_Length - length;
				}
				transfer_event.Completion_Code = 0x1;
			} else {
				TRB_Generic* gTRB = (TRB_Generic*)&last_setup_trb;
				printf("[ERROR] UN HANDLED TRANSFER bmRequest: %02X, bRequest: %02X, wValue: %04X, wIndex: %04X, len: %d\n",
					last_setup_trb.bm_Request_Type,
					last_setup_trb.b_Request,
					last_setup_trb.w_Value,
					last_setup_trb.w_Index,
					last_setup_trb.w_Length);
				printf("%08X %08X %08X %08X\n", gTRB->fields[0], gTRB->fields[1], gTRB->fields[2], gTRB->fields[3]);
				transfer_event.TRB_Transfer_Length = last_setup_trb.w_Length;
				transfer_event.Completion_Code = 0x6;
			}

			if (last_setup_trb.Interrupt_On_Completion) {
				xhci::send_event_ring(0, (PBYTE)&transfer_event);
			}
		}
		else if (uTRB.TRB_Type == 7) {
			TRB_Event_Data* trb = (TRB_Event_Data*)&uTRB;
			printf("===== EVENT DATA TRB =====\n");
			printf("Event Data Pointer: 0x%llX\n", trb->Event_Data);
			printf("Interrupter Target: 0x%d\n", trb->Interrupter_Target);
			printf("Evaluate Next TRB: %d\n", trb->Evaluate_Next_TRB);
			printf("Chain bit: %d\n", trb->Chain_bit);
			printf("Interrupt on completion: %d\n", trb->Interrupt_On_Completion);
			printf("Block Event Interrupt: %d\n", trb->Block_Event_Interrupt);
			printf("==========================\n");

			if (trb->Interrupt_On_Completion) {
				TRB_Transfer_Event trb_completion = {};
				trb_completion.TRB_Pointer = trb->Event_Data;
				trb_completion.TRB_Type = 32;
				trb_completion.Endpoint_ID = ep_id;
				trb_completion.Slot_ID = slot_id;
				trb_completion.Event_Data = 1;
				trb_completion.Completion_Code = 0x1;
				trb_completion.TRB_Transfer_Length = last_setup_trb.w_Length;
				xhci::send_event_ring(trb->Interrupter_Target, (PBYTE)&trb_completion);
			}
		}

		tr_dequeue_pointer += 0x10;
	}

	iec->tr_dequeue_pointer = tr_dequeue_pointer;//xhci::ep0_dequeue_pointer[slot_id - 1] = tr_dequeue_pointer;
	iec->cycle_state = cycle_state;//xhci::ep0_cycle_state[slot_id - 1] = cycle_state;
}