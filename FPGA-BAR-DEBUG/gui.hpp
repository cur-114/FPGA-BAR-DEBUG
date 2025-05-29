#pragma once

namespace gui {

	void initialize();

	namespace xHCI {

		void render();
		void render_eXtencibleHostControllerCapabilityRegister();
		void render_HostControllerOperationalRegisters();
		void render_HostControllerRuntimeRegister();
		void render_eXtendedCapabilities();
		void render_DoorbellRegisters();
		void render_PortRegisters();
		void render_DCBAA();
	}
}