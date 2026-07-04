#include "ipc/pause_controller.hpp"

#include <csignal>
#include <termios.h>
#include <unistd.h>

volatile std::sig_atomic_t g_signal_cmd = 0;

namespace ipc {

PauseController::~PauseController() { stop(); }

void PauseController::start() {
    std::signal(SIGUSR1, [](int) { g_signal_cmd = 1; });
    std::signal(SIGUSR2, [](int) { g_signal_cmd = 2; });

    if (isatty(STDIN_FILENO)) {
        m_keyboard_running.store(true);
        m_keyboard_thread = std::thread(&PauseController::keyboard_loop, this);
    }
}

void PauseController::stop() {
    m_keyboard_running.store(false);
    if (m_keyboard_thread.joinable()) {
        m_keyboard_thread.join();
    }
}

void PauseController::process() {
    const std::sig_atomic_t cmd = g_signal_cmd;
    if (cmd == 0) {
        return;
    }
    g_signal_cmd = 0;
    m_paused.store(cmd == 1);
}

void PauseController::keyboard_loop() {
    termios old_settings{};
    tcgetattr(STDIN_FILENO, &old_settings);

    termios raw = old_settings;
    raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));    // no press Enter | Typed are not printed
    raw.c_cc[VMIN] = 0;                                        // read immediately
    raw.c_cc[VTIME] = 1;                                       // 100 ms timeout
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    while (m_keyboard_running.load()) {
        char ch = 0;
        if (read(STDIN_FILENO, &ch, 1) == 1) {
            m_paused.store(!m_paused.load());
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
}

}  // namespace ipc
