#pragma once
#include <stdint.h>
#include <vector>
#include <string>

struct HostControllerCapabilityRegisters {
	uint8_t CAPLENGTH;
	uint16_t HCIVERSION;
	uint32_t HCSPARAMS1;
	uint32_t HCSPARAMS2;
	uint32_t HCSPARAMS3;
	uint32_t HCCPARAMS1;
	uint32_t DBOFF;
	uint32_t RTSOFF;
	uint32_t HCCPARAMS2;

	uint8_t OperationalRegisterSpaceOffset() {
		return this->CAPLENGTH;
	}

	uint16_t HostControllerInterfaceVersionNumber() {
		return this->HCIVERSION;
	}

	uint8_t MaxDeviceSlots() {
		return this->HCSPARAMS1 & 0xFF;
	}

	uint16_t MaxInterrupters() {
		return (this->HCSPARAMS1 >> 8) & 0x7FF;
	}

	uint8_t MaxPorts() {
		return (this->HCSPARAMS1 >> 25) & 0xFF;
	}

	bool IST_BIT3() {
		return (this->HCSPARAMS2 >> 2) & 0x1;
	}

	uint8_t IST_Frame() {
		return this->HCSPARAMS2 & 0x3;
	}

	uint32_t EventRingSegmentTableMax() {
		return 1 << ((this->HCSPARAMS2 >> 4) & 0xF);
	}

	uint16_t MaxScratchPadBuffers() {
		uint16_t max_scratchpad_bufs_hi = (this->HCSPARAMS2 >> 21) & 0x1F;
		uint16_t max_scratchpad_bufs_lo = (this->HCSPARAMS2 >> 27) & 0x1F;
		return (max_scratchpad_bufs_hi << 5) | max_scratchpad_bufs_lo;
	}

	bool ScratchPadRestore() {
		return (this->HCSPARAMS2 >> 26) & 0x1;
	}

	uint8_t U1DeviceExitLatency() {
		return this->HCSPARAMS3 & 0xFF;
	}

	uint8_t U2DeviceExitLatency() {
		return (this->HCSPARAMS3 >> 16) & 0xFF;
	}

	bool Addressing64Bit() {
		return this->HCCPARAMS1 & 0x1;
	}

	bool BWNegotiation() {
		return (this->HCCPARAMS1 >> 1) & 0x1;
	}

	bool ContextSize() {
		return (this->HCCPARAMS1 >> 2) & 0x1;
	}

	bool PortPowerControl() {
		return (this->HCCPARAMS1 >> 3) & 0x1;
	}

	bool PortIndicators() {
		return (this->HCCPARAMS1 >> 4) & 0x1;
	}

	bool LightHCReset() {
		return (this->HCCPARAMS1 >> 5) & 0x1;
	}

	bool LatencyToleranceMessaging() {
		return (this->HCCPARAMS1 >> 6) & 0x1;
	}

	bool NoSecnodarySIDSupport() {
		return (this->HCCPARAMS1 >> 7) & 0x1;
	}

	bool ParseAllEventData() {
		return (this->HCCPARAMS1 >> 8) & 0x1;
	}

	bool StoppedShortPacket() {
		return (this->HCCPARAMS1 >> 9) & 0x1;
	}

	bool StoppedEDTLA() {
		return (this->HCCPARAMS1 >> 10) & 0x1;
	}

	bool ContiguousFrameID() {
		return (this->HCCPARAMS1 >> 11) & 0x1;
	}

	uint8_t MaximumPrimaryStreamArraySize() {
		return (this->HCCPARAMS1 >> 12) & 0xFF;
	}

	uint16_t xHCIExtendedCapabilitiesPointer() {
		return ((this->HCCPARAMS1 >> 16) & 0xFFFF) << 2;
	}

	bool U3Entry() {
		return this->HCCPARAMS2 & 0x1;
	}

	bool EpCMD_MaxExitLatencyTooLarge() {
		return (this->HCCPARAMS2 >> 1) & 0x1;
	}

	bool ForceSaveContext() {
		return (this->HCCPARAMS2 >> 2) & 0x1;
	}

	bool ComplianceTransition() {
		return (this->HCCPARAMS2 >> 3) & 0x1;
	}

