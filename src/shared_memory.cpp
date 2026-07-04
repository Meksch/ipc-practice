#include "ipc/shared_memory.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <utility>

namespace ipc {

SharedMemory::SharedMemory(int fd, void* addr, std::size_t size, std::string name, bool owner)
    : m_fd(fd), m_addr(addr), m_size(size), m_name(std::move(name)), m_owner(owner) {}

SharedMemory::SharedMemory(SharedMemory&& other) noexcept
        : m_fd(other.m_fd),
          m_addr(other.m_addr),
          m_size(other.m_size),
          m_name(std::move(other.m_name)),
          m_owner(other.m_owner) {
    other.m_fd = -1;
    other.m_addr = nullptr;
    other.m_size = 0;
    other.m_owner = false;
}

SharedMemory& SharedMemory::operator=(SharedMemory&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (m_addr != nullptr) {
        munmap(m_addr, m_size);
    }
    if (m_fd >= 0) {
        close(m_fd);
    }
    if (m_owner) {
        shm_unlink(m_name.c_str());
    }

    m_fd = other.m_fd;
    m_addr = other.m_addr;
    m_size = other.m_size;
    m_name = std::move(other.m_name);
    m_owner = other.m_owner;

    other.m_fd = -1;
    other.m_addr = nullptr;
    other.m_size = 0;
    other.m_owner = false;

    return *this;
}

SharedMemory SharedMemory::create(std::size_t size) {
    const std::string name = "/ipc-practice-" + std::to_string(getpid());
    const int fd = shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666);

    const int result = ftruncate(fd, static_cast<off_t>(size));
    void* addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (addr == MAP_FAILED || result != 0) {
        close(fd);
        shm_unlink(name.c_str());
    }

    return SharedMemory(fd, addr, size, std::move(name), true);
}

SharedMemory SharedMemory::attach(const std::string& name) {
    const int fd = shm_open(name.c_str(), O_RDWR, 0);

    struct stat st {};
    fstat(fd, &st);
    const auto size = static_cast<std::size_t>(st.st_size);

    void* addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
    }

    return SharedMemory(fd, addr, size, name, false);
}

SharedMemory::~SharedMemory() {
    if (m_addr != nullptr && m_addr != MAP_FAILED) {
        munmap(m_addr, m_size);
    }
    if (m_fd >= 0) {
        close(m_fd);
    }
    if (m_owner) {
        shm_unlink(m_name.c_str());
    }
}

}  // namespace ipc
