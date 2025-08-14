#include "main.hpp"

#include "./launch/config_global.hpp"
#include "logger.hpp"

static std::string g_config_file_path;

const std::string &getConfigPath() {
    return g_config_file_path;
}

bool is_conf_file(std::string filename) {
    std::ifstream file(filename);

    if (filename.size() < 5 || filename.substr(filename.size() - 5) != ".conf") {
        Logger::error("Must be a .conf extension file");
        return false;
    }
    if (!std::filesystem::exists(filename)) {
        Logger::error(filename + " file doesn't exist");
        return false;
    }
    if (!file.is_open()) {
        Logger::error("Can't open: " + filename);
        return false;
    }

    return true;
}

int main(int ac, char **av) {
    TaskmasterConfig config;

    if (ac != 2 || !is_conf_file(av[1])) {
        Logger::error("Usage: ./daemon.out <file.conf>");
        Logger::error("Or   :  make server <file.conf>");
        return 1;
    }
    Logger::info(".conf : " + std::string(av[1]));

    try {
        config = parse_taskmaster_conf(av[1]);
        Logger::info("Configuration file parsed successfully.");
    } catch (const std::exception &e) {
        Logger::error("Error parsing config file: " + std::string(e.what()));
        return 1;
    }

    // first load after config is done
    try {
        rereadCommand(config);
        updateCommand(config);
        Logger::info("Included config files processed.");
    } catch (const std::exception &e) {
        Logger::error("Error during reread/update: " + std::string(e.what()));
        return 1;
    }

    log_config(config.programs);
    std::thread program_thread(exec_programs, config.programs);

    if (!config.nodaemon)
        daemonize(config);
    else
        Logger::info("Logfile only works in daemonized mode.");

    apply_runtime_settings(config);
    run_server(config);

    program_thread.join();

    return 0;
}