	bool LargeESITPayload() {
		return (this->HCCPARAMS2 >> 4) & 0x1;
	}

	bool ConfigurationInformation() {
		return (this->HCCPARAMS2 >> 5) & 0x1;
	}

	bool ExtendedTBC() {
		return (this->HCCPARAMS2 >> 6) & 0x1;
	}

	bool ExtendedTBC_TRB_Status() {
		return (this->HCCPARAMS2 >> 7) & 0x1;
	}

	bool GetSet_Extended_Proterty() {
		return (this->HCCPARAMS2 >> 8) & 0x1;
	}

	bool VirtualizationBasedTrustedIO() {
		return (this->HCCPARAMS2 >> 9) & 0x1;
	}
};

struct PortRegisterSet_PORTSC {
	uint32_t Current_Connect_Status : 1;
	uint32_t Port_Enable_Disable : 1;
	uint32_t PORTSC_RsvdZ_0 : 1;
	uint32_t Over_Current_Active : 1;
	uint32_t Port_Reset : 1;
	uint32_t Port_Link_State : 4;
	uint32_t Port_Power : 1;
	uint32_t Port_Speed : 4;
	uint32_t Port_Indicator_Control : 2;

	uint32_t Port_Link_State_Write_Strobe : 1;
	uint32_t Connect_Status_Change : 1;
	uint32_t Port_Enabled_Disabled_Change : 1;
	uint32_t Warm_Port_Reset_Change : 1;
	uint32_t Over_Current_Change : 1;
	uint32_t Port_Reset_Change : 1;
	uint32_t Port_Link_State_Change : 1;
	uint32_t Port_Config_Error_Change : 1;

	uint32_t Cold_Attach_Status : 1;
	uint32_t Wake_on_Connect_Enable : 1;
	uint32_t Wake_on_Disconnect_Enable : 1;
	uint32_t Wake_on_Over_current_Enable : 1;
	uint32_t PORTSC_RsvdZ_1 : 2;
	uint32_t Device_Removable : 1;
	uint32_t Warm_Port_Reset : 1;
};

struct PortRegisterSet_PORTPMSC_USB3 {
	uint32_t U1_Timeout : 8;
	uint32_t U2_Timeout : 8;
	uint32_t Force_Link_PM_Accept : 1;
	uint32_t RsvdP_0 : 15;
};

struct PortRegisterSet_PORTPMSC_USB2 {
	uint32_t L1_Status : 3;
	uint32_t Remote_Wake_Enable : 1;
	uint32_t Best_Effort_Service_Latency : 4;
	uint32_t L1_Device_Slot : 8;
	uint32_t Hardware_LPM_Enable : 1;
	uint32_t RsvdP_0 : 11;
};

struct PortRegisterSet_PORTLI_USB3 {
	uint32_t Link_Error_Count : 16;
	uint32_t Rx_Lane_Count : 4;
	uint32_t Tx_Lane_Count : 4;
	uint32_t RsvdP_0 : 8;
};

struct PortRegisterSet_PORTLI_USB2 {
	uint32_t RsvdP_0;
};

struct PortRegisterSet_PORTHLPMC_USB3 {
	uint32_t Link_Soft_Error_Count : 16;
	uint32_t RsvdP_0 : 16;
};

struct PortRegisterSet_PORTHLPMC_USB2 {
	uint32_t Host_Initiated_Resume_Duration_Mode : 2;
	uint32_t L1_Timeout : 8;
	uint32_t Best_Effort_Service_Latency_Deep : 4;
	uint32_t RsvdP_0 : 8;
};

struct PortRegisterSet {
	PortRegisterSet_PORTSC PORTSC;

	union {
		PortRegisterSet_PORTPMSC_USB3 PORTPMSC_USB3;
		PortRegisterSet_PORTPMSC_USB2 PORTPMSC_USB2;
		uint32_t PORTPMSC;
	};
	union {
		PortRegisterSet_PORTLI_USB3 PORTLI_USB3;
		PortRegisterSet_PORTLI_USB2 PORTLI_USB2;
		uint32_t PORTLI;
	};
	union {
		PortRegisterSet_PORTHLPMC_USB3 PORTHLPMC_USB3;
		PortRegisterSet_PORTHLPMC_USB2 PORTHLPMC_USB2;
		uint32_t PORTHLPMC;
	};
};

