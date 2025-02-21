#include "../cmds.hpp"

void startCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                  const std::map<std::string, ProgramConfig>            &programs) {
    if (!cmd.count("command") || cmd.at("command").empty() || cmd.at("command")[0] != "start") {
        std::cerr << "[DEBUG] Command is not 'start'" << std::endl;
        return;
    }

    if (!cmd.count("args") || cmd.at("args").empty()) {
        std::cerr << "No program specified to start." << std::endl;
        return;
    }

    std::string requestedProgram = cmd.at("args")[0];
    std::cout << "[DEBUG] Requested program: " << requestedProgram << std::endl;

    auto it = programs.find(requestedProgram);
    if (it == programs.end()) {
        std::cerr << "Program " << requestedProgram << " not found in configuration." << std::endl;
        return;
    }

    std::map<std::string, ProgramConfig> toLaunch;
    toLaunch[requestedProgram]          = it->second;
    const ProgramConfig &configFromFile = it->second;

    if (isAlreadyRunning(configFromFile.getCmd())) {
        std::cerr << "The program " << requestedProgram << " is already running!" << std::endl;
        return;
    }

    exec_programs(toLaunch);
}
