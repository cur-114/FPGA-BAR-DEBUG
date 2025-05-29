#include "gui.hpp"
#include "app.hpp"
#include "utils.hpp"
#include "structs.hpp"
#include "memory.hpp"
#include <thread>
#include <vector>
#include <iomanip>
#include <sstream>
#include <GL/gl3w.h>
#include "imgui\imgui.h"
#include "imgui\imgui_impl_glfw.h"
#include "imgui\imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

namespace gui {

    GLFWwindow *window;

	void thread_entry();
    void render_gui();
    void render_bar(int index, std::vector<unsigned char> bar);
    void render_bar_logs();
    void render_driver();

	void initialize() {
		std::thread gui_thread(thread_entry);
		gui_thread.detach();
	}

	void thread_entry() {
        if (!glfwInit()) {
            printf("Failed to init glfw!\n");
            return;
        }

        const char* glsl_version = "#version 330";

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

        window = glfwCreateWindow(1080, 900, "xHCI-FPGA-DEBUGGER", NULL, NULL);

        if (!window) {
            glfwTerminate();
            printf("Failed to create window!\n");
            return;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        gl3wInit();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            int WIDTH, HEIGHT;
            glfwGetFramebufferSize(window, &WIDTH, &HEIGHT);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

            ImGui::NewFrame();

            render_gui();

            // Rendering
            ImGui::Render();
            glClearColor(0.8f, 0.8f, 0.8f, 0.0f);
            glViewport(0, 0, WIDTH, HEIGHT);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }
        app::exit = true;

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
	}

    void render_gui() {
        render_bar(0, memory::bar0);
        render_bar(2, memory::bar2);
        render_bar(4, memory::bar4);
        render_bar_logs();
        render_driver();
        xHCI::render();
    }

    void render_bar(int index, std::vector<unsigned char> bar) {
        char title_buffer[32];
        sprintf_s(title_buffer, "BAR%d", index);
        ImGui::Begin(title_buffer);
        size_t data_size = bar.size();
        for (int i = 0; i < data_size; i+= 0x10) {
            std::stringstream line;
            line << "0x" << std::setw(4) << std::setfill('0') << std::hex << i << ": ";
            for (size_t j = 0; j < 0x10; ++j) {
                if (i + j < data_size) {
                    line << std::setw(2) << std::setfill('0') << std::hex << (int)bar[i + j] << " ";
                }
                else {
                    line << "   ";
                }
            }
            ImGui::Text("%s", line.str().c_str());
        }
        ImGui::End();
    }

    bool enable_filter = false;
    bool show_vd_ext_cap_1 = false;
    bool show_vd_ext_cap_2 = false;

