#include <unistd.h>

#include <csignal>
#include <iostream>

void signal_handler(int signum) {
    switch (signum) {
        case SIGTSTP:
            std::cout << std::endl << "Ctrl+Z is blocked in this shell" << std::endl;
            break;
        case SIGTERM:
            std::cout << "Leaving..." << std::endl;
            exit(0);
        // case SIGQUIT:
        //     std::cout << "\n[CLIENT] Detaching... Press Enter to return to the shell." << std::endl;
        //     return;
        default:
            std::cout << "Stop trying to break this shell" << std::endl;
            break;
    }
}

void setup_signal_handlers(void) {
    struct sigaction sa;

    sa.sa_handler = signal_handler;
    sa.sa_flags   = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGTSTP, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
    // sigaction(SIGQUIT, &sa, 0);

    struct sigaction ignore_sa;
    ignore_sa.sa_handler = SIG_IGN;
    sigemptyset(&ignore_sa.sa_mask);
    ignore_sa.sa_flags = 0;
    sigaction(SIGQUIT, &ignore_sa, 0);
}
