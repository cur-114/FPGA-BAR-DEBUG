#include "xhci.hpp"
#include "memory.hpp"
#include "utils.hpp"
#include "app.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <thread>
#include <format>

namespace memory {
	
	void callback(_Inout_ PLC_BAR_REQUEST pReq);
	void cb_bar0(_Inout_ PLC_BAR_REQUEST pReq, BARLogEntry* entry);
	void cb_bar_0_v2(_Inout_ PLC_BAR_REQUEST pReq, BARLogEntry* entry);
	void cb_bar2(_Inout_ PLC_BAR_REQUEST pReq);
	void cb_bar4(_Inout_ PLC_BAR_REQUEST pReq);

	template<typename T>
	void copy_bits(T* data, T new_data, int low, int high) {
		T mask = ((static_cast<T>(1) << (high - low + 1)) - 1) << low;

		*data = (*data & ~mask) | (new_data & mask);
	}

	template<typename T>
	void set_bits(T* data, int low, int high) {
		T mask = ((static_cast<T>(1) << (high - low + 1)) - 1) << low;

		*data |= mask;
	}

	template<typename T>
	void clear_bits(T* data, int low, int high) {
		T mask = ~(((static_cast<T>(1) << (high - low + 1)) - 1) << low);

		*data &= mask;
	}

	template<typename T>
	bool rw1c(T* data, T new_data, int bit) {
		T mask = static_cast<T>(1) << bit;
		if ((new_data & mask) && (*data & mask)) {
			*data &= ~mask;  // そのビットをクリア
			return true;     // 実際にクリアされた
		}
		return false;        // クリアされなかった
	}
	//template<typename T>
	//void rw1c(T* data, T new_data, int bit) {
	//	T mask = ~(static_cast<T>(1) << bit);
	//	if (new_data >> bit) *data &= mask;
	//}

	template<typename T>
	bool rw1s(T* data, T new_data, int bit) {
		T mask = static_cast<T>(1) << bit;
		if (new_data & mask) {
			*data |= mask;
			return true;
		}
		return false;
	}

	template<typename T>
	T get_bit(T data, int bit) {
		return (data >> bit) & 0x1;
	}

	template<typename T>
	int get_bit_change(T old_value, T new_value, int bit) {
		T ov = (old_value >> bit) & 0x1;
		T nv = (new_value >> bit) & 0x1;
		if (!ov && nv)
			return 1;
		else if (ov && !nv)
			return -1;
		return 0;
	}