    void render_bar_logs() {
        ImGui::Begin("BAR Requests");
        ImGui::Text("ALIVE: %d", memory::read_alive);
        if (ImGui::Button("CLEAR LOGS")) {
            memory::bar_logs.clear();
        }
        if (ImGui::CollapsingHeader("Filters")) {
            ImGui::Checkbox("Enable Filter", &enable_filter);
            ImGui::Checkbox("Vendor Defined Extended Capability(1)", &show_vd_ext_cap_1);
            ImGui::Checkbox("Vendor Defined Extended Capability(2)", &show_vd_ext_cap_2);
        }
        ImGui::BeginTable("BAR Requests", 7);
        ImGui::TableSetupColumn("OP", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("BAR", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("RAW", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("DESC", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("LENGTH", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("BEFORE", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("AFTER", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();
        std::vector<BARLogEntry> cached_logs = memory::bar_logs;
        for (int i = 0; i < cached_logs.size(); i++) {
            BARLogEntry entry = cached_logs[i];
            bool show = true;
            if (enable_filter) {
                show = false;
                if (show_vd_ext_cap_1 && entry.iBar == 0 && 0x8040 <= entry.offset && entry.offset < 0x8070)
                    show = true;
                if (show_vd_ext_cap_2 && entry.iBar == 0 && 0x8070 <= entry.offset && entry.offset < 0x8330)
                    show = true;
            }
            if (!show)
                continue;
            ImGui::TableNextRow();
            if (entry.type == 0 || entry.type == 1) {
                ImGui::TableNextColumn();
                ImGui::Text(entry.type == 0 ? "READ" : "WRITE");
                ImGui::TableNextColumn();
                ImGui::Text("BAR%d", entry.iBar);
                ImGui::TableNextColumn();
                ImGui::Text("0x%s", utils::toHexString2(entry.offset).c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%s", utils::data_name(entry.iBar, entry.offset).c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%d", entry.length);
                if (entry.type == 1) {
                    ImGui::TableNextColumn();
                    //ImGui::Text(utils::toHexString(entry.buffer, entry.length).c_str());
                    if (entry.length == 4) {
                        ImGui::Text(utils::toHexString2(*(uint32_t*)entry.buffer, 8).c_str());
                    }
                    else {
                        ImGui::Text(utils::toHexString2(*(uint64_t*)entry.buffer, 16).c_str());
                    }
                    ImGui::TableNextColumn();
                    //ImGui::Text(utils::toHexString(entry.buffer + entry.length, entry.length).c_str());
                    if (entry.length == 4) {
                        ImGui::Text(utils::toHexString2(*(uint32_t*)(entry.buffer + entry.length), 8).c_str());
                    }
                    else {
                        ImGui::Text(utils::toHexString2(*(uint64_t*)(entry.buffer + entry.length), 16).c_str());
                    }
                }
                else if (entry.type == 0) {
                    ImGui::TableNextColumn();
                    //ImGui::Text(utils::toHexString(entry.buffer, entry.length).c_str());
                    if (entry.length == 4) {
                        ImGui::Text(utils::toHexString2(*(uint32_t*)entry.buffer, 8).c_str());
                    }
                    else {
                        ImGui::Text(utils::toHexString2(*(uint64_t*)entry.buffer, 16).c_str());
                    }
                    ImGui::TableNextColumn();
                }
            }
            for (int j = 0; j < entry.descs.size(); j++) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TableNextColumn();
                ImGui::TableNextColumn();
                ImGui::TableNextColumn();
                ImGui::Text("%s", entry.descs[j].c_str());
            }
        }
        ImGui::EndTable();
        ImGui::End();
    }

    void render_driver() {
        ImGui::Begin("DRIVER");
        ImGui::Text("FLxHCIc");
        ImGui::Indent(10);
        ImGui::Text("Base: 0x%s", utils::toHexString2(memory::FLxHCIc_base).c_str());
        ImGui::Unindent(10);
        if (ImGui::Button("CONNECT DEVICE 4")) {
            memory::fuck_men();
        }
        if (ImGui::Button("Examine Command Ring")) {
            memory::control::examine_command_ring = true;
        }
        if (ImGui::Button("Examine Event Ring 0")) {
            memory::control::examine_event_ring_0 = true;
        }
        if (ImGui::Button("Dump Vendor Defined Caps")) {
            memory::control::dump_vendor_defined_caps = true;
        }
        //LEAVE:
        ImGui::End();
    }

    namespace xHCI {
        void render() {
            render_eXtencibleHostControllerCapabilityRegister();
            render_HostControllerOperationalRegisters();
            render_HostControllerRuntimeRegister();
            render_eXtendedCapabilities();
            render_DoorbellRegisters();
            render_PortRegisters();
            render_DCBAA();
        }

        void render_eXtencibleHostControllerCapabilityRegister() {
            ImGui::Begin("eXtencible Host Controller Capability Register");
            HostControllerCapabilityRegisters* cReg = (HostControllerCapabilityRegisters*)memory::bar0.data();
            ImGui::Text("Operational Register Space Offset: 0x%X", cReg->OperationalRegisterSpaceOffset());
            ImGui::Text("Host Controller Interface Version Number: 0x%X", cReg->HostControllerInterfaceVersionNumber());
            ImGui::Text("HCSPARAMS1");
            ImGui::Indent(10);
            ImGui::Text("Max Device Slots: %d", cReg->MaxDeviceSlots());
            ImGui::Text("Max Interrupters: %d", cReg->MaxInterrupters());
            ImGui::Text("Max Ports: %d", cReg->MaxPorts());
            ImGui::Unindent(10);
            ImGui::Text("HCSPARAMS2");
            ImGui::Indent(10);
            ImGui::Text("Isochronous Scheduling Threshold");
            ImGui::Indent(10);
            ImGui::Text("Bit3: %d", cReg->IST_BIT3());
            ImGui::Text("Frame: %d", cReg->IST_Frame());
            ImGui::Unindent(10);
            ImGui::Text("Event Ring Segment Table Max: %d", cReg->EventRingSegmentTableMax());
            ImGui::Text("Max Scratchpad Buffers: %d", cReg->MaxScratchPadBuffers());
            ImGui::Text("Scratchpad Restore: %d", cReg->ScratchPadRestore());
            ImGui::Unindent(10);
            ImGui::Text("HCSPARAMS3");
            ImGui::Indent(10);
            ImGui::Text("U1 Device Exit Latency: %d", cReg->U1DeviceExitLatency());
            ImGui::Text("U2 Device Exit Latency: %d", cReg->U2DeviceExitLatency());
            ImGui::Unindent(10);
            ImGui::Text("HCCPARAMS1");
            ImGui::Indent(10);
            ImGui::Text("64Bit Addressing: %d", cReg->Addressing64Bit());
            ImGui::Text("BW Negotiation: %d", cReg->BWNegotiation());
            ImGui::Text("Context Size: %d", cReg->ContextSize());
            ImGui::Text("Port Power Control: %d", cReg->PortPowerControl());
            ImGui::Text("Port Indicators: %d", cReg->PortIndicators());
            ImGui::Text("Light HC Rest: %d", cReg->LightHCReset());
            ImGui::Text("Latency Tolerance Messaging: %d", cReg->LatencyToleranceMessaging());
            ImGui::Text("No Secondary SID Support: %d", cReg->NoSecnodarySIDSupport());
            ImGui::Text("Pase All Event Data: %d", cReg->ParseAllEventData());
            ImGui::Text("Stopped Short Packet: %d", cReg->StoppedShortPacket());
            ImGui::Text("Stopped EDTLA: %d", cReg->StoppedEDTLA());
            ImGui::Text("Contiguous Frame ID: %d", cReg->ContiguousFrameID());
            ImGui::Text("Maximum Primary Stream Array Size: %d", cReg->MaximumPrimaryStreamArraySize());
            ImGui::Text("xHCI Extended Capabilities Pointer: 0x%X", cReg->xHCIExtendedCapabilitiesPointer());
            ImGui::Unindent(10);
            ImGui::Text("Doorbell Offset: 0x%X", cReg->DBOFF);
            ImGui::Text("Runtime Register Space Offset: 0x%X", cReg->RTSOFF);
            ImGui::Text("HCCPARAMS2");
            ImGui::Indent(10);
            ImGui::Text("U3 Entry: %d", cReg->U3Entry());
            ImGui::Text("EpCMD MaxExitLatencyTooLarge: %d", cReg->EpCMD_MaxExitLatencyTooLarge());
            ImGui::Text("Force Save Context: %d", cReg->ForceSaveContext());
            ImGui::Text("Compliance Transition: %d", cReg->ComplianceTransition());
            ImGui::Text("Large ESIT Payload: %d", cReg->LargeESITPayload());
            ImGui::Text("Configuration Information: %d", cReg->ConfigurationInformation());
            ImGui::Text("Extended TBC: %d", cReg->ExtendedTBC());
            ImGui::Text("Extended TBC TRB Status: %d", cReg->ExtendedTBC_TRB_Status());
            ImGui::Text("Get/Set Extended Property: %d", cReg->GetSet_Extended_Proterty());
            ImGui::Text("Virtualization Based Trusted IO: %d", cReg->VirtualizationBasedTrustedIO());
            ImGui::Unindent(10);
            ImGui::End();
        }

        void render_HostControllerOperationalRegisters() {
            ImGui::Begin("Host Controller Operational Registers");
            HostControllerCapabilityRegisters *cReg = (HostControllerCapabilityRegisters *)memory::bar0.data();
            HostControllerOperationalRegisters* oReg = (HostControllerOperationalRegisters*)(memory::bar0.data() + cReg->OperationalRegisterSpaceOffset());
            ImGui::Text("USB Command");
            ImGui::Indent(10);
            ImGui::Text("RUN/STOP: %d", oReg->USBCMD.RunStop);
            ImGui::Text("Host Controller Reset: %d", oReg->USBCMD.Host_Controller_Reset);
            ImGui::Text("Interrupter Enable: %d", oReg->USBCMD.Interrupter_Enable);
            ImGui::Text("Host System Error Enable: %d", oReg->USBCMD.Host_System_Error_Enable);
            ImGui::Text("Light Host Controller Reset: %d", oReg->USBCMD.Light_Host_Controller_Reset);
            ImGui::Text("Controller Save State: %d", oReg->USBCMD.Controller_Save_State);
            ImGui::Text("Controller Restore State: %d", oReg->USBCMD.Controller_Restore_State);
            ImGui::Text("Enable Wrap Event: %d", oReg->USBCMD.Enable_Wrap_Event);
            ImGui::Text("Enable U3 MFINDEX Stop: %d", oReg->USBCMD.Enable_U3_MFINDEX_Stop);
            ImGui::Text("CEM Enable: %d", oReg->USBCMD.CEM_Enable);
            ImGui::Text("Extended TBC Enable: %d", oReg->USBCMD.Extended_TBC_Enable);
            ImGui::Text("Extended TBC TRB Status Enable: %d", oReg->USBCMD.Extended_TBC_TRB_Status_Enable);
            ImGui::Text("VTIO Enable: %d", oReg->USBCMD.VTIO_Enable);
            ImGui::Unindent(10);
            ImGui::Text("USB Status");
            ImGui::Indent(10);
            ImGui::Text("HCHalted: %d", oReg->USBSTS.HCHalted);
            ImGui::Text("Host System Error: %d", oReg->USBSTS.Host_System_Error);
            ImGui::Text("Event Interrupt: %d", oReg->USBSTS.Event_Interrupt);
            ImGui::Text("Port Change Detect: %d", oReg->USBSTS.Port_Change_Detect);
            ImGui::Text("Save State Status: %d", oReg->USBSTS.Save_State_Status);
            ImGui::Text("Restore State Status: %d", oReg->USBSTS.Restore_State_Status);
            ImGui::Text("SAVERESTORE Error: %d", oReg->USBSTS.SaveRestore_Error);
            ImGui::Text("Controller Not Ready: %d", oReg->USBSTS.Controller_Not_Ready);
            ImGui::Text("Host Controller Error: %d", oReg->USBSTS.Host_Controller_Error);
            ImGui::Unindent(10);
            ImGui::Text("Page Size: 0x%X", oReg->PageSize());
            if (ImGui::CollapsingHeader("Device Notification Register")) {
                ImGui::Indent(10);
                for (int i = 0; i <= 15; i++) {
                    ImGui::Text("Enable %d: %d", i, oReg->DNCTRL_Notification_Enable(i));
                }
                ImGui::Unindent(10);
            }
            ImGui::Text("Command Ring Control Register");
            ImGui::Indent(10);
            ImGui::Text("Ring Cycle State: %d", oReg->CRCR.Ring_Cycle_State);
            ImGui::Text("Command Stop: %d", oReg->CRCR.Command_Stop);
            ImGui::Text("Command Abort: %d", oReg->CRCR.Command_Abort);
            ImGui::Text("Command Ring Running: %d", oReg->CRCR.Command_Ring_Running);
            ImGui::Text("Command Ring Pointer: 0x%s", utils::toHexString2(oReg->CRCR.Command_Ring_Pointer << 6));
            ImGui::Unindent(10);
            ImGui::Text("Device Context Base Address Array Pointer Register");
            ImGui::Indent(10);
            ImGui::Text("Pointer: 0x%s", utils::toHexString2(oReg->DCBAAP));
            for (int i = 0; i < 9; i++) {
                if (i == 0) {
                    ImGui::Text("ScratchPad: 0x%s", utils::toHexString2(memory::DCBAA[i]));
                }
                else {
                    ImGui::Text("DeviceCTX#%d: 0x%s", i, utils::toHexString2(memory::DCBAA[i]));
                }
            }
            ImGui::Unindent(10);
            ImGui::Text("Configuration Register");
            ImGui::Indent(10);
            ImGui::Text("Max Device Slots Enabled: %d", oReg->CONFIG.Max_Device_Slots_Enabled);
            ImGui::Text("U3 Entry Enable: %d", oReg->CONFIG.U3_Entry_Enable);
            ImGui::Text("Configuration Information Enable: %d", oReg->CONFIG.Configuration_Information_Enable);
            ImGui::Unindent(10);
            ImGui::End();
        }

        void render_HostControllerRuntimeRegister() {
            HostControllerCapabilityRegisters* cReg = (HostControllerCapabilityRegisters*)memory::bar0.data();
            HostControllerRuntimeRegisters* rReg = (HostControllerRuntimeRegisters*)(memory::bar0.data() + cReg->RTSOFF);
            ImGui::Begin("Host Controller Runtime Register");
            ImGui::Text("Microframe Index: %d", rReg->MFINDEX_Microframe_Index());
            if (ImGui::CollapsingHeader("Interrupter Registers")) {
                ImGui::BeginTable("Interrupter Registers", 10);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("#");
                ImGui::TableNextColumn();
                ImGui::Text("PEDNGING");
                ImGui::TableNextColumn();
                ImGui::Text("Enabled");
                ImGui::TableNextColumn();
                ImGui::Text("Moderation\nInterval");
                ImGui::TableNextColumn();
                ImGui::Text("Moderation\nCount");
                ImGui::TableNextColumn();
                ImGui::Text("Event\nRing\nSegment\nTable\nSize");
                ImGui::TableNextColumn();
                ImGui::Text("Event\nRing\nSegment\nTable\nBase\nAddress");
                ImGui::TableNextColumn();
                ImGui::Text("Dequeue\nERST\nSegment\nIndex");
                ImGui::TableNextColumn();
                ImGui::Text("Event\nHandler\nBusy");
                ImGui::TableNextColumn();
                ImGui::Text("Event\nRing\nDequeue\nPointer");
                for (int i = 0; i < 0x400; i++) {
                    InterrupterRegisterSet* iReg = &rReg->InterrupterRegisters[i];
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("#%d", i);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", iReg->IMAN.Interrupt_Pending);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", iReg->IMAN.Interrupt_Enable);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", iReg->IMOD.Interrupt_Moderation_Interval);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", iReg->IMOD.Interrupt_Moderation_Count);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", iReg->ERSTSZ.Event_Ring_Segment_Table_Size);
                    ImGui::TableNextColumn();
                    //ImGui::Text("%X", iReg->Event_Ring_Segment_Table_Base_Address);
                    ImGui::Text("0x%s", utils::toHexString2(iReg->ERSTBA.Event_Ring_Segment_Table_Base_Address_Register << 6).c_str());
                    
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", iReg->ERDP.Dequeue_ERST_Segment_Index);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", iReg->ERDP.Event_Handler_Busy);
                    ImGui::TableNextColumn();
                    ImGui::Text("0x%s", utils::toHexString2(iReg->ERDP.Event_Ring_Dequeue_Pointer << 4));
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }

        void render_DCBAA() {
            HostControllerCapabilityRegisters* cReg = (HostControllerCapabilityRegisters*)memory::bar0.data();
            HostControllerOperationalRegisters* opReg = (HostControllerOperationalRegisters*)(memory::bar0.data() + cReg->OperationalRegisterSpaceOffset());
            ImGui::Begin("Device Context Base Address Array");
            ImGui::Text("Pointer: 0x%s", utils::toHexString2(opReg->DCBAAP));
            for (int i = 0; i < 9; i++) {
                char title[64];
                if (i == 0) {
                    sprintf_s(title, "ScratchPad: 0x%s", utils::toHexString2(memory::DCBAA[i]).c_str());
                }
                else {
                    sprintf_s(title, "DeviceCTX#%d: 0x%s", i, utils::toHexString2(memory::DCBAA[i]).c_str());
                }
                if (ImGui::CollapsingHeader(title)) {
                    SlotContext* SlotCTX = &memory::DeviceContexts[i].SlotContext;
                    ImGui::Indent(10);
                    ImGui::Text("SlotContext");
                    ImGui::Indent(10);
                    ImGui::Text("Route String: %d", SlotCTX->Route_String);
                    ImGui::Text("Speed: %d", SlotCTX->Speed);
                    ImGui::Text("Multi TT: %d", SlotCTX->Multi_TT);
                    ImGui::Text("Hub: %d", SlotCTX->Hub);
                    ImGui::Text("Context Entries: %d", SlotCTX->Context_Entries);
                    ImGui::Text("Max Exit Latency: %d", SlotCTX->Max_Exit_Latency);
                    ImGui::Text("Root Hub Port Number: %d", SlotCTX->Root_Hub_Port_Number);
                    ImGui::Text("Number of Ports: %d", SlotCTX->Number_of_Ports);
                    ImGui::Text("Parent Hub Slot ID: %d", SlotCTX->Parent_Hub_Slot_ID);
                    ImGui::Text("Parent Port Number: %d", SlotCTX->Parent_Port_Number);
                    ImGui::Text("TT Think Time: %d", SlotCTX->TT_Think_Time);
                    ImGui::Text("Interrupter Target: %d", SlotCTX->Interrupter_Target);
                    ImGui::Text("USB Device Address: %d", SlotCTX->USB_Device_Address);
                    ImGui::Text("Slot State: %d", SlotCTX->Slot_State);
                    ImGui::Unindent(10);
                    ImGui::Unindent(10);
                }
            }
            ImGui::End();
        }
    
        void render_eXtendedCapabilities() {
            HostControllerCapabilityRegisters* cReg = (HostControllerCapabilityRegisters*)memory::bar0.data();
            eXtendedCapability* eCap = (eXtendedCapability*)(memory::bar0.data() + cReg->xHCIExtendedCapabilitiesPointer());
            ImGui::Begin("Extended Capabilities");
            while (true) {
                uint64_t next = eCap->Next_Pointer;
                next <<= 2;
                uint8_t* next_ptr = (uint8_t*)eCap + next;
                std::string cap_name = "unknown";
                switch (eCap->Capability_ID) {
                case 1:
                    cap_name = "USB Legacy Support";
                    break;
                case 2:
                    cap_name = "Supported Protocol";
                    break;
                case 192:
                    cap_name = "Vendor Defined 1";
                    break;
                case 193:
                    cap_name = "Vendor Defined 2";
                    break;
                default:
                    cap_name = "unknown";
                }
                ImGui::Text("Capability ID: %d(%s)", eCap->Capability_ID, cap_name.c_str());
                ImGui::Indent(10);
                ImGui::Text("Range: 0x%X => 0x%X", (uint64_t)eCap - (uint64_t)memory::bar0.data(), next_ptr - (uint64_t)memory::bar0.data());
                if (eCap->Capability_ID == 1) {
                    eXtendedCapability_USB_Legacy_Support* xCap = (eXtendedCapability_USB_Legacy_Support*)eCap;
                    ImGui::Text("HC BIOS Owned Semaphore: %d", xCap->HC_BIOS_Owned_Semaphore);
                    ImGui::Text("HC OS Owned Semaphore: %d", xCap->HC_OS_Owned_Semaphore);
                    ImGui::Text("USB Legacy Support Control/Status");
                    ImGui::Indent(10);
                    ImGui::Text("USB SMI Enable: %d", xCap->USBLGCTLSTS_USB_SMI_Enable());
                    ImGui::Text("SMI on Host System Error Enable: %d", xCap->USBLGCTLSTS_SMI_on_Host_System_Error_Enable());
                    ImGui::Text("SMI on OS Ownership Enable: %d", xCap->USBLGCTLSTS_SMI_on_OS_Ownership_Enable());
                    ImGui::Text("SMI on PCI Command Enable: %d", xCap->USBLGCTLSTS_SMI_on_PCI_Command_Enable());
                    ImGui::Text("SMI on BAR Enable: %d", xCap->USBLGCTLSTS_SMI_on_BAR_Enable());
                    ImGui::Text("SMI on Event Interrupt: %d", xCap->USBLGCTLSTS_SMI_on_Event_Interrupt());
                    ImGui::Text("SMI on Host System Error: %d", xCap->USBLGCTLSTS_SMI_on_Host_System_Error());
                    ImGui::Text("SMI on OS Ownership Change: %d", xCap->USBLGCTLSTS_SMI_on_OS_Ownership_Change());
                    ImGui::Text("SMI on PCI Command: %d", xCap->USBLGCTLSTS_SMI_on_PCI_Command());
                    ImGui::Text("SMI on BAR: %d", xCap->USBLGCTLSTS_SMI_on_BAR());
                    ImGui::Unindent(10);
                }
                else if (eCap->Capability_ID == 2) {
                    eXtendedCapability_Supported_Protocol* xCap = (eXtendedCapability_Supported_Protocol*)eCap;
                    ImGui::Text("Revision Minor: %X", xCap->Revision_Minor);
                    ImGui::Text("Revision Major: %X", xCap->Revision_Major);
                    
                    ImGui::Text("Name: %.4s", xCap->name);
                    ImGui::Text("Compatible Port Offset: %d", xCap->Compatible_Port_Offset);
                    ImGui::Text("Compatible Port Count: %d", xCap->Compatible_Port_Count);
                    ImGui::Text("Protocol Defined: %d", xCap->Protocol_Defined);
                    ImGui::Text("Protocol Speed ID Count: %d", xCap->Protocol_Speed_ID_Count);
                    ImGui::Text("Protocol Slot Type: %d", xCap->Protocol_Slot_Type);
                    ImGui::Text("Protocol Speed ID Value: %d", xCap->Protocol_Speed_ID_Value);
                    ImGui::Text("Protocol Speed ID Exponent: %d", xCap->Protocol_Speed_ID_Exponent);
                    ImGui::Text("PSI Type: %d", xCap->PSI_Type);
                    ImGui::Text("PSI Full Duplex: %d", xCap->PSI_Full_Duplex);
                    ImGui::Text("Link Protocol: %d", xCap->Link_Protocol);
                    ImGui::Text("Protocol Speed ID Mantissa: %d", xCap->Protocol_Speed_ID_Mantissa);
                }
                ImGui::Unindent(10);
                if (next == 0)
                    break;
                eCap = (eXtendedCapability*)(next_ptr);
            }
            ImGui::End();
        }

        void render_DoorbellRegisters() {
            HostControllerCapabilityRegisters* cReg = (HostControllerCapabilityRegisters*)memory::bar0.data();
            DoorbellRegisters* dbRegs = (DoorbellRegisters*)(memory::bar0.data() + cReg->DBOFF);
            ImGui::Begin("Doorbell Registers");
            if (ImGui::BeginTable("Doorbell Registers", 3)) {
                ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("DB Target", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("DB Task ID", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();
                for (int i = 0; i < 0x100; i++) {
                    DoorbellRegister* dbReg = &dbRegs->DoorbellRegisters[i];
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", i);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", dbReg->DBTarget);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", dbReg->DBTaskID);
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }
        
        int draw_registers = 8;

        void render_PortRegisters() {
            HostControllerCapabilityRegisters* cReg = (HostControllerCapabilityRegisters*)memory::bar0.data();
            HostControllerOperationalRegisters* opReg = (HostControllerOperationalRegisters*)(memory::bar0.data() + cReg->OperationalRegisterSpaceOffset());
            ImGui::Begin("Operational Registers => Port Registers");
            ImGui::InputInt("Draw Registers", &draw_registers);
            if (ImGui::BeginTable("Port Registers", 1 + draw_registers)) {
                ImGui::TableSetupColumn("NAME", ImGuiTableColumnFlags_WidthStretch);
                for (int i = 0; i < draw_registers; i++) {
                    std::stringstream ss;
                    ss << "#" << i;
                    ImGui::TableSetupColumn(ss.str().c_str(), ImGuiTableColumnFlags_WidthFixed);
                }
                ImGui::TableHeadersRow();
                int count = 0;
                while (true) {
                    ImGui::TableNextRow();
                    for (int i = 0; i < draw_registers + 1; i++) {
                        ImGui::TableNextColumn();
                        if (i == 0) {
                            if (count == 0) {
                                ImGui::Text("Port Status and Control Register");
                            }
                            else if (count == 1) {
                                ImGui::Text(" Current Connect Status");
                            }
                            else if (count == 2) {
                                ImGui::Text(" Port Enabled/Disabled");
                            }
                            else if (count == 3) {
                                ImGui::Text(" Over-current Active");
                            }
                            else if (count == 4) {
                                ImGui::Text(" Port Reset");
                            }
                            else if (count == 5) {
                                ImGui::Text(" Port Link State");
                            }
                            else if (count == 6) {
                                ImGui::Text(" Port Power");
                            }
                            else if (count == 7) {
                                ImGui::Text(" Port Speed");
                            }
                            else if (count == 8) {
                                ImGui::Text(" Port Indicator Control");
                            }
                            else if (count == 9) {
                                ImGui::Text(" Port Link State Write Strobe");
                            }
                            else if (count == 10) {
                                ImGui::Text(" Connect Status Change");
                            }
                            else if (count == 11) {
                                ImGui::Text(" Port Enabled/Disabled Change");
                            }
                            else if (count == 12) {
                                ImGui::Text(" Warm Port Reset Change");
                            }
                            else if (count == 13) {
                                ImGui::Text(" Over-current Change");
                            }
                            else if (count == 14) {
                                ImGui::Text(" Port Reset Change");
                            }
                            else if (count == 15) {
                                ImGui::Text(" Port Link State Change");
                            }
                            else if (count == 16) {
                                ImGui::Text(" Port Config Error Change");
                            }
                            else if (count == 17) {
                                ImGui::Text(" Cold Attach Status");
                            }
                            else if (count == 18) {
                                ImGui::Text(" Wake on Connect Enable");
                            }
                            else if (count == 19) {
                                ImGui::Text(" Wake on Disconnect Enable");
                            }
                            else if (count == 20) {
                                ImGui::Text(" Wake on Over-current Enable");
                            }
                            else if (count == 21) {
                                ImGui::Text(" Device Removable");
                            }
                            else if (count == 22) {
                                ImGui::Text(" Warm Port Reset");
                            }
                            else if (count == 23) {
                                ImGui::Text("Port Type");
                            }
                            else if (count == 24) {
                                ImGui::Text("Port PM Status and Control Register(USB3)");
                            }
                            else if (count == 25) {
                                ImGui::Text(" U1 Timeout");
                            }
                            else if (count == 26) {
                                ImGui::Text(" U2 Timeout");
                            }
                            else if (count == 27) {
                                ImGui::Text(" Force Link PM Accept");
                            }
                            else if (count == 28) {
                                ImGui::Text("Port PM Status and Control Register(USB2)");
                            }
                            else if (count == 29) {
                                ImGui::Text(" L1 Status");
                            }
                            else if (count == 30) {
                                ImGui::Text(" Remote Wake Enable");
                            }
                            else if (count == 31) {
                                ImGui::Text(" Best Effort Service Latency");
                            }
                            else if (count == 32) {
                                ImGui::Text(" L1 Device Slot");
                            }
                            else if (count == 33) {
                                ImGui::Text(" Hardware LPM Enable");
                            }
                            else if (count == 34) {
                                ImGui::Text("Port Link Register(USB3)");
                            }
                            else if (count == 35) {
                                ImGui::Text(" Link Error Count");
                            }
                            else if (count == 36) {
                                ImGui::Text(" Rx Lane Count");
                            }
                            else if (count == 37) {
                                ImGui::Text(" Tx Lane Count");
                            }
                            else if (count == 38) {
                                ImGui::Text("Port Link Register(USB2)");
                            }
                            else if (count == 39) {
                                ImGui::Text(" RsvdP");
                            }
                            else if (count == 40) {
                                ImGui::Text("Port Extended Status and Control Register(USB3)");
                            }
                            else if (count == 41) {
                                ImGui::Text(" Link Soft Error Count");
                            }
                            else if (count == 42) {
                                ImGui::Text("Port Extended Status and Control Register(USB2)");
                            }
                            else if (count == 43) {
                                ImGui::Text(" Host initiated Resume Duration Mode");
                            }
                            else if (count == 44) {
                                ImGui::Text(" L1 Timeout");
                            }
                            else if (count == 45) {
                                ImGui::Text(" Best Effort Service Latency Deep");
                            }
                        }
                        else {
                            int index = i - 1;
                            PortRegisterSet pr = opReg->PortRegisters[index];
                            bool is_usb3 = utils::get_port_type(index) == 3;
                            bool is_usb2 = utils::get_port_type(index) == 2;
                            if (count == 0) {
                            }
                            else if (count == 1) {
                                ImGui::Text("%d", pr.PORTSC.Current_Connect_Status);
                            }
                            else if (count == 2) {
                                ImGui::Text("%d", pr.PORTSC.Port_Enable_Disable);
                            }
                            else if (count == 3) {
                                ImGui::Text("%d", pr.PORTSC.Over_Current_Active);
                            }
                            else if (count == 4) {
                                ImGui::Text("%d", pr.PORTSC.Port_Reset);
                            }
                            else if (count == 5) {
                                ImGui::Text("%d", pr.PORTSC.Port_Link_State);
                            }
                            else if (count == 6) {
                                ImGui::Text("%d", pr.PORTSC.Port_Power);
                            }
                            else if (count == 7) {
                                ImGui::Text("%d", pr.PORTSC.Port_Speed);
                            }
                            else if (count == 8) {
                                ImGui::Text("%d", pr.PORTSC.Port_Indicator_Control);
                            }
                            else if (count == 9) {
                                ImGui::Text("%d", pr.PORTSC.Port_Link_State_Write_Strobe);
                            }
                            else if (count == 10) {
                                ImGui::Text("%d", pr.PORTSC.Connect_Status_Change);
                            }
                            else if (count == 11) {
                                ImGui::Text("%d", pr.PORTSC.Port_Enabled_Disabled_Change);
                            }
                            else if (count == 12) {
                                ImGui::Text("%d", pr.PORTSC.Warm_Port_Reset_Change);
                            }
                            else if (count == 13) {
                                ImGui::Text("%d", pr.PORTSC.Over_Current_Change);
                            }
                            else if (count == 14) {
                                ImGui::Text("%d", pr.PORTSC.Port_Reset_Change);
                            }
                            else if (count == 15) {
                                ImGui::Text("%d", pr.PORTSC.Port_Link_State_Change);
                            }
                            else if (count == 16) {
                                ImGui::Text("%d", pr.PORTSC.Port_Config_Error_Change);
                            }
                            else if (count == 17) {
                                ImGui::Text("%d", pr.PORTSC.Cold_Attach_Status);
                            }
                            else if (count == 18) {
                                ImGui::Text("%d", pr.PORTSC.Wake_on_Connect_Enable);
                            }
                            else if (count == 19) {
                                ImGui::Text("%d", pr.PORTSC.Wake_on_Disconnect_Enable);
                            }
                            else if (count == 20) {
                                ImGui::Text("%d", pr.PORTSC.Wake_on_Over_current_Enable);
                            }
                            else if (count == 21) {
                                ImGui::Text("%d", pr.PORTSC.Device_Removable);
                            }
                            else if (count == 22) {
                                ImGui::Text("%d", pr.PORTSC.Warm_Port_Reset);
                            }
                            else if (count == 23) {
                                ImGui::Text("%d", utils::get_port_type(index));
                            }
                            else if (count == 24) {}
                            else if (count == 25) {
                                if (is_usb3) {
                                    ImGui::Text("%d", pr.PORTPMSC_USB3.U1_Timeout);
                                }
                                else {
                                    ImGui::Text("-");
                                }
                            }
                            else if (count == 26) {
                                if (is_usb3) {
                                    ImGui::Text("%d", pr.PORTPMSC_USB3.U2_Timeout);
                                }
                                else {
                                    ImGui::Text("-");
                                }
                            }
                            else if (count == 27) {
                                if (is_usb3) {
                                    ImGui::Text("%d", pr.PORTPMSC_USB3.Force_Link_PM_Accept);
                                }
                                else {
                                    ImGui::Text("-");
                                }
                            }
                            else if (count == 28) {}
                            else if (count == 29) {
                                if (is_usb2) {
                                    ImGui::Text("%d", pr.PORTPMSC_USB2.L1_Status);
                                }
                                else {
                                    ImGui::Text("-");
                                }
                            }
                            else if (count == 30) {
                                if (is_usb2) {
                                    ImGui::Text("%d", pr.PORTPMSC_USB2.Remote_Wake_Enable);
                                }
                                else {
                                    ImGui::Text("-");
                                }
                            }
                            else if (count == 31) {
                                if (is_usb2) {
                                    ImGui::Text("%d", pr.PORTPMSC_USB2.Best_Effort_Service_Latency);
                                }
                                else {
                                    ImGui::Text("-");
                                }
                            }
                            else if (count == 32) {
                                if (is_usb2) {
                                    ImGui::Text("%d", pr.PORTPMSC_USB2.L1_Device_Slot);
                                }
                                else {
                                    ImGui::Text("-");
                                }
                            }
                            else if (count == 33) {
                                if (is_usb2) {
                                    ImGui::Text("%d", pr.PORTPMSC_USB2.Hardware_LPM_Enable);
                                }
                                else {
                                    ImGui::Text("-");
                                }
                            }
                            else if (count == 34) {}
                            else if (count == 35) {
                                if (is_usb3) {
                                    ImGui::Text("%d", pr.PORTLI_USB3.Link_Error_Count);
                                }
                                else {
                                    ImGui::Text("-");
                                }
                            }
                            else if (count == 36) {
                                if (is_usb3) {
                                    ImGui::Text("%d", pr.PORTLI_USB3.Rx_Lane_Count);
                                }
                                else {
                                    ImGui::Text("-");
                                }
                            }
                            else if (count == 37) {
                                if (is_usb3) {
                                    ImGui::Text("%d", pr.PORTLI_USB3.Tx_Lane_Count);
                                }
                                else {
                                    ImGui::Text("-");
                                }
                            }
                            else if (count == 38) {}
                            else if (count == 39) {
                                if (is_usb2) {
                                    ImGui::Text("0x%X", pr.PORTLI_USB2.RsvdP_0);
                                }
                                else {
                                    ImGui::Text("-");
                                }
                            }
                            else if (count == 40) {}
                            else if (count == 41) {
                                if (is_usb3) {
                                    ImGui::Text("%d", pr.PORTHLPMC_USB3.Link_Soft_Error_Count);
                                }
                                else {
                                    ImGui::Text("-");
                                }
                            }
                            else if (count == 42) {}
                            else if (count == 43) {
                                if (is_usb2) {
                                    ImGui::Text("%d", pr.PORTHLPMC_USB2.Host_Initiated_Resume_Duration_Mode);
                                }
                                else {
                                    ImGui::Text("-");
                                }
                            }
                            else if (count == 44) {
                                if (is_usb2) {
                                    ImGui::Text("%d", pr.PORTHLPMC_USB2.L1_Timeout);
                                }
                                else {
                                    ImGui::Text("-");
                                }
                            }
                            else if (count == 45) {
                                 if (is_usb2) {
                                     ImGui::Text("%d", pr.PORTHLPMC_USB2.Best_Effort_Service_Latency_Deep);
                                 }
                                 else {
                                     ImGui::Text("-");
                                 }
                             }
                        }
                    }
                    if (count == 45) {
                        break;
                    }
                    count++;
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }
    }
}