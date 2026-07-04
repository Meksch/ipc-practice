#include "ipc/checksum.hpp"
#include "ipc/mem_buffer.hpp"
#include "ipc/packet.hpp"
#include "ipc/pause_controller.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <iomanip>
#include <string>
#include <thread>

volatile std::sig_atomic_t g_running = 1;

struct Stats {
    std::atomic<uint64_t> total{0};
    std::atomic<uint64_t> invalid_checksum{0};
    std::atomic<uint64_t> bad_header{0};
};

bool parse_args(int argc, char* argv[], std::string& shm_name) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--mem-name" && i + 1 < argc) {
            shm_name = argv[++i];
        } else {
            return false;
        }
    }
    return !shm_name.empty();
}

void gather_stats(const ipc::Slot& slot, uint32_t payload_size, Stats& stats) {
    stats.total.fetch_add(1);

    if (slot.header->marker != ipc::kMarker || slot.header->payload_size != payload_size) {
        stats.bad_header.fetch_add(1);
    }
    if (ipc::crc32(slot.payload, payload_size) != slot.header->checksum) {
        stats.invalid_checksum.fetch_add(1);
    }
}

void stats_loop(const Stats& stats, uint32_t payload_size, const ipc::PauseController& pause) {
    uint64_t prev_total = 0;

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        const uint64_t total = stats.total.load();
        const uint64_t delta = total - prev_total;
        prev_total = total;

        const double mib_per_s = (static_cast<double>(delta) * payload_size) / (1024.0 * 1024.0);
        const uint64_t invalid = stats.invalid_checksum.load() + stats.bad_header.load();

        std::cerr << std::fixed << std::setprecision(1)
                  << "stats: total=" << total
                  << " throughput=" << mib_per_s << " MiB/s"
                  << " invalid=" << invalid
                  << " paused=" << (pause.is_paused() ? "yes" : "no") << "\n";
    }
}

int main(int argc, char* argv[]) {
    std::string mem_name;
    if (!parse_args(argc, argv, mem_name)) {
        std::cerr << "usage: consumer --mem-name NAME\n";
        return 1;
    }

    struct sigaction sa{};
    sa.sa_handler = [](int) { g_running = 0; };
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    ipc::MemBuff buffer = ipc::MemBuff::attach(mem_name);
    const uint32_t payload_size = buffer.payload_size();
    if (payload_size == 0) {
        std::cerr << "failed to attach to " << mem_name << " (start producer first)\n";
        return 1;
    }

    ipc::PauseController pause;
    pause.start();

    std::cout << "consumer ready" << '\n';

    Stats stats;
    std::thread througput_tracker(stats_loop, std::cref(stats), payload_size, std::cref(pause));

    while (g_running) {
        pause.process();
        if (pause.is_paused()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        ipc::Slot slot = buffer.consumer_acquire();
        if (slot.header == nullptr) {  // on shutdown guard against empty slot
            continue;
        }
        gather_stats(slot, payload_size, stats);
        buffer.consumer_release();
    }

    pause.stop();
    througput_tracker.join();
    std::cerr << "consumer stopped after " << stats.total.load() << " packets\n";
    return 0;
}