	void alive_entry() {
		HostControllerCapabilityRegisters* capRegister = (HostControllerCapabilityRegisters*)(bar0.data());
		HostControllerOperationalRegisters* opRegister = (HostControllerOperationalRegisters*)(bar0.data() + capRegister->CAPLENGTH);
		while (!app::exit) {
			Sleep(100);
			{
				//keep alive
				uint32_t buf;
				read_alive = LcRead(hLC, 0x1000, 4, (PBYTE)&buf);
				if (opRegister->DCBAAP != 0) {
					LcRead(hLC, opRegister->DCBAAP, sizeof(memory::DCBAA), (PBYTE)memory::DCBAA);
					for (int i = 0; i < 9; i++) {
						if (memory::DCBAA[i] != 0) {
							LcRead(hLC, memory::DCBAA[i], sizeof(DeviceContext), (PBYTE)&memory::DeviceContexts[i]);
						}
					}
				}
			}


			if (memory::control::examine_command_ring) {
				memory::control::examine_command_ring = false;

				printf("========Command Ring INFO========\n");
				void* buf = malloc(0x10);
				if (buf) {
					uint64_t ring_entry = opRegister->CRCR.Command_Ring_Pointer << 6;
					bool next = true;
					while (next) {
						LcRead(hLC, ring_entry, 0x10, (PBYTE)buf);
						uint8_t type = (*(uint16_t*)((unsigned char*)buf + 0xC)) >> 10;
						uint8_t cycle_bit = (*(uint16_t*)((unsigned char*)buf + 0xC)) & 0x1;
						if (type == 6) {
							next = false;
						}
						std::string type_name = "undefined";
						switch (type) {
						case 6:
							type_name = "Link";
							break;
						case 33:
							type_name = "Command Completion Event";
							break;
						case 34:
							type_name = "Port Status Change Event";
							break;
						}
						
						printf("ENTRY 0x%llX => %d(%s) CB: %d\n", (void *)ring_entry, type, type_name.c_str(), cycle_bit);
						if (type == 6) {
							TRB_Link_TRB* link_trb = (TRB_Link_TRB*)buf;
							printf(" Ring Segment Pointer: 0x%llX\n", link_trb->Ring_Segment_Pointer << 4);
							printf(" Interrupter Target: %d\n", link_trb->Interrupter_Target);
							printf(" Toggle Cycle : %d\n", link_trb->Toggle_cycle);
							printf(" Chain Bit: %d\n", link_trb->Chain_Bit);
							printf(" Interrupt On Completion: %d\n", link_trb->Interrupt_On_Completion);
						}
						ring_entry += 0x10;
					}
				}

				printf("===============END===============\n");
			}
			if (memory::control::examine_event_ring_0) {
				memory::control::examine_event_ring_0 = false;

				void* buffer = malloc(0x10);
				TRB_Unknown* uTRB = (TRB_Unknown*)buffer;
				if (!buffer) {
					printf("[! Event Ring] Failed to allocate buffer for TRB!\n");
					return;
				}

				int ERSTE_count = xhci::rtReg->InterrupterRegisters[0].ERSTSZ.Event_Ring_Segment_Table_Size;
				std::vector<Event_Ring_Segment_Table_Entry> ERSTEs(ERSTE_count);
				if (!LcRead(memory::hLC, xhci::rtReg->InterrupterRegisters[0].ERSTBA.Event_Ring_Segment_Table_Base_Address_Register << 6, sizeof(Event_Ring_Segment_Table_Entry) * ERSTE_count, (PBYTE)ERSTEs.data())) {
					printf("[! Event Ring] Failed to read EventRingSegmentTableEntries!\n");
					return;
				}

				for (int i = 0; i < ERSTEs.size(); i++) {
					printf("TABLE %d Size => %d !\n", i, ERSTEs[i].Ring_Segment_Size);
					for (int j = 0; j < ERSTEs[i].Ring_Segment_Size; j++) {
						uint64_t pRingEntry = (ERSTEs[i].Ring_Segment_Base_Address << 6) + 0x10 * j;
						LcRead(memory::hLC, pRingEntry, 0x10, (PBYTE)buffer);
						std::string type_name = utils::get_trb_type_name(uTRB->TRB_Type);
						printf("%d => ENTRY 0x%llX => %d(%s) CB: %d\n", i, (void*)pRingEntry, uTRB->TRB_Type, type_name.c_str(), uTRB->Cycle_Bit);
					}
				}
				free(buffer);
			}
			if (memory::control::dump_vendor_defined_caps) {
				memory::control::dump_vendor_defined_caps = false;
				/*
				printf("interface xHCI_ExCaps;\n");
				for (uint64_t pos = 0x8040; pos < 0x8330; pos += 0x4) {
					uint32_t data = *(uint32_t*)(memory::bar0.data() + pos);
					//printf("%08llX => %08llX\n", pos, data);
					//printf("                    32'h%08llX : rd_rsp_data <= 32'h%08llX;\n", pos, data);
					printf("    reg [31:0] reg_%04llX;\n", pos);
				}
				printf("endinterface\n");*/
				/*
				for (uint32_t pos = 0x8040; pos < 0x8070; pos += 0x8) {
					uint64_t data_0 = *(uint64_t*)(memory::bar0.data() + pos);
					printf("            if_ex_cap.vd_cap_1[%d:%d] <= 64'h%016llX;\n", (pos - 0x8040) * 8 + 63, (pos - 0x8040) * 8, data_0);
				}
				printf("\n");
				for (uint32_t pos = 0x8070; pos < 0x8330; pos += 0x8) {
					uint64_t data_0 = *(uint64_t*)(memory::bar0.data() + pos);
					printf("            if_ex_cap.vd_cap_2[%d:%d] <= 64'h%016llX;\n", (pos - 0x8070) * 8 + 63, (pos - 0x8070) * 8, data_0);
				}
				printf("\n");*/
				for (uint32_t pos = 0x480; pos < 0x500; pos += 0x4) {
					uint32_t index = (pos - 0x480) / 0x4;
					uint32_t data_0 = *(uint32_t*)(memory::bar0.data() + pos);
					printf("            if_op_reg.port_regs[%d] <= 32'h%08X;\n", index, data_0);
				}
				
				//for (int i = 0; i < 64; i++) {
				//	printf("if_rt_reg.interrupter_regs[%d] = 32'h0;\n", i);
				//}
			}
		}
	}

