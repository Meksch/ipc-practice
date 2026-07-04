#pragma once

#include <cstddef>
#include <cstdint>

namespace ipc {

uint32_t crc32(const uint8_t* data, std::size_t length);

}  // namespace ipc