struct HostControllerOperationalRegisters_USBCMD {
	uint32_t RunStop : 1;
	uint32_t Host_Controller_Reset : 1;
	uint32_t Interrupter_Enable : 1;
	uint32_t Host_System_Error_Enable : 1;
	uint32_t RsvdP_0 : 3;
	uint32_t Light_Host_Controller_Reset : 1;

	uint32_t Controller_Save_State : 1;
	uint32_t Controller_Restore_State : 1;
	uint32_t Enable_Wrap_Event : 1;
	uint32_t Enable_U3_MFINDEX_Stop : 1;
	uint32_t RsvdP_1 : 1;
	uint32_t CEM_Enable : 1;
	uint32_t Extended_TBC_Enable : 1;
	uint32_t Extended_TBC_TRB_Status_Enable : 1;

	uint32_t VTIO_Enable : 1;
	uint32_t RsvdP_2 : 15;
};

struct HostControllerOperationalRegisters_USBSTS {
	uint32_t HCHalted : 1;
	uint32_t RsvdZ_0 : 1;
	uint32_t Host_System_Error : 1;
	uint32_t Event_Interrupt : 1;
	uint32_t Port_Change_Detect : 1;
	uint32_t RsvdZ_1 : 3;

	uint32_t Save_State_Status : 1;
	uint32_t Restore_State_Status : 1;
	uint32_t SaveRestore_Error : 1;
	uint32_t Controller_Not_Ready : 1;
	uint32_t Host_Controller_Error : 1;
	uint32_t RsvdZ_2 : 19;
};

struct HostControllerOperationalRegisters_CRCR {
	uint64_t Ring_Cycle_State : 1;
	uint64_t Command_Stop : 1;
	uint64_t Command_Abort : 1;
	uint64_t Command_Ring_Running : 1;
	uint64_t RsvdP_0 : 2;
	uint64_t Command_Ring_Pointer : 58;
};

struct HostControllerOperationalRegisters_CONFIG {
	uint32_t Max_Device_Slots_Enabled : 8;
	uint32_t U3_Entry_Enable : 1;
	uint32_t Configuration_Information_Enable : 1;
	uint32_t RsvdP_0 : 22;
};

#pragma pack(push, 1)
struct HostControllerOperationalRegisters {
	union {
		HostControllerOperationalRegisters_USBCMD USBCMD;
		uint32_t USBCMD_RAW;
	};
	union {
		HostControllerOperationalRegisters_USBSTS USBSTS;
		uint32_t USBSTS_RAW;
	};
	uint32_t PAGESIZE;
	char RsvdZ_1[0x8];
	uint32_t DNCTRL;
	HostControllerOperationalRegisters_CRCR CRCR;
	char RsvdZ_2[0x10];
	uint64_t DCBAAP;
	HostControllerOperationalRegisters_CONFIG CONFIG;
	char RsvdZ_3[0x3C4];
	PortRegisterSet PortRegisters[0x100];

	uint64_t PageSize() {
		uint32_t page_size = PAGESIZE;
		uint64_t position = 0;
		while (page_size) {
			if (page_size & 1) {
				break;
			}
			page_size >>= 1;
			position++;
		}
		return static_cast<uint64_t>(1) << (position + 12);
	}

	bool DNCTRL_Notification_Enable(int bit) {
		return (this->DNCTRL >> bit) & 0x1;
	}

	uint64_t DCBAAP_Pointer() {
		return this->DCBAAP & 0xFFFFFFFFFFFFFFC0;
	}
};
#pragma pack(pop)

struct InterrupterRegisterSet_IMAN {
	uint32_t Interrupt_Pending : 1;
	uint32_t Interrupt_Enable : 1;
	uint32_t RsvdP_0 : 30;
};

struct InterrupterRegisterSet_IMOD {
	uint32_t Interrupt_Moderation_Interval : 16;
	uint32_t Interrupt_Moderation_Count : 16;
};

struct InterruptRegisterSet_ERSTSZ {
	uint32_t Event_Ring_Segment_Table_Size : 16;
	uint32_t RsvdP_0 : 16;
};

