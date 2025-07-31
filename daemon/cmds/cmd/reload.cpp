#include "../../logger.hpp"
#include "../cmds.hpp"

/**
 * @brief Compares two program configurations to detect changes that requires a restart
 */
std::optional<std::pair<std::string, bool>> isProgramConfigurationChanged(
    const std::string& name, const ProgramConfig& old_config, const ProgramConfig& new_config) {
    // require restart
    if (old_config.getCommand() != new_config.getCommand() ||
        old_config.getNumprocs() != new_config.getNumprocs() ||
        old_config.getWorkingDir() != new_config.getWorkingDir() ||
        old_config.getUmask() != new_config.getUmask() ||
        old_config.getEnvironment() != new_config.getEnvironment()) {
        return std::make_pair(name, true);
    }

    // not require restart
    if (old_config.getStopsignalString() != new_config.getStopsignalString() ||
        old_config.getStopwaitsecs() != new_config.getStopwaitsecs() ||
        old_config.getStdoutLogfile() != new_config.getStdoutLogfile() ||
        old_config.getStderrLogfile() != new_config.getStderrLogfile() ||
        old_config.getAutorerestartString() != new_config.getAutorerestartString() ||
        old_config.getExitcodes() != new_config.getExitcodes() ||
        old_config.getStartretries() != new_config.getStartretries() ||
        old_config.getStartsecs() != new_config.getStartsecs()) {
        return std::make_pair(name, false);
    }

    return std::nullopt;
}

/**
 * @brief Updates the configuration by reloading the config file and applying changes
 */
std::string reloadCommand(const std::map<std::string, std::vector<std::string>>& cmd,
                          std::map<std::string, ProgramConfig>&                  programs,
                          const TaskmasterConfig&                                config) {
    std::ostringstream                        response;
    TaskmasterConfig                          new_config;
    std::vector<std::pair<std::string, bool>> changed_programs;
    std::map<std::string, bool>               running;

    try {
        new_config = parse_taskmaster_conf(config.conf_path);
        Logger::info("Configuration successfully reloaded from file.");
    } catch (const std::exception& e) {
        std::string error = "Failed to reload configuration: ";
        response << error << e.what();
        Logger::error(error + e.what());
        return response.str();
    }

    for (const auto& [name, old_config] : programs) {
        auto it = new_config.programs.find(name);
        if (it != new_config.programs.end()) {
            auto result = isProgramConfigurationChanged(name, old_config, it->second);
            if (result.has_value()) {
                changed_programs.push_back(result.value());
                running[name] = getServiceInstanceCount(name) > 0;
                Logger::info("Program '" + name + "' config changed. Restart required: " +
                             std::string(result->second ? "yes" : "no"));
            }
        }
    }

    for (const auto& [name, needs_restart] : changed_programs) {
        if (needs_restart && running[name]) {
            std::map<std::string, std::vector<std::string>> stop_cmd = {{"args", {name}}};
            response << stopCommand(stop_cmd, programs);
            Logger::info("Stopped program: " + name + " due to config change.");
        }
    }

    for (const auto& [name, _] : changed_programs) {
        programs[name] = new_config.programs[name];
        Logger::info("Updated configuration for program: " + name);
    }

    for (const auto& [name, needs_restart] : changed_programs) {
        if (needs_restart && running[name]) {
            std::map<std::string, std::vector<std::string>> start_cmd = {{"args", {name}}};
            response << startCommand(start_cmd, programs);
            Logger::info("Restarted updated program: " + name);
        }
    }

    if (changed_programs.empty()) {
        response << "No configuration changes detected. Nothing restarted.";
    } else {
        size_t restarted = std::count_if(changed_programs.begin(), changed_programs.end(),
                                         [](const auto& p) { return p.second; });
        size_t updated   = changed_programs.size();
        response << "Reloaded configuration. " << restarted << " program(s) restarted. "
                 << (updated - restarted) << " updated without restart.";
    }

    return response.str();
}