	bool initialize() {
		//LPCSTR szParams[] = { "-device", "fpga", "-memmap", "auto" };
		//hVMM = VMMDLL_Initialize(4, szParams);
		//if (!hVMM) {
		//	printf("Failed to initialize memory process file system in call to vmm.dll!VMMDLL_Initialize (Error: %d)\n", GetLastError());
		//	return false;
		//}
		LC_CONFIG LcConfig = {
			.dwVersion = LC_CONFIG_VERSION,
			.szDevice = "fpga"
			//.szDevice = "existing"
		};
		hLC = LcCreate(&LcConfig);
		if (!hLC) {
			printf("Failed to initialize LeechCore Handle!\n");
			//return false;
		}
		//loadFromFile("C:\\Users\\cur\\Documents\\CHAIR\\FIRMWARE\\PCIE-USB\\bar_dump.bin", bar0);
		//loadFromFile("C:\\Users\\cur\\Documents\\CHAIR\\FIRMWARE\\PCIE-USB\\bar_dump_XHCI.bin", bar0);
		
		//loadFromFile("C:\\Users\\cur\\Documents\\CHAIR\\FIRMWARE\\PCIE-USB\\BAR-F6700000-F670FFFF.bin", bar0);
		
		//loadFromFile("C:\\Users\\cur\\Documents\\CHAIR\\FIRMWARE\\PCIE-USB\\M00000000F6900000.bin", bar0);
		loadFromFile("C:\\Users\\cur\\Documents\\CHAIR\\FIRMWARE\\PCIE-USB\\BAR0.bin", bar0);
		//loadFromFile("C:\\Users\\cur\\Documents\\CHAIR\\FIRMWARE\\PCIE-USB\\dump_bar0_FL1100_keyboard.bin", bar0);
		//loadFromFile("C:\\Users\\cur\\Documents\\CHAIR\\FIRMWARE\\PCIE-USB\\dump_bar0_FL1100_USBTTL.bin", bar0);

		bar2.resize(1024 * 4);
		bar4.resize(1024 * 4);

		xhci::init_pointers();
		if (hLC)
			LcCommand(hLC, LC_CMD_FPGA_BAR_FUNCTION_CALLBACK, 0, (PBYTE)callback, NULL, NULL);
		std::thread alive(alive_entry);
		alive.detach();
		for (int i = 0; i < 8; i++) {
			Producer_Cycle_State[i] = false;
		}
		return true;
	}

	void cleanup() {
		LcCommand(hLC, LC_CMD_FPGA_BAR_FUNCTION_CALLBACK, 0, (PBYTE)LC_BAR_FUNCTION_CALLBACK_DISABLE, NULL, NULL);
		//VMMDLL_Close(hVMM);
	}

	bool loadFromFile(std::string filename, std::vector<unsigned char> &bar) {
		std::ifstream file(filename, std::ios::binary);
		if (!file) {
			std::cerr << "Error opening file: " << filename << std::endl;
			return false;
		}

		file.seekg(0, std::ios::end);
		std::streampos fileSize = file.tellg();
		file.seekg(0, std::ios::beg);
		bar.resize(fileSize);
		file.read(reinterpret_cast<char*>(bar.data()), fileSize);
		file.close();
		printf("Loaded BAR %d bytes from %s!\n", bar.size(), filename.c_str());

		return true;
	}

