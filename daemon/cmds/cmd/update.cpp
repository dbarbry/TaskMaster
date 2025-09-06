#include "../cmds.hpp"

std::string updateCommand(TaskmasterConfig& config) {
    std::ostringstream response;

    if (config.program_to_start.empty() && config.program_to_stop.empty()) {
        Logger::info("No changes to update.");
        response << "No changes to update." << std::endl;
        return response.str();
    }

    for (const auto& prog : config.program_to_stop) {
        std::map<std::string, std::vector<std::string>> stopCmd = {{"args", {prog}}};
        std::string stopResult = stopCommand(stopCmd, config.programs);
        response << stopResult;

        config.programs.erase(prog);
        Logger::info(prog + ": removed from configuration");
    }

    std::map<std::string, ProgramConfig> fresh_configs;
    std::vector<std::string>             files = utils::expand_glob(config.files);

    for (const auto& filepath : files) {
        std::map<std::string, ProgramConfig> parsed = parsing(filepath);
        fresh_configs.insert(parsed.begin(), parsed.end());
    }

    for (const auto& prog : config.program_to_start) {
        if (fresh_configs.find(prog) == fresh_configs.end()) {
            Logger::error(prog + ": config missing from files after reread");
            response << prog + ": ERROR (config not found)" << std::endl;
            continue;
        }

        config.programs[prog] = fresh_configs[prog];

        std::map<std::string, std::vector<std::string>> startCmd = {{"args", {prog}}};
        std::string startResult = startCommand(startCmd, config.programs);
        response << startResult;
    }

    config.program_to_start.clear();
    config.program_to_stop.clear();

    return response.str();
}
