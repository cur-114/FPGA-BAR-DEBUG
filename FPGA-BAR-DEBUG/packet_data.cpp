#define WIN32

#include "packet_data.hpp"
#include "structs.hpp"
#include <stdint.h>
#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <iomanip>
#include <cstdint>
#include <filesystem>
#include <pcap.h>
#include <pcap/usb.h>

std::string pcap_capture_file = "D:\\dev\\archives\\arcade-controller-2.pcap";
#define MAX_MAP_ENTRIES 128
usb_packet_map_entry map_entries[MAX_MAP_ENTRIES];

struct device_map {
	int bus_num;
	int dev_num;

	int slot_id;
};

std::vector<device_map> devices;

void init_device_map() {
	device_map dev_map = {};
	dev_map.bus_num = 1;
	dev_map.dev_num = 4;
	dev_map.slot_id = 1;
	devices.push_back(dev_map);
}

std::map<uint64_t, usbmon_packet> pending_setups;

int map_entry_index = 0;

static void new_entry(
	uint8_t slot_id,
	uint8_t endpoint_id,
	uint8_t bm_request_type,
	uint8_t b_request,
	uint16_t w_value,
	uint16_t w_index,
	uint16_t data_length,
	uint16_t bram_address,
	void* pointer
) {

	for (usb_packet_entry& entry : packet::entries) {
		if (entry.map_entry->slot_id != slot_id) continue;
		if (entry.map_entry->endpoint_id != endpoint_id) continue;
		if (entry.map_entry->bm_request_type != bm_request_type) continue;
		if (entry.map_entry->b_request != b_request) continue;
		if (entry.map_entry->w_value != w_value) continue;
		if (entry.map_entry->w_index != w_index) continue;
		if (entry.map_entry->data_length >= data_length) continue;
		printf("data update %d -> %d!\n", entry.map_entry->data_length, data_length);
		entry.data = pointer;
		entry.map_entry->data_length = data_length;
		return;
	}

	usb_packet_map_entry map_entry = {};
	map_entry.slot_id = slot_id;
	map_entry.endpoint_id = endpoint_id;
	map_entry.bm_request_type = bm_request_type;
	map_entry.b_request = b_request;
	map_entry.w_value = w_value;
	map_entry.w_index = w_index;
	map_entry.data_length = data_length;
	map_entry.bram_address = bram_address;
	map_entries[map_entry_index] = map_entry;
	usb_packet_entry entry = {};
	entry.map_entry = &map_entries[map_entry_index];
	entry.data = pointer;
	packet::entries.push_back(entry);
	map_entry_index++;
}

void load_pcap() {
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t* handle = pcap_open_offline(pcap_capture_file.c_str(), errbuf);

	if (handle == NULL) {
		std::cerr << errbuf << std::endl;
		return;
	}

	pcap_pkthdr* header;
	usbmon_packet* packet;

	int result;
	printf("datalink: %d\n", pcap_datalink(handle));
	while ((result = pcap_next_ex(handle, &header, (const u_char**) &packet)) == 1) {
		device_map *match_dev = 0;
		for (device_map& dev_map : devices)
			if (dev_map.bus_num == packet->busnum && dev_map.dev_num == packet->devnum)
				match_dev = &dev_map;
		if (!match_dev)
			continue;

		//Ctrl Packet + Submit
		if (packet->xfer_type == 2 && packet->type == 'S') {
			printf("SUBMIT  : packet: %llX %d.%d.%d\n", packet->id, packet->busnum, packet->devnum, packet->epnum & 0xF);
			pending_setups[packet->id] = *packet;
		}
		if (packet->xfer_type == 2 && packet->type == 'C') {
			printf("COMPLETE: packet: %llX %d.%d.%d\n", packet->id, packet->busnum, packet->devnum, packet->epnum & 0xF);
			setup_t setup = pending_setups[packet->id].s.setup;
			printf(" %02X %02X %04X %04X %d\n", setup.bm_request_type, setup.b_request, setup.w_index, setup.w_value, setup.w_length);
			void* payload = (void*)((uint64_t)packet + sizeof(*packet));

			void* buffer = 0;
			if (setup.w_length != 0) {
				buffer = malloc(setup.w_length);
				if (!buffer) {
					printf("failed to allocate buffer!\n");
					return;
				}
				memcpy(buffer, payload, setup.w_length);
			}

			new_entry(match_dev->slot_id, packet->epnum & 0xF, setup.bm_request_type, setup.b_request, setup.w_value, setup.w_index, setup.w_length, 0, buffer);
		}
	}
}

namespace packet {
	void init() {
		init_device_map();
		load_pcap();


	}
}