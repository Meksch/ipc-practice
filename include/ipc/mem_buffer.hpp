#pragma once

#include "ipc/packet.hpp"
#include "ipc/shared_memory.hpp"

#include <atomic>
#include <cstdint>
#include <semaphore.h>
#include <string>

namespace ipc {

struct ControlBlock {
    uint32_t payload_size{0};
    uint32_t slot_count{0};
    std::atomic<bool> producer_paused{false};
    std::atomic<bool> consumer_paused{false};
    std::atomic<uint32_t> write_index{0};
    std::atomic<uint32_t> read_index{0};
    sem_t sem_empty{};
    sem_t sem_full{};
};

struct Slot {
    PacketHeader* header{nullptr};
    uint8_t* payload{nullptr};
};

class MemBuff {
public:
    MemBuff() = default;
    ~MemBuff();

    static MemBuff create(uint32_t payload_size, uint32_t slot_count);   // for producer
    static MemBuff attach(const std::string& name);                      // for consumer

    const std::string& name() const { return m_memory.name(); }
    uint32_t payload_size() const { return m_control->payload_size; }

    Slot producer_acquire();     // get a slot for producer
    void producer_release();     // release a slot for producer

    Slot consumer_acquire();    // get a slot for consumer
    void consumer_release();    // release a slot for consumer

private:
    MemBuff(SharedMemory memory, ControlBlock* control, uint8_t* slots, bool owner);

    Slot slot_at(uint32_t index) const;

    SharedMemory m_memory;
    ControlBlock* m_control{nullptr};
    uint8_t* m_slots{nullptr};            // pointer to the slots array
    bool m_owner{false};                  // is memory owned by this process
};

}  // namespace ipc
