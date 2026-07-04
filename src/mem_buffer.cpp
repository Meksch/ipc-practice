#include "ipc/mem_buffer.hpp"

#define SLOT_BYTES(payload_size) (sizeof(PacketHeader) + payload_size)
#define REGION_BYTES(payload_size, slot_count) (sizeof(ControlBlock) + static_cast<std::size_t>(slot_count) * SLOT_BYTES(payload_size))
#define SLOTS_PTR(base) (static_cast<uint8_t*>(base) + sizeof(ControlBlock))

namespace ipc {

MemBuff::MemBuff(SharedMemory memory, ControlBlock* control, uint8_t* slots, bool owner)
        : m_memory(std::move(memory)), m_control(control), m_slots(slots), m_owner(owner) {}

MemBuff::~MemBuff() {
    if (m_owner) {
        sem_destroy(&m_control->sem_empty);
        sem_destroy(&m_control->sem_full);
    }
}

MemBuff MemBuff::create(uint32_t payload_size, uint32_t slot_count) {
    SharedMemory memory = SharedMemory::create(REGION_BYTES(payload_size, slot_count));

    void* base = memory.data();
    auto* control = new (base) ControlBlock();
    control->payload_size = payload_size;
    control->slot_count = slot_count;

    sem_init(&control->sem_empty, 1, slot_count);
    sem_init(&control->sem_full, 1, 0);

    return MemBuff(std::move(memory), control, SLOTS_PTR(base), true);
}

MemBuff MemBuff::attach(const std::string& name) {
    SharedMemory memory = SharedMemory::attach(name);
    auto* control = static_cast<ControlBlock*>(memory.data());
    return MemBuff(std::move(memory), control, SLOTS_PTR(memory.data()), false);
}

Slot MemBuff::slot_at(uint32_t index) const {
    uint8_t* base = m_slots + static_cast<std::size_t>(index) * SLOT_BYTES(m_control->payload_size);

    Slot slot;
    slot.header = reinterpret_cast<PacketHeader*>(base);
    slot.payload = base + sizeof(PacketHeader);
    return slot;
}

Slot MemBuff::producer_acquire() {
    sem_wait(&m_control->sem_empty);

    uint32_t index = m_control->write_index++ % m_control->slot_count;
    return slot_at(index);
}

void MemBuff::producer_release() {
    sem_post(&m_control->sem_full);
}

Slot MemBuff::consumer_acquire() {
    sem_wait(&m_control->sem_full);

    uint32_t index = m_control->read_index++ % m_control->slot_count;
    return slot_at(index);
}

void MemBuff::consumer_release() {
    sem_post(&m_control->sem_empty);
}

}  // namespace ipc
