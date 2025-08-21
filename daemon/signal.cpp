
#include "logger.hpp"
#include "main.hpp"

static TaskmasterConfig* g_config = nullptr;

void cleanup(std::optional<TaskmasterConfig*> config = std::nullopt) {
    TaskmasterConfig* cfg = config.value_or(g_config);

    if (cfg) {
        if (!cfg->pidfile.empty()) {
            unlink(cfg->pidfile.c_str());
            Logger::info("Removed PID file: " + cfg->pidfile);
        }
        if (!cfg->file.empty()) {
            unlink(cfg->file.c_str());
            Logger::info("Removed socket file: " + cfg->file);
        }
    }
    fflush(NULL);
}

void shutdown_taskmaster(int signum) {
    std::cout << std::endl;
    Logger::info("Received signal " + std::to_string(signum) + ", shutting down gracefully...");
    cleanup();
    Logger::info("Cleanup complete. Exiting.");
    _exit(EXIT_SUCCESS);
}

void register_signal_handlers(void) {
    struct sigaction sa {};
    sa.sa_handler = shutdown_taskmaster;

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGHUP);
    sigaddset(&sa.sa_mask, SIGQUIT);

    sa.sa_flags = SA_RESTART;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGHUP, &sa, nullptr);
    sigaction(SIGQUIT, &sa, nullptr);

    // Ignore SIGCHLD (no zombies)
    struct sigaction sa_chld {};
    sa_chld.sa_handler = SIG_IGN;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sa_chld, nullptr);

    // Ignore SIGPIPE (if pipe issue won't crash)
    struct sigaction sa_pipe {};
    sa_pipe.sa_handler = SIG_IGN;
    sigemptyset(&sa_pipe.sa_mask);
    sa_pipe.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &sa_pipe, nullptr);
}
