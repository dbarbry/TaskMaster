#include "../../logger.hpp"
#include "../cmds.hpp"

/**
 * @brief Helper to join vector<string> into a single string with separator
 */
std::string joinStrings(const std::vector<std::string>& vec, const std::string& sep) {
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i + 1 < vec.size()) {
            oss << sep;
        }
    }
    return oss.str();
}

/**
 * @brief Compares two program configurations to detect changes that requires a restart
 */
std::optional<std::pair<std::string, bool>> isProgramConfigurationChanged(
    const std::string& name, const ProgramConfig& old_config, const ProgramConfig& new_config) {
    Logger::debug(name + ": old workingdir=" + std::to_string(old_config.getNumprocs()) +
                  ", new workingdir=" + std::to_string(new_config.getNumprocs()));
    // require restart
    if (old_config.getCommand() != new_config.getCommand() ||
        old_config.getNumprocs() != new_config.getNumprocs() ||
        old_config.getWorkingDir() != new_config.getWorkingDir() ||
        old_config.getUmask() != new_config.getUmask() ||
        old_config.getEnvironment() != new_config.getEnvironment()) {
        Logger::debug(name + ": change detected (restart required)");
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
        Logger::debug(name + ": change detected (no restart required)");
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
    std::ostringstream       response;
    TaskmasterConfig         new_config;
    std::vector<std::string> updated;
    std::vector<std::string> restarted;

    try {
        new_config = parse_taskmaster_conf(config.conf_path);

        std::map<std::string, ProgramConfig> fresh_configs;
        std::vector<std::string>             files = utils::expand_glob(new_config.files);

        for (const auto& filepath : files) {
            std::map<std::string, ProgramConfig> parsed = parsing(filepath);
            fresh_configs.insert(parsed.begin(), parsed.end());
        }

        new_config.programs = fresh_configs;

        Logger::info("Configuration successfully reloaded from file: " + config.conf_path);
    } catch (const std::exception& e) {
        std::string error = "Failed to reload configuration: ";
        response << error << e.what();
        Logger::error(error + e.what());
        return response.str();
    }

    for (const auto& [name, old_prog] : programs) {
        Logger::info("Checking program: " + name);
        Logger::info("New config contains programs:");
        for (const auto& [n, _] : new_config.programs) {
            Logger::info(" - " + n);
        }

        auto it = new_config.programs.find(name);
        if (it == new_config.programs.end()) {
            Logger::info("New program found here ====:");
            // reload ignores new/removed programs
            continue;
        }

        auto result = isProgramConfigurationChanged(name, old_prog, it->second);
        if (result.has_value()) {
            bool needs_restart = result->second;

            programs[name] = it->second;
            updated.push_back(name);

            if (needs_restart) {
                std::map<std::string, std::vector<std::string>> restart_cmd = {{"args", {name}}};
                std::string restart_response = restartCommand(restart_cmd, programs);
                response << restart_response;
                Logger::info("Reload: restarted program " + name);
                restarted.push_back(name);
            }
        }
    }

    if (updated.empty()) {
        response << "No configuration changes detected. Nothing restarted.";
    } else {
        response << std::endl << "Reloaded configuration." << std::endl;
        if (!updated.empty()) {
            response << "Updated: " << joinStrings(updated, ", ") << std::endl;
        }
        if (!restarted.empty()) {
            response << "Restarted: " << joinStrings(restarted, ", ") << std::endl;
        }
    }

    return response.str();
}
