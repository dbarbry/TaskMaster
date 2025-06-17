#include "../cmds.hpp"

std::string rereadCommand(TaskmasterConfig& config) {
    std::ostringstream response;

    std::vector<std::string> current_programs = config.get_current_program_names(config);
    std::vector<std::string> new_programs     = config.get_programs_names_in_files(config.files);

    std::set<std::string> current_set(current_programs.begin(), current_programs.end());
    std::set<std::string> new_set(new_programs.begin(), new_programs.end());

    config.program_to_start.clear();
    config.program_to_stop.clear();

    for (const auto& prog : new_set) {
        if (!current_set.count(prog)) {
            config.program_to_start.push_back(prog);
        }
    }

    for (const auto& prog : current_set) {
        if (!new_set.count(prog)) {
            config.program_to_stop.push_back(prog);
        }
    }

    if (!config.program_to_start.empty()) {
        for (const auto& prog : config.program_to_start) {
            Logger::info(prog + ": available");
            response << prog + ": available" << std::endl;
        }
    }

    if (!config.program_to_stop.empty()) {
        for (const auto& prog : config.program_to_stop) {
            Logger::info(prog + ": disappeared");
            response << prog + ": disappeared" << std::endl;
        }
    }

    if (config.program_to_start.empty() && config.program_to_stop.empty()) {
        Logger::info("No config updates to processes");
        response << "No config updates to processes" << std::endl;
    }

    return response.str();
}
