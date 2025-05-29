#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

namespace utils {

	std::string toHexString(unsigned char* ptr, size_t length);
	std::string toHexString2(uint64_t value, int fill = 0);
	std::vector<int> changedBits(uint32_t ov, uint32_t nv);
	std::string data_name(uint32_t bar, uint32_t offset);
	int get_port_type(int port);
	std::string get_trb_type_name(int type);
}