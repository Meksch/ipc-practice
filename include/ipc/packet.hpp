#pragma once

#include <cstdint>

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

uint64_t now_ns();

}  // namespace ipc