struct InterruptRegisterSet_ERSTBA {
	uint64_t RsvdP_0 : 6;
	uint64_t Event_Ring_Segment_Table_Base_Address_Register : 58;
};

struct InterruptRegisterSet_ERDP {
	uint64_t Dequeue_ERST_Segment_Index : 3;
	uint64_t Event_Handler_Busy : 1;
	uint64_t Event_Ring_Dequeue_Pointer : 60;
};

struct InterrupterRegisterSet {
	//Interrupter Management Register : 32bit
	InterrupterRegisterSet_IMAN IMAN;

	//Interrupter Moderation Register : 32bit
	InterrupterRegisterSet_IMOD IMOD;
	
	//Event Ring Segment Table Size Register : 32bit
	InterruptRegisterSet_ERSTSZ ERSTSZ;
	
	uint32_t RsvdP_0;

	//Event Ring Segment Table Base Address Register Bit Definitions : 64bit
	InterruptRegisterSet_ERSTBA ERSTBA;

	//Event Ring Dequeue Pointer Register
	InterruptRegisterSet_ERDP ERDP;
};

#pragma pack(push, 1)
struct HostControllerRuntimeRegisters {
	uint32_t MFINDEX;
	char RsvdZ_1[0x1C];
	InterrupterRegisterSet InterrupterRegisters[0x8];//0x400

	uint16_t MFINDEX_Microframe_Index() {
		return this->MFINDEX & 0x3FFF;
	}
};
#pragma pack(pop)

#pragma pack(push, 1)
struct DoorbellRegister {
	uint32_t DBTarget : 8;
	uint32_t RsvdZ_0 : 8;
	uint32_t DBTaskID : 16;
};
#pragma pack(pop)

struct DoorbellRegisters {
	DoorbellRegister DoorbellRegisters[0x100];
};

struct BARLogEntry {
	uint32_t iBar;
	int64_t offset;
	uint32_t length;
	/*
	0: Read
	1: Write
	*/
	int type;
	/*
	* Write: <Before:Len>:<After:Len>
	* Read: <Data:Len>
	*/
	unsigned char* buffer;
	std::vector<std::string> descs;
};

struct eXtendedCapability {
	uint8_t Capability_ID;
	uint8_t Next_Pointer;
};

struct eXtendedCapability_USB_Legacy_Support {
	uint8_t Capability_ID;
	uint8_t Next_Pointer;
	uint8_t HC_BIOS_Owned_Semaphore : 1;
	uint8_t RsvdP_0 : 7;
	uint8_t HC_OS_Owned_Semaphore : 1;
	uint8_t RsvdP_1 : 7;

	uint32_t USBLGCTLSTS;

	bool USBLGCTLSTS_USB_SMI_Enable() {
		return this->USBLGCTLSTS & 0x1;
	}

	bool USBLGCTLSTS_SMI_on_Host_System_Error_Enable() {
		return (this->USBLGCTLSTS >> 4) & 0x1;
	}

	bool USBLGCTLSTS_SMI_on_OS_Ownership_Enable() {
		return (this->USBLGCTLSTS >> 13) & 0x1;
	}

	bool USBLGCTLSTS_SMI_on_PCI_Command_Enable() {
		return (this->USBLGCTLSTS >> 14) & 0x1;
	}

	bool USBLGCTLSTS_SMI_on_BAR_Enable() {
		return (this->USBLGCTLSTS >> 15) & 0x1;
	}

	bool USBLGCTLSTS_SMI_on_Event_Interrupt() {
		return (this->USBLGCTLSTS >> 16) & 0x1;
	}

	bool USBLGCTLSTS_SMI_on_Host_System_Error() {
		return (this->USBLGCTLSTS >> 20) & 0x1;
	}

	bool USBLGCTLSTS_SMI_on_OS_Ownership_Change() {
		return (this->USBLGCTLSTS >> 29) & 0x1;
	}

	bool USBLGCTLSTS_SMI_on_PCI_Command() {
		return (this->USBLGCTLSTS >> 30) & 0x1;
	}

	bool USBLGCTLSTS_SMI_on_BAR() {
		return (this->USBLGCTLSTS >> 31) & 0x1;
	}
};

