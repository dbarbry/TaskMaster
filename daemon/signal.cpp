
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
    exit(EXIT_SUCCESS);
}

void register_signal_handlers(void) {
    signal(SIGINT, shutdown_taskmaster);
    signal(SIGTERM, shutdown_taskmaster);
    signal(SIGHUP, shutdown_taskmaster);
    signal(SIGQUIT, shutdown_taskmaster);
}
