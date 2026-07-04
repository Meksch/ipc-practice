#include "ipc/checksum.hpp"
#include "ipc/mem_buffer.hpp"
#include "ipc/packet.hpp"

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

volatile std::sig_atomic_t g_running = 1;

void fill_payload(uint8_t* payload, uint32_t size) {
    for (uint32_t i = 0; i < size; ++i) {
        payload[i] = static_cast<uint8_t>(i % 256U);
    }
}

bool parse_args(int argc, char* argv[], uint32_t& payload_size, uint32_t& slot_count) {
    payload_size = 0;
    slot_count = 1024;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--payload-size" && i + 1 < argc) {
            payload_size = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--slots" && i + 1 < argc) {
            slot_count = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else {
            return false;
        }
    }

    return payload_size > 0 && slot_count >= 2;
}

int main(int argc, char* argv[]) {
    uint32_t payload_size = 0;
    uint32_t slot_count = 0;
    if (!parse_args(argc, argv, payload_size, slot_count)) {
        std::cerr << "usage: producer --payload-size N [--slots N]\n";
        return 1;
    }

    std::signal(SIGINT, [](int) { g_running = 0; });  //kill -INT <pid>
    std::signal(SIGTERM, [](int) { g_running = 0; }); //kill -TERM <pid>

    ipc::MemBuff buffer = ipc::MemBuff::create(payload_size, slot_count);
    std::cout << "producer ready, mem-name=" << buffer.name()
              << " payload=" << payload_size << " slots=" << slot_count << '\n';

    uint32_t sequence = 0;
    while (g_running) {
        ipc::Slot slot = buffer.producer_acquire();

        fill_payload(slot.payload, payload_size);
        slot.header->marker = ipc::kMarker;
        slot.header->sequence = sequence++;
        slot.header->timestamp_ns = ipc::now_ns();
        slot.header->payload_size = payload_size;
        slot.header->checksum = ipc::crc32(slot.payload, payload_size);

        buffer.producer_release();
    }

    std::cerr << "producer stopped after " << sequence << " packets\n";
    return 0;
}
