#include "ipc/checksum.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace ipc {

constexpr uint32_t crc32_table_entry(uint32_t byte) {
    uint32_t crc = byte;
    for (int bit = 0; bit < 8; ++bit) {
        crc = (crc & 1U) != 0U ? (crc >> 1U) ^ 0xEDB88320U : (crc >> 1U);
    }
    return crc;
}

template <std::size_t... I>
constexpr std::array<uint32_t, 256> make_crc32_table(std::index_sequence<I...>) {
    return {{crc32_table_entry(static_cast<uint32_t>(I))...}};
}

constexpr auto kCrc32Table = make_crc32_table(std::make_index_sequence<256>{});

uint32_t crc32(const uint8_t* data, std::size_t length) {
    uint32_t crc = 0xFFFFFFFFU;

    for (std::size_t i = 0; i < length; ++i) {
        crc = kCrc32Table[(crc ^ data[i]) & 0xFFU] ^ (crc >> 8U);
    }

    return crc ^ 0xFFFFFFFFU;
}

}  // namespace ipc
