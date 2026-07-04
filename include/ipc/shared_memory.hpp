#pragma once

#include <cstddef>
#include <string>

namespace ipc {

class SharedMemory {
public:
    SharedMemory() = default;

    //move
    SharedMemory(SharedMemory&& other) noexcept;
    SharedMemory& operator=(SharedMemory&& other) noexcept;

    //copy deleted
    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;
    ~SharedMemory();

    static SharedMemory create(std::size_t size);          // for producer
    static SharedMemory attach(const std::string& name);   // for consumer

    void* data() const { return m_addr; }
    std::size_t size() const { return m_size; }
    const std::string& name() const { return m_name; }
    bool is_owner() const { return m_owner; }

    void unlink();

private:
    SharedMemory(int fd, void* addr, std::size_t size, std::string name, bool owner);

    int m_fd{-1};            // file descriptor
    void* m_addr{nullptr};   // address, returned by mmap
    std::size_t m_size{0};   // shared memory size
    std::string m_name;      // memory name
    bool m_owner{false};     // is memory owned by this process
};

}  // namespace ipc