	void callback(_Inout_ PLC_BAR_REQUEST pReq) {
		unsigned char* buffer;
		unsigned char* data_pos = nullptr;
		DWORD len = pReq->cbData;
		switch (pReq->pBar->iBar) {
		case 0:
			data_pos = bar0.data() + pReq->oData;
			break;
		case 2:
			data_pos = bar2.data() + pReq->oData;
			break;
		case 4:
			data_pos = bar4.data() + pReq->oData;
			break;
		default:
			printf("Unhandled request to BAR%d\n", pReq->pBar->iBar);
			return;
		}
		int type = -1;
		if (pReq->fWrite) {
			buffer = (unsigned char*)malloc(len * 2);
			memcpy_s(buffer, len, data_pos, len);
			memcpy_s((buffer + len), len, pReq->pbData, len);
			type = 1;
		}
		else {
			buffer = (unsigned char*)malloc(len);
			memcpy_s(buffer, len, data_pos, len);
			type = 0;
		}

		BARLogEntry entry = {
			pReq->pBar->iBar,
			pReq->oData,
			pReq->cbData,
			type,
			buffer
		};
		switch (pReq->pBar->iBar) {
		case 0:
			cb_bar_0_v2(pReq, &entry);
			break;
		case 2:
			cb_bar2(pReq);
			break;
		case 4:
			cb_bar4(pReq);
			break;
		}
		bar_logs.push_back(entry);
	}

	/*void send_event_ring(PBYTE trb, DWORD trb_size) {
		HostControllerCapabilityRegisters* capRegister = (HostControllerCapabilityRegisters*)(memory::bar0.data());
		HostControllerOperationalRegisters* opRegister = (HostControllerOperationalRegisters*)(memory::bar0.data() + capRegister->CAPLENGTH);
		HostControllerRuntimeRegisters* rtRegister = (HostControllerRuntimeRegisters*)(memory::bar0.data() + capRegister->RTSOFF);
		//DoorbellRegisters* dbRegister = (DoorbellRegisters*)(memory::bar0.data() + capRegister->DBOFF);
		uint64_t write_address = 0;

		int ERSTE_count = rtRegister->InterrupterRegisters[0].ERSTSZ.Event_Ring_Segment_Table_Size;
		std::vector<Event_Ring_Segment_Table_Entry> ERSTEs(ERSTE_count);
		if (!LcRead(memory::hLC, rtRegister->InterrupterRegisters[0].ERSTBA.Event_Ring_Segment_Table_Base_Address_Register << 6, sizeof(Event_Ring_Segment_Table_Entry) * ERSTE_count, (PBYTE)ERSTEs.data())) {
			printf("Failed to read EventRingSegmentTableEntries!\n");
			return;
		}
		for (int i = 0; i < ERSTEs.size(); i++) {
			uint64_t base_addr = ERSTEs[i].Ring_Segment_Base_Address << 6;
			printf("Entry 0x%X, Size: %d\n", base_addr, ERSTEs[i].Ring_Segment_Size);
			void* buffer = malloc(0x10 * ERSTEs[i].Ring_Segment_Size);
			if (!buffer) {
				printf("failed to allocate buffer!\n");
				return;
			}
			if (LcRead(memory::hLC, base_addr, 0x10 * ERSTEs[i].Ring_Segment_Size, (PBYTE)buffer)) {
				for (int j = 0; j < ERSTEs[i].Ring_Segment_Size; j++) {
					if (((*(uint32_t*)((uint8_t*)buffer + 0x10 * j + 0xC)) & 0x1) == memory::Producer_Cycle_State[4]) {
						printf("Writable EventRing pos: 0x%s\n", utils::toHexString2(base_addr + 0x10 * j).c_str());
						write_address = base_addr + 0x10 * j;
						break;
					}
				}
			}
			else {
				printf("Failed to read Ring Segment!\n");
				return;
			}
			free(buffer);
			if (write_address)
				break;
		}
		if (write_address) {
			if (!LcWrite(memory::hLC, write_address, trb_size, trb)) {
				printf("Failed to Write TRB to Event Ring!\n");
				return;
			}
			printf("Event Interrupt Status: %d\n", opRegister->USBSTS.Event_Interrupt);
			uint64_t msix_addr = *(uint64_t*)(memory::bar2.data() + 0x10 * 0);
			uint32_t msix_data = *(uint32_t*)(memory::bar2.data() + 0x10 * 0 + 0x8);
			printf("MSIX addr: 0x%s, data: %s\n", utils::toHexString2(msix_addr).c_str(), utils::toHexString2(msix_data).c_str());
			if (!LcWrite(memory::hLC, msix_addr, 4, (PBYTE)&msix_data)) {
				printf("Failed to Write MSI-X!\n");
				return;
			}
			printf("SENT MSI-X Intrrupt!\n");
		}
	}*/

