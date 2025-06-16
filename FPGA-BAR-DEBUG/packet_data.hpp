#pragma once

#include "structs.hpp"
#include <vector>

namespace packet {

	inline std::vector<usb_packet_entry> entries;

	void init();
}