struct eXtendedCapability_Supported_Protocol {
	uint8_t Capability_ID;
	uint8_t Next_Pointer;
	uint8_t Revision_Minor;
	uint8_t Revision_Major;
	char name[4];

	uint32_t Compatible_Port_Offset : 8;
	uint32_t Compatible_Port_Count : 8;
	uint32_t Protocol_Defined : 12;
	uint32_t Protocol_Speed_ID_Count : 4;

	unsigned Protocol_Slot_Type : 5;
	unsigned RsvdP_1 : 27;
	unsigned Protocol_Speed_ID_Value : 4;
	unsigned Protocol_Speed_ID_Exponent : 2;
	unsigned PSI_Type : 2;
	unsigned PSI_Full_Duplex : 1;
	unsigned RsvdP_2 : 5;
	unsigned Link_Protocol : 2;
	unsigned Protocol_Speed_ID_Mantissa : 16;
};

struct Event_Ring_Segment_Table_Entry {
	uint64_t RsvdZ_0 : 6;
	uint64_t Ring_Segment_Base_Address : 58;
	uint32_t Ring_Segment_Size : 16;
	uint32_t Rsvd_1 : 16;
	uint32_t Rsvd_2 : 16;
};

struct TRB_Port_Status_Change_Event {
	uint32_t RsvdZ_0 : 24;
	uint32_t Port_ID : 8;

	uint32_t RsvdZ_1;

	uint32_t RsvdZ_2 : 24;
	uint32_t Completion_Code : 8;

	uint32_t Cycle_Bit : 1;
	uint32_t RsvdZ_3 : 9;
	uint32_t TRB_Type : 6;
	uint32_t Rsvd_4 : 16;
};

struct TRB_Enable_Slot_Command {
	uint32_t RsvdZ_0;
	uint32_t RsvdZ_1;
	uint32_t RsvdZ_2;
	
	uint32_t Cycle_Bit : 1;
	uint32_t RsvdZ_3 : 9;
	uint32_t TRB_Type : 6;
	uint32_t Slot_Type : 5;
	uint32_t RsvdZ_4 : 11;
};

struct TRB_Disable_Slot_Command {
	uint32_t RsvdZ_0;
	uint32_t RsvdZ_1;
	uint32_t RsvdZ_2;

	uint32_t Cycle_Bit : 1;
	uint32_t RsvdZ_3 : 9;
	uint32_t TRB_Type : 6;
	uint32_t RsvdZ_4 : 8;
	uint32_t Slot_ID : 8;
};

struct TRB_Reset_Device_Command {
	uint32_t RsvdZ_0;
	uint32_t RsvdZ_1;
	uint32_t RsvdZ_2;

	uint32_t Cycle_bit : 1;
	uint32_t RsvdZ_3 : 9;
	uint32_t TRB_Type : 6;
	uint32_t RsvdZ_4 : 8;
	uint32_t Slot_ID : 8;
};

struct TRB_Address_Device_Command {
	uint64_t RsvdZ_0 : 4;
	uint64_t Input_Context_Pointer : 60;

	uint32_t RsvdZ_1;

	uint32_t Cycle_Bit : 1;
	uint32_t RsvdZ_2 : 8;
	uint32_t Block_Set_Address_Request : 1;
	uint32_t TRB_Type : 6;
	uint32_t RsvdZ_3 : 8;
	uint32_t Slot_ID : 8;
};

struct TRB_Reset_Endpoint_Command {
	uint32_t RsvdZ_0;
	uint32_t RsvdZ_1;
	uint32_t RsvdZ_2;

	uint32_t Cycle_bit : 1;
	uint32_t RsvdZ_3 : 8;
	uint32_t Transfer_State_Preserve : 1;
	uint32_t TRB_Type : 6;
	uint32_t Endpoint_ID : 5;
	uint32_t RsvdZ_4 : 3;
	uint32_t Slot_ID : 8;
};

struct TRB_Set_TR_Dequeue_Pointer_Command {
	uint64_t Dequeue_Cycle_state : 1;
	uint64_t Stream_Context_Type : 3;
	uint64_t New_TR_Dequeue_Pointer : 60;

	uint32_t RsvdZ_0 : 16;
	uint32_t Stream_ID : 16;