	void fuck_men() {
		HostControllerCapabilityRegisters* capRegister = (HostControllerCapabilityRegisters*)(bar0.data());
		HostControllerOperationalRegisters* opRegister = (HostControllerOperationalRegisters*)(bar0.data() + capRegister->CAPLENGTH);
		HostControllerRuntimeRegisters* rtRegister = (HostControllerRuntimeRegisters*)(bar0.data() + capRegister->RTSOFF);

		TRB_Port_Status_Change_Event TRB = {};
		TRB.Cycle_Bit = 1;
		TRB.Port_ID = 1;
		TRB.TRB_Type = 0x22;
		TRB.Completion_Code = 1;
		opRegister->PortRegisters[0].PORTSC.Port_Enable_Disable = 1;
		opRegister->PortRegisters[0].PORTSC.Port_Enabled_Disabled_Change = 1;
		//opRegister->USBSTS.Event_Interrupt = 1;
		opRegister->PortRegisters[0].PORTSC.Connect_Status_Change = 1;
		opRegister->PortRegisters[0].PORTSC.Current_Connect_Status = 1;
		opRegister->PortRegisters[0].PORTSC.Port_Link_State = 7;
		opRegister->PortRegisters[0].PORTSC.Port_Speed = 1;
		opRegister->PortRegisters[0].PORTSC.Wake_on_Connect_Enable = 0;
		opRegister->PortRegisters[0].PORTSC.Wake_on_Over_current_Enable = 0;
		xhci::send_event_ring(0, (PBYTE)&TRB);
	}

	bool done = false;

