#pragma once

#include <cstdint>
#include <time.h>

namespace ipc {

inline constexpr uint32_t kMarker =
        static_cast<uint32_t>('I') | (static_cast<uint32_t>('P') << 8) |
        (static_cast<uint32_t>('C') << 16) | (static_cast<uint32_t>('P') << 24);

struct PacketHeader {
    uint32_t marker{kMarker};
    uint32_t sequence{0};
    uint64_t timestamp_ns{0};
    uint32_t payload_size{0};
    uint32_t checksum{0};
};

inline uint64_t now_ns() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL + static_cast<uint64_t>(ts.tv_nsec);
}
}  // namespace ipc