	uint32_t Cycle_bit : 1;
	uint32_t RsvdZ_1 : 9;
	uint32_t TRB_Type : 6;
	uint32_t Endpoint_ID : 5;
	uint32_t RsvdZ_2 : 3;
	uint32_t Slot_ID : 8;
};

struct TRB_Configure_Endpoint_Command {
	uint64_t RsvdZ_0 : 4;
	uint64_t Input_Context_Pointer : 60;

	uint32_t RsvdZ_1;

	uint32_t Cycle_bit : 1;
	uint32_t RsvdZ_2 : 8;
	uint32_t Deconfigure : 1;
	uint32_t TRB_Type : 6;
	uint32_t RsvdZ_3 : 8;
	uint32_t Slot_ID : 8;
};

struct TRB_Stop_Endpoint_Command_TRB {
	uint32_t RsvdZ_0;
	uint32_t RsvdZ_1;
	uint32_t RsvdZ_2;

	uint32_t Cycle_bit : 1;
	uint32_t RsvdZ_3 : 8;
	uint32_t Deconfigure : 1;
	uint32_t TRB_Type : 6;
	uint32_t Endpoint_ID : 5;
	uint32_t RsvdZ_4 : 2;
	uint32_t Suspend : 1;
	uint32_t Slot_ID : 8;
};

struct TRB_Command_Completion_Event {
	uint64_t RsvdZ_0 : 4;
	uint64_t Command_TRB_Pointer : 60;

	uint32_t Command_Completion_Parameter : 24;
	uint32_t Completion_Code : 8;

	uint32_t Cycle_Bit : 1;
	uint32_t RsvdZ_1 : 9;
	uint32_t TRB_Type : 6;
	uint32_t VF_ID : 8;
	uint32_t Slot_ID : 8;
};

struct TRB_Transfer_Event {
	uint64_t TRB_Pointer;
	
	uint32_t TRB_Transfer_Length : 24;
	uint32_t Completion_Code : 8;

	uint32_t Cycle_bit : 1;
	uint32_t RsvdZ_1 : 1;
	uint32_t Event_Data : 1;
	uint32_t RsvdZ_2 : 7;
	uint32_t TRB_Type : 6;
	uint32_t Endpoint_ID : 5;
	uint32_t RsvdZ_3 : 3;
	uint32_t Slot_ID : 8;
};

struct TRB_Link_TRB {
	uint64_t RsvdZ_0 : 4;
	uint64_t Ring_Segment_Pointer : 60;
	
	uint32_t RsvdZ_1 : 22;
	uint32_t Interrupter_Target : 10;

	uint32_t Cycle_Bit : 1;
	uint32_t Toggle_cycle : 1;
	uint32_t RsvdZ_2 : 2;
	uint32_t Chain_Bit : 1;
	uint32_t Interrupt_On_Completion : 1;
	uint32_t RsvdZ_3 : 4;
	uint32_t TRV_Type : 6;
	uint32_t RsvdZ_4 : 16;
};

struct TRB_Setup_Stage {
	uint32_t bm_Request_Type : 8;
	uint32_t b_Request : 8;
	uint32_t w_Value : 16;

	uint32_t w_Index : 16;
	uint32_t w_Length : 16;

	uint32_t TRB_Transfer_Length : 17;
	uint32_t RsvdZ_0 : 5;
	uint32_t Interrupter_Target : 10;

	uint32_t Cycle_Bit : 1;
	uint32_t RsvdZ_2 : 4;
	uint32_t Interrupt_On_Completion : 1;
	uint32_t Immediate_Data : 1;
	uint32_t RsvdZ_3 : 3;
	uint32_t TRB_Type : 6;
	uint32_t Transfer_Type : 2;
	uint32_t RsvdZ_4 : 14;
};

struct TRB_Data_Stage {
	uint64_t Data_Buffer_Pointer;

	uint32_t TRB_Transfer_Length : 17;
	uint32_t TD_Size : 5;
	uint32_t Interrupter_Target : 10;

