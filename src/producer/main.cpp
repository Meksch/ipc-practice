#include "ipc/checksum.hpp"
#include "ipc/mem_buffer.hpp"
#include "ipc/packet.hpp"
#include "ipc/pause_controller.hpp"

#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

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

    struct sigaction sa{};
    sa.sa_handler = [](int) { g_running = 0; };
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    ipc::MemBuff buffer = ipc::MemBuff::create(payload_size, slot_count);
    ipc::PauseController pause;
    pause.start();

    std::cout << "producer ready, mem-name=" << buffer.name()
              << " payload=" << payload_size << " slots=" << slot_count << '\n';

    uint32_t sequence = 0;
    while (g_running) {
        pause.process();
        if (pause.is_paused()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        ipc::Slot slot = buffer.producer_acquire();
        if (slot.header == nullptr) {  // on shutdown guard against empty slot
            continue;
        }

        fill_payload(slot.payload, payload_size);
        slot.header->marker = ipc::kMarker;
        slot.header->sequence = sequence++;
        slot.header->timestamp_ns = ipc::now_ns();
        slot.header->payload_size = payload_size;
        slot.header->checksum = ipc::crc32(slot.payload, payload_size);

        buffer.producer_release();
    }

    pause.stop();
    std::cerr << "producer stopped after " << sequence << " packets\n";
    return 0;
}
