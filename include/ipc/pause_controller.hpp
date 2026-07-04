#pragma once

#include <atomic>
#include <thread>

namespace ipc {

class PauseController {
public:
    PauseController() = default;
    ~PauseController();

    PauseController(const PauseController&) = delete;
    PauseController& operator=(const PauseController&) = delete;

    void start();
    void stop();
    void process();
    bool is_paused() const { return m_paused.load(); }

private:
    void keyboard_loop();

    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_keyboard_running{false};
    std::thread m_keyboard_thread;
};

}  // namespace ipc