	uint32_t Cycle_Bit : 1;
	uint32_t Evaluate_Next_TRB : 1;
	uint32_t Interrupt_on_Short_Packet : 1;
	uint32_t No_Snoop : 1;
	uint32_t Chain_bit : 1;
	uint32_t Interrupt_On_Completion : 1;
	uint32_t Immediate_data : 1;
	uint32_t RsvdZ_1 : 3;
	uint32_t TRB_Type : 6;
	uint32_t Direction : 1;
	uint32_t RsvdZ_2 : 15;
};

struct TRB_Event_Data {
	uint64_t Event_Data;
	
	uint32_t RsvdZ_0 : 22;
	uint32_t Interrupter_Target : 10;

	uint32_t Cycle_bit : 1;
	uint32_t Evaluate_Next_TRB : 1;
	uint32_t RsvdZ_1 : 2;
	uint32_t Chain_bit : 1;
	uint32_t Interrupt_On_Completion : 1;
	uint32_t RsvdZ_2 : 3;
	uint32_t Block_Event_Interrupt : 1;
	uint32_t TRB_Type : 6;
	uint32_t RsvdZ_3 : 16;
};

struct TRB_Status_Stage {
	uint64_t RsvdZ_1;
	
	uint32_t RsvdZ_2 : 22;
	uint32_t Interrupter_Target : 10;
	
	uint32_t Cycle_bit : 1;
	uint32_t Evaluate_Next_TRB : 1;
	uint32_t RsvdZ_3 : 2;
	uint32_t Chain_bit : 1;
	uint32_t Interrupt_On_Completion : 1;
	uint32_t RsvdZ_4 : 4;
	uint32_t TRB_Type : 6;
	uint32_t Direction : 1;
	uint32_t RsvdZ_5 : 15;
};

struct TRB_Generic {
	uint32_t fields[4];
};

struct TRB_Unknown {
	uint32_t pad_0;
	uint32_t pad_1;
	uint32_t pad_2;
	
	uint32_t Cycle_Bit : 1;
	uint32_t Rsvd_0 : 9;
	uint32_t TRB_Type : 6;
	uint32_t Rsvd_1 : 16;
};

struct SlotContext {
	uint32_t Route_String : 20;
	uint32_t Speed : 4;
	uint32_t RsvdZ_0 : 1;
	uint32_t Multi_TT : 1;
	uint32_t Hub : 1;
	uint32_t Context_Entries : 5;

	uint32_t Max_Exit_Latency : 16;
	uint32_t Root_Hub_Port_Number : 8;
	uint32_t Number_of_Ports : 8;

	uint32_t Parent_Hub_Slot_ID : 8;
	uint32_t Parent_Port_Number : 8;
	uint32_t TT_Think_Time : 2;
	uint32_t RsvdZ_1 : 4;
	uint32_t Interrupter_Target : 10;

	uint32_t USB_Device_Address : 8;
	uint32_t RsvdZ : 19;
	uint32_t Slot_State : 5;

	uint32_t xHCI_Rsvd_0;
	uint32_t xHCI_Rsvd_1;
	uint32_t xHCI_Rsvd_2;
	uint32_t xHCI_Rsvd_3;
};

struct EndpointContext {
	uint32_t Endpoint_State : 3;
	uint32_t RsvdZ_0 : 5;
	uint32_t Mult : 2;
	uint32_t Max_Primary_Streams : 5;
	uint32_t Linear_Stream_Array : 1;
	uint32_t Interval : 8;
	uint32_t Max_Endpoinjt_Service_Time_Interval_Payload_Hi : 8;

	uint32_t Rsvd_1 : 1;
	uint32_t Error_Count : 2;
	uint32_t Endpoint_Type : 3;
	uint32_t RsvdZ_2 : 1;
	uint32_t Host_Initiate_Disable : 1;
	uint32_t Max_Burst_Size : 8;
	uint32_t Max_Packet_Size : 16;
	
	uint64_t Dequeue_Cycle_State : 1;
	uint64_t RsvdZ_3 : 3;
	uint64_t TR_Dequeue_Pointer : 60;

	uint32_t Average_TRB_Length : 16;
	uint32_t Max_Endpoint_Service_Time_Interval_Payload_Low : 16;
	
	uint32_t xHCI_Rsvd_0;
	uint32_t xHCI_Rsvd_1;
	uint32_t xHCI_Rsvd_2;
};