	void cb_bar_0_v2(_Inout_ PLC_BAR_REQUEST pReq, BARLogEntry* entry) {
		HostControllerCapabilityRegisters* capRegister = (HostControllerCapabilityRegisters*)(bar0.data());
		HostControllerOperationalRegisters* opRegister = (HostControllerOperationalRegisters*)(bar0.data() + capRegister->CAPLENGTH);
		HostControllerRuntimeRegisters* rtRegister = (HostControllerRuntimeRegisters*)(bar0.data() + capRegister->RTSOFF);
		DoorbellRegisters* dbRegister = (DoorbellRegisters*)(bar0.data() + capRegister->DBOFF);
		DWORD offset = pReq->oData;
		bool handled = false;
		if (pReq->fRead) {
			for (int i = 0; i < pReq->cbData; i++) {
				pReq->pbData[i] = bar0[offset + i];
			}
			pReq->fReadReply = true;
		}
		if (pReq->fWrite && pReq->cbData == 4) {
			uint32_t* p_old_value = (uint32_t*)(bar0.data() + offset);
			uint32_t old_value = *p_old_value;
			uint32_t new_value = *(uint32_t*)pReq->pbData;

			if (offset == 0x80) { //USB Command
				//RUN/STOP 0 => 1
				//if (!get_bit(old_value, 0) && get_bit(new_value, 0)) {
				if (get_bit_change(old_value, new_value, 0) == 1) {
					xhci::xhci_run();
				}
				//RUN/STOP 1 => 0
				//else if (get_bit(old_value, 0) && !get_bit(new_value, 0)) {
				if (get_bit_change(old_value, new_value, 0) == -1) {
					xhci::xhci_stop();
				}
				//Host Controller Reset 0 => 1
				//if (!get_bit(old_value, 1) && get_bit(new_value, 1)) {
				if (get_bit_change(old_value, new_value, 1) == 1) {
					xhci::xhci_reset();
				}

				copy_bits(p_old_value, new_value, 2, 3); //RW
				copy_bits(p_old_value, new_value, 7, 7); //RW
				copy_bits(p_old_value, new_value, 8, 11); //RW
				copy_bits(p_old_value, new_value, 13, 15); //RW
				copy_bits(p_old_value, new_value, 16, 16); //RW
			}
			else if (offset == 0x84) { //USB Status
				rw1c(p_old_value, new_value, 2);
				rw1c(p_old_value, new_value, 3);
				rw1c(p_old_value, new_value, 4);
				rw1c(p_old_value, new_value, 10);
			}
			else if (offset == 0x94) { //Device Notification Control Register
				copy_bits(p_old_value, new_value, 0, 15);
			}
			else if (offset == 0x98) {
				copy_bits(p_old_value, new_value, 0, 0);
				rw1s(p_old_value, new_value, 1);
				rw1s(p_old_value, new_value, 2);
				copy_bits(p_old_value, new_value, 6, 31);
			}
			else if (offset == 0x9C) {
				copy_bits(p_old_value, new_value, 0, 31);
			}
			else if (offset == 0xB0) { //Device Context Base Address Register LO
				copy_bits(p_old_value, new_value, 6, 31);
			}
			else if (offset == 0xB4) { //Device Context Base Address Register HI
				copy_bits(p_old_value, new_value, 0, 31);
			}
			else if (0x480 <= offset && offset < 0x500) {
				int index = (offset - 0x480) / 4;
				int port_index = ((offset - 0x480) >> 4) & 0b1111;
				int data_index = index & 0b11;
				if (data_index == 0) { //PORTSC
					rw1c(p_old_value, new_value, 1);
					if (rw1s(p_old_value, new_value, 4)) {
						xhci::portsc_reset(port_index);
						printf("offset: %X\n", offset - 0x480);
					}
					copy_bits(p_old_value, new_value, 5, 8);
					copy_bits(p_old_value, new_value, 9, 9);
					copy_bits(p_old_value, new_value, 14, 16);
					rw1c(p_old_value, new_value, 17);
					rw1c(p_old_value, new_value, 18);
					rw1c(p_old_value, new_value, 19);
					rw1c(p_old_value, new_value, 20);
					rw1c(p_old_value, new_value, 21);
					rw1c(p_old_value, new_value, 22);
					rw1c(p_old_value, new_value, 23);
					copy_bits(p_old_value, new_value, 25, 25);
					copy_bits(p_old_value, new_value, 26, 26);
					copy_bits(p_old_value, new_value, 27, 27);
					rw1s(p_old_value, new_value, 31);
				}
				else if (data_index == 1) {
					if (index < 16) {//USB2
						copy_bits(p_old_value, new_value, 3, 3);
						copy_bits(p_old_value, new_value, 4, 7);
						copy_bits(p_old_value, new_value, 8, 15);
						copy_bits(p_old_value, new_value, 16, 16);
						copy_bits(p_old_value, new_value, 28, 31);
					}
					else {//USB3
						copy_bits(p_old_value, new_value, 0, 7);
						copy_bits(p_old_value, new_value, 8, 15);
						copy_bits(p_old_value, new_value, 16, 16);
					}
				}
				else if (data_index == 2) {
					copy_bits(p_old_value, new_value, 0, 15);
				}
				else if (data_index == 3) {
					copy_bits(p_old_value, new_value, 0, 1);
					copy_bits(p_old_value, new_value, 2, 9);
					copy_bits(p_old_value, new_value, 10, 13);
				}
			}
			else if (0x2020 <= offset && offset < 0x2120) {
				int index = (offset - 0x2020) / 4;
				int data_index = index & 0b111;
				if (data_index == 0) {
					rw1c(p_old_value, new_value, 0);
					if ((*p_old_value >> 1 & 0x1) == 0 && ((new_value >> 1) & 0x1) == 1) {
						xhci::interrupter_enable((offset - 0x2020) / 0x20);
					}
					copy_bits(p_old_value, new_value, 1, 1);

				}
				else if (data_index == 1) {
					copy_bits(p_old_value, new_value, 0, 15);
					copy_bits(p_old_value, new_value, 16, 31);
				}
				else if (data_index == 2) {
					copy_bits(p_old_value, new_value, 0, 15);
				}
				else if (data_index == 3) {}
				else if (data_index == 4) {
					copy_bits(p_old_value, new_value, 6, 31);
				}
				else if (data_index == 5) {
					copy_bits(p_old_value, new_value, 0, 31);
				}
				else if (data_index == 6) {
					copy_bits(p_old_value, new_value, 6, 31);
				}
				else if (data_index == 7) {
					copy_bits(p_old_value, new_value, 0, 31);
				}
			}
			else if (offset == 0x3000) {
				xhci::consume_command_ring(entry);
			}

			else if (0x3004 <= offset && offset < 0x3400) {
				uint8_t ep_id = new_value & 0xFF;
				uint16_t stream_id = (new_value >> 16) && 0xFFFF;
				xhci::handle_transfer_ring(offset - 0x3000, ep_id, stream_id);
			}
			else if (0x8000 <= offset && offset < 0x8380) {
				for (int i = 0; i < pReq->cbData; i++) {
					bar0[offset + i] = pReq->pbData[i];
				}
			}
		}
		if (pReq->fWrite && pReq->cbData == 8) {
			uint64_t* p_old_value = (uint64_t*)(bar0.data() + offset);
			uint64_t old_value = *p_old_value;
			uint64_t new_value = *(uint64_t*)pReq->pbData;

			if (offset == 0x98) {
				copy_bits(p_old_value, new_value, 0, 0);
				rw1s(p_old_value, new_value, 1);
				rw1s(p_old_value, new_value, 2);
				copy_bits(p_old_value, new_value, 6, 63);
				handled = true;
			}
			else if (offset == 0xB0) {
				copy_bits(p_old_value, new_value, 6, 63);
				handled = true;
			}
			else if (0x2020 <= offset && offset < 0x2120) {
				int index = (offset - 0x2020) / 4;
				int data_index = index & 0b111;
				if (data_index == 4) {
					copy_bits(p_old_value, new_value, 6, 63);
					handled = true;
				}
				else if (data_index == 6) {
					copy_bits(p_old_value, new_value, 6, 63);
					handled = true;
				}
			}
		}
		if (pReq->cbData != 4 && !handled) {
			if (pReq->fWrite)
				printf("[WR] Non 4bytes Access to 0x%llX, Length: %d\n", pReq->oData, pReq->cbData);
			else
				printf("[RD] Non 4bytes Access to 0x%llX, Length: %d\n", pReq->oData, pReq->cbData);
		}
	}

