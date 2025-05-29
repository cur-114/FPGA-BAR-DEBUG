#pragma once
#include "structs.hpp"
#include "vmmdll.h"
#include "leechcore.h"
#include <vector>
#include <string>
#include <map>

namespace memory {

	inline HANDLE hLC;
	inline VMM_HANDLE hVMM;

	inline bool read_alive = false;

	inline std::vector<unsigned char> bar0;
	inline std::vector<unsigned char> bar2;
	inline std::vector<unsigned char> bar4;

	inline std::map<int, bool> Producer_Cycle_State;

	inline std::vector<BARLogEntry> bar_logs;

	inline uint64_t FLxHCIc_base;
	inline uint64_t DCBAA[9];
	inline DeviceContext DeviceContexts[9];

	namespace control {
		inline bool examine_command_ring = false;
		inline bool examine_event_ring_0 = false;
		inline bool dump_vendor_defined_caps = false;
	}

	bool initialize();
	void cleanup();
	bool loadFromFile(std::string filename, std::vector<unsigned char>& bar);
	//void send_event_ring(PBYTE trb, DWORD trb_size);
	void fuck_men();

	inline void read_raw(uint64_t address, DWORD size, PBYTE buf) {
		LcRead(hLC, address, size, buf);
	}

	inline void write_raw(uint64_t address, DWORD size, PBYTE buf) {
		LcWrite(hLC, address, size, buf);
	}

	template<class T> __forceinline T read0(int pos) {
		T buf;
		if (bar0.size() < pos + 1) {
			memset(&buf, 0, sizeof(T));
			return buf;
		}
		buf = *(T*)(bar0.data() + pos);
		return buf;
	}

	template<class T> __forceinline T readt_raw(uint64_t address) {
		T buf;
		LcRead(hLC, address, sizeof(T), (PBYTE)&buf);
		return buf;
	}

	template<class T> __forceinline T read(DWORD PID, uint64_t address, bool cache = true) {
		T buf;
		if (cache) {
			VMMDLL_MemRead(hVMM, PID, address, (PBYTE)&buf, sizeof(T));
		}
		else {
			VMMDLL_MemReadEx(hVMM, PID, address, (PBYTE)&buf, sizeof(T), 0, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT | VMMDLL_FLAG_NOPAGING_IO);
		}
		return buf;
	}
}