struct DeviceContext {
	SlotContext SlotContext;
	EndpointContext EP_CTX_0_BI;
	EndpointContext EP_Contexts[30];

	EndpointContext* get_ctx_by_DCI(int dci) {
		if (dci == 0) {
			return (EndpointContext*)&SlotContext;
		}
		if (dci == 1) {
			return &EP_CTX_0_BI;
		}
		if (dci >= 2) {
			return &this->EP_Contexts[dci - 2];
		}
		return nullptr;
	}
};

struct InputControlContext {
	uint32_t RsvdZ_0 : 2;
	uint32_t Drop_Context_Flag : 30;

	uint32_t Add_Context_Flag;

	uint32_t RsvdZ_1;
	uint32_t RsvdZ_2;
	uint32_t RsvdZ_3;
	uint32_t RsvdZ_4;
	uint32_t RsvdZ_5;

	uint32_t Configuration_Value : 8;
	uint32_t Interface_Number : 8;
	uint32_t Alternate_String : 8;
	uint32_t RsvdZ_6 : 8;
};

struct InputContext {
	InputControlContext Input_Control_Context;
	DeviceContext Device_Context;
};

enum SlotState {
	DISABLE,
	ENABLE,
	DEFAULT,
	ADDRESSED,
	CONFIGURED
};

enum EPType {
	ET_INVALID = 0,
	ET_ISO_OUT,
	ET_BULK_OUT,
	ET_INTR_OUT,
	ET_CONTROL,
	ET_ISO_IN,
	ET_BULK_IN,
	ET_INTR_IN,
};

struct XHCIStreamContext {
	uint64_t pctx;
	unsigned int sct;

	uint64_t dequeue_pointer;
	uint32_t cycle_state : 1;
};

struct InternalEndpointContext {
	uint64_t p_ctx;

	EPType type;
	uint32_t state;

	unsigned int max_psize;
	unsigned int max_pstreams;
	bool         lsa;

	unsigned int nr_pstreams;
	XHCIStreamContext* pstreams;

	uint64_t tr_dequeue_pointer;
	uint32_t cycle_state : 1;
};

struct InternalSlotContext {
	uint32_t slot_id;
	SlotState slot_state;
	uint64_t device_context;

	uint16_t intr;

	InternalEndpointContext internal_endpoint_contexts[31];
};

struct setup_t {
	uint8_t bm_request_type;
	uint8_t b_request;

	uint16_t w_value;
	uint16_t w_index;
	uint16_t w_length;
};

struct usbmon_packet {
	uint64_t id;             /*  0: URB ID - from submission to callback */
	unsigned char type;      /*  8: Same as text; extensible. */
	unsigned char xfer_type; /*     ISO (0), Intr, Control, Bulk (3) */
	unsigned char epnum;     /*     Endpoint number and transfer direction */
	unsigned char devnum;    /*     Device address */
	uint16_t busnum;         /* 12: Bus number */
	char flag_setup;         /* 14: Same as text */
	char flag_data;          /* 15: Same as text; Binary zero is OK. */
	int64_t ts_sec;          /* 16: gettimeofday */
	int32_t ts_usec;         /* 24: gettimeofday */
	int32_t status;          /* 28: */
	unsigned int length;     /* 32: Length of data (submitted or actual) */
	unsigned int len_cap;    /* 36: Delivered length */
	union {                  /* 40: */
		//unsigned char setup[8];         /* Only for Control S-type */
		setup_t setup;
		struct iso_rec {                /* Only for ISO */
			int32_t error_count;
			int32_t numdesc;
		} iso;
	} s;
	int32_t interval;        /* 48: Only for Interrupt and ISO */
	int32_t start_frame;     /* 52: For ISO */
	uint32_t xfer_flags;     /* 56: copy of URB's transfer_flags */
	uint32_t ndesc;          /* 60: Actual number of ISO descriptors */
};

struct usb_packet_map_entry {
	uint8_t slot_id;
	uint8_t endpoint_id;
	uint8_t bm_request_type;
	uint8_t b_request;

	uint16_t w_value;
	uint16_t w_index;

	uint16_t data_length;
	uint16_t bram_address;
};

struct usb_packet_entry {
	usb_packet_map_entry* map_entry;
	void* data;
};