	/*
	void cb_bar0(_Inout_ PLC_BAR_REQUEST pReq, BARLogEntry *entry) {
		HostControllerCapabilityRegisters* capRegister = (HostControllerCapabilityRegisters*)(bar0.data());
		HostControllerOperationalRegisters* opRegister = (HostControllerOperationalRegisters*)(bar0.data() + capRegister->CAPLENGTH);
		HostControllerRuntimeRegisters* rtRegister = (HostControllerRuntimeRegisters*)(bar0.data() + capRegister->RTSOFF);
		DoorbellRegisters* dbRegister = (DoorbellRegisters*)(bar0.data() + capRegister->DBOFF);
		DWORD offset = pReq->oData;
		if (pReq->fRead) {
			for (int i = 0; i < pReq->cbData; i++) {
				pReq->pbData[i] = bar0[offset + i];
			}
			pReq->fReadReply = true;
		}
		if (pReq->fWrite) {
			uint32_t old_value = *(uint32_t*)(bar0.data() + offset);
			uint32_t new_value = *(uint32_t*)(pReq->pbData);

			for (int i = 0; i < pReq->cbData; i++) {
				bar0[offset + i] = pReq->pbData[i];
			}
			
			uint32_t changed = old_value ^ new_value;
			if (offset == 0x3000) {
				DoorbellRegister db = (DoorbellRegister)new_value;
				printf("OH MEN DOORBELL : %s\n", utils::toHexString(pReq->pbData, pReq->cbData).c_str());
				entry->descs.push_back("OH MEN DOORBELL!");
				xhci::consume_command_ring(entry);
			}
			if (offset == 0x80) {//USBCommand
				HostControllerOperationalRegisters_USBCMD old_reg = *reinterpret_cast<HostControllerOperationalRegisters_USBCMD*>(&old_value);
				HostControllerOperationalRegisters_USBCMD new_reg = *reinterpret_cast<HostControllerOperationalRegisters_USBCMD*>(&new_value);
				if (!old_reg.RunStop && new_reg.RunStop) {
					xhci::xhci_run();
				}
				else if (old_reg.RunStop && !new_reg.RunStop) {
					xhci::xhci_stop();
				}
				if (new_reg.Controller_Save_State) {
					opRegister->USBSTS.SaveRestore_Error = 0;
				}
				if (new_reg.Controller_Restore_State) {
					opRegister->USBSTS.SaveRestore_Error = 1;
				}
				printf("WRITE USBCMD 0x%X => 0x%X %d 0x%X\n", old_value, new_value, new_reg.Host_Controller_Reset, *(uint32_t*)&new_reg);
				if (new_reg.Host_Controller_Reset) {
					entry->descs.push_back("HOST CONTROLLER RESET");
					printf("HOST CONTROLLER RESET !\n");
					xhci::xhci_reset();
				}

				//opRegister->USBCMD_RAW = new_value & 0xC0F;

				if (changed & (1 << 2)) {//Interrupter Enable
					int pos = 0;
					if (old_value & (1 << 2)) {
						entry->descs.push_back("USB COMMAND Interupter Enable 1 => 0");
						
					}
					else {
						entry->descs.push_back("USB COMMAND Interupter Enable 0 => 1");
						if (!done && opRegister->USBSTS.HCHalted == 0) {
							//printf("SENDING Port Status Change Event! From Interrupter Enable\n");
							//fuck_men();
							//done = true;
						}
					}
				}
			}
			else if (capRegister->CAPLENGTH + 0x400 <= offset && offset < capRegister->CAPLENGTH + 0x13FF) { //OperationalRegister->PortRegisterSet[index]
				uint32_t offset_in_ports = offset - capRegister->CAPLENGTH - 0x400;
				uint32_t field_offset = offset_in_ports % 0x10;
				uint32_t index = (offset_in_ports - field_offset) / 0x10;
				if (field_offset == 0x0) {//PORTSC
					if ((changed & (1 << 4)) && (new_value & (1 << 4))) {//Port Reset 0 => 1
						printf("Port Reset Index: %d Value: 0x%X\n", index, new_value);
						opRegister->PortRegisters[index].PORTSC.Port_Reset = 0;

						opRegister->PortRegisters[index].PORTSC.Port_Enable_Disable = 1;
						opRegister->PortRegisters[index].PORTSC.Port_Link_State = 0;
						opRegister->PortRegisters[index].PORTSC.Port_Speed = 1;
						opRegister->PortRegisters[index].PORTSC.Port_Reset_Change = 1;
						xhci::send_port_status_change_event();
					}
					else if (new_value & (1 << 17)) {//Connect Status Change = 1
						printf("WRITE Connect Status Change Index(NEED RESET): %d Value: %d\n", index, (new_value >> 17) & 0x1);
						opRegister->PortRegisters[index].PORTSC.Connect_Status_Change = 0;

					}
					else if ((changed & (1 << 18)) && (new_value & (1 << 18))) {//Port Enabled/Disabled Change 0 => 1
						printf("Port Enabled/Disabled Change Index: %d Value: 0x%X\n", index, new_value);
						
					}
					else if ((changed & (1 << 21)) && (new_value & (1 <<21))) {//PORT RESET Change 0 => 1
						printf("Port Reset Change Index: %d Value: 0x%X\n", index, new_value);
						opRegister->PortRegisters[index].PORTSC.Port_Reset_Change = 0;
					}
					else if ((changed & (1 << 25)) && (new_value & (1 << 25))) {//Wake on Connect Enable Change 0 => 1
						printf("Wake on Connect Enable Index: %d Value: 0x%X\n", index, new_value);
					}
					else if ((changed & (1 << 26)) && (new_value & (1 << 26))) {//Wake on Disconnect Enable Change 0 => 1
						printf("Wake on Disconnect Enable Index: %d Value: 0x%X\n", index, new_value);
					}
				
					else if ((changed & (1 << 27)) && (new_value & (1 << 27))) {//Wake on Over-current Enable Change 0 => 1
						printf("Wake on Over-current Enable Index: %d Value: 0x%X\n", index, new_value);
					}
				}
			}
		}
	}
	*/

	void cb_bar2(_Inout_ PLC_BAR_REQUEST pReq) {
		if (pReq->fRead) {
			for (int i = 0; i < pReq->cbData; i++) {
				pReq->pbData[i] = bar2[pReq->oData + i];
			}
			pReq->fReadReply = true;
		}
		if (pReq->fWrite) {
			for (int i = 0; i < pReq->cbData; i++) {
				bar2[pReq->oData + i] = pReq->pbData[i];
			}
		}
	}

	void cb_bar4(_Inout_ PLC_BAR_REQUEST pReq) {
		if (pReq->fRead) {
			for (int i = 0; i < pReq->cbData; i++) {
				pReq->pbData[i] = bar4[pReq->oData + i];
			}
			pReq->fReadReply = true;
		}
		if (pReq->fWrite) {
			for (int i = 0; i < pReq->cbData; i++) {
				bar4[pReq->oData + i] = pReq->pbData[i];
			}
		}
	}
}