#include "utils.hpp"
#include "memory.hpp"
#include "structs.hpp"
#include <sstream>
#include <format>

namespace utils {

	std::string toHexString(unsigned char* ptr, size_t length) {
		std::stringstream ss;

		for (size_t i = 0; i < length; ++i) {
			ss << std::setw(2) << std::setfill('0') << std::hex << (int)ptr[i];
		}

		return ss.str();
	}

	std::string toHexString2(uint64_t value, int fill) {
		std::stringstream ss;
		if (fill == 0) {
			ss << std::hex << std::uppercase << std::setfill('0') << value;
		}
		else {
			ss << std::hex << std::setw(fill) << std::uppercase << std::setfill('0') << value;
		}
		return ss.str();
	}

	std::vector<int> changedBits(uint32_t ov, uint32_t nv) {
		std::vector<int> changed;
		uint32_t changedBits = ov ^ nv;
		for (int i = 0; i < 32; i++)
			if (changedBits & (1 << i))
				changed.push_back(i);
		return changed;
	}

	std::string data_name(uint32_t bar, uint32_t offset) {
		std::stringstream ss;
		ss << "0x" << std::hex << std::setfill('0') << offset;
		std::string name = ss.str();
		if (bar == 0) {
			switch (offset) {
			case 0x0:
				name = "CapRegister->CAPLENGTH";
				break;
			case 0x4:
				name = "CapRegister->HCSPARAMS1";
				break;
			case 0x8:
				name = "CapRegister->HCSPARAMS2";
				break;
			case 0xC:
				name = "CapRegister->HCSPARAMS3";
				break;
			case 0x10:
				name = "CapRegister->HCCPARAMS1";
				break;
			case 0x14:
				name = "CapRegister->DBOFF";
				break;
			case 0x18:
				name = "CapRegister->RTSOFF";
				break;
			case 0x1C:
				name = "CapRegister->HCCPARAMS2";
				break;
			}
			HostControllerCapabilityRegisters* capRegister = (HostControllerCapabilityRegisters*)memory::bar0.data();
			if (capRegister->CAPLENGTH && capRegister->CAPLENGTH <= offset && offset < capRegister->CAPLENGTH + 0x13FF) {
				name = "OpeRegister: " + name;
				uint32_t opOffset = offset - capRegister->CAPLENGTH;
				switch (opOffset) {
				case 0x0:
					name = "OpeRegister->USB Command";
					break;
				case 0x4:
					name = "OpeRegister->USB Status";
					break;
				case 0x8:
					name = "OpeRegister->Page Size";
					break;
				case 0x14:
					name = "OpeRegister->Device Notification Control";
					break;
				case 0x18:
					name = "OpeRegister->Command Ring Control LO";
					break;
				case 0x1C:
					name = "OpeRegister->Command Ring Control HI";
					break;
				case 0x30:
					name = "OpeRegister->Device Context Base Address Array Pointer";
					break;
				case 0x38:
					name = "OpeRegister->Configure";
					break;
				}
				if (0x400 <= opOffset && opOffset < 0x13FF) {
					uint32_t arOffset = opOffset - 0x400;
					uint32_t data = arOffset % 0x10;
					uint32_t index = (arOffset - data) / 0x10;
					name = std::format("OpeRegister->PortRegisters[{:d}]->", index);
					switch (data) {
					case 0x0:
						name = name + "PORTSC";
						break;
					case 0x4:
						name = name + "PORTPMSC";
						break;
					case 0x8:
						name = name + "PORTLI";
						break;
					case 0xC:
						name = name + "PORTHLPMC";
						break;
					default:
						name = name + std::format("0x{:X}", data);
						break;
					}
				}
			}
			if (capRegister->RTSOFF && capRegister->RTSOFF <= offset && offset < capRegister->RTSOFF + sizeof(HostControllerRuntimeRegisters)) {
				name = "OpeRegister: " + name;
				uint32_t rtOffset = offset - capRegister->RTSOFF;
				if (rtOffset == 0) {
					name = "RtRegister->Microframe Index";
				}
				if (0x20 <= rtOffset && rtOffset <= sizeof(HostControllerRuntimeRegisters)) {
					uint32_t arOffset = rtOffset - 0x20;
					uint32_t data = arOffset % 0x20;
					uint32_t index = (arOffset - data) / 0x20;
					name = std::format("RtRegister->InterrupterRegisters[{:d}]->", index);
					switch (data) {
					case 0x0:
						name = name + "Management";
						break;
					case 0x4:
						name = name + "Moderation";
						break;
					case 0x8:
						name = name + "Event Ring Segment Table Size";
						break;
					case 0x10:
						name = name + "Event Ring Segment Table Base Address";
						break;
					case 0x18:
						name = name + "Event Ring Dequeue Pointer";
						break;
					default:
						name = name + std::format("0x{:X}", data);
						break;
					}
				}
			}
		}
		return name;
	}

	int get_port_type(int port) {
		HostControllerCapabilityRegisters* cReg = (HostControllerCapabilityRegisters*)memory::bar0.data();
		eXtendedCapability* eCap = (eXtendedCapability*)(memory::bar0.data() + cReg->xHCIExtendedCapabilitiesPointer());

		while (true) {
			if (eCap->Capability_ID == 2) {
				eXtendedCapability_Supported_Protocol* xCap = (eXtendedCapability_Supported_Protocol*)eCap;
				if ((xCap->Compatible_Port_Offset - 1) <= port && port <= (xCap->Compatible_Port_Offset - 2 + xCap->Compatible_Port_Count)) {
					return xCap->Revision_Major;
				}
			}
			uint64_t next = eCap->Next_Pointer;
			if (next == 0)
				break;
			next <<= 2;
			uint8_t* next_ptr = (uint8_t*)eCap + next;
			eCap = (eXtendedCapability*)(next_ptr);
		}
		return 0;
	}

	std::string get_trb_type_name(int type) {
		std::string name = "Unknown";
		switch (type) {
			case 2:
				name = "Setup Stage";
				break;
			case 3:
				name = "Data Stage";
				break;
			case 4:
				name = "Status Stage";
				break;
			case 6:
				name = "Link";
				break;
			case 7:
				name = "Event Data";
				break;
			case 9:
				name = "Enable Slot Command";
				break;
			case 10:
				name = "Disable Slot Command";
				break;
			case 11:
				name = "Address Device Command";
				break;
			case 12:
				name = "Configure Endpoint Command";
				break;
			case 13:
				name = "Evaluate Context Command";
				break;
			case 14:
				name = "Reset Endpoint Command";
				break;
			case 15:
				name = "Stop Endpoint Comand";
				break;
			case 16:
				name = "Set TR Dequeue Pointer Command";
				break;
			case 17:
				name = "Reset Device Command";
				break;
			case 32:
				name = "Transfer Event";
				break;
			case 33:
				name = "Command Completion Event";
				break;
			case 34:
				name = "Port Status Change Event";
				break;
		}
		return name;
	}
}