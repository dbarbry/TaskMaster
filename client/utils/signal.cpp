#include "utils.hpp"

static void clean_exit(int code) {
    std::cout << "Leaving..." << std::endl;
    exit(code);
}

void signal_handler(int signum) {
    switch (signum) {
        case SIGINT:   // Ctrl-C
        case SIGTERM:  // kill
        case SIGQUIT:  // Ctrl-
            clean_exit(0);
            break;
        case SIGTSTP:  // Ctrl-Z
            std::cout << std::endl << "'fg' to come back to taskmaster." << std::endl;
            break;
        case SIGPIPE:  // write on closed socket
            std::cerr << "Connection lost (Server closed)." << std::endl;
            clean_exit(1);
            break;
        default:
            std::cout << std::endl << "Unhandled signal." << std::endl;
            break;
    }
}

void setup_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags   = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    // Exit/terminate signals
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGQUIT, &sa, nullptr);

    // Block suspension
    sigaction(SIGTSTP, &sa, nullptr);

    // Handle broken pipe
    sigaction(SIGPIPE, &sa, nullptr);
}
