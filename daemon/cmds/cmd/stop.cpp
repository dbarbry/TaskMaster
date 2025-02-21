#include "../cmds.hpp"

void stopCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                 const std::map<std::string, ProgramConfig>            &programs) {
    std::cout << "Closing command logic" << std::endl;

    if (!cmd.count("command") || cmd.at("command").empty() || cmd.at("command")[0] != "stop") {
        std::cerr << "[DEBUG] Command is not 'close'" << std::endl;
        return;
    }

    if (!cmd.count("args") || cmd.at("args").empty()) {
        std::cerr << "No program specified to close." << std::endl;
        return;
    }

    std::string requestedProgram = cmd.at("args")[0];
    std::cout << "[DEBUG] Requested program to close: " << requestedProgram << std::endl;

    auto it = programs.find(requestedProgram);
    if (it == programs.end()) {
        std::cerr << "Program " << requestedProgram << " not found in configuration." << std::endl;
        return;
    }
    const ProgramConfig &configFromFile = it->second;

    if (!isAlreadyRunning(configFromFile.getCmd())) {
        std::cerr << "The program " << requestedProgram << " is not running!" << std::endl;
        return;
    }

    std::string programName = basename(const_cast<char *>(configFromFile.getCmd().c_str()));
    std::cout << "Killing program " << programName << "..." << std::endl;

    std::string killCommand = "pkill -f " + programName;
    int         killResult  = system(killCommand.c_str());
    if (killResult == 0) {
        std::cout << "Program " << requestedProgram << " closed successfully." << std::endl;
    } else {
        std::cerr << "Failed to close program " << requestedProgram << "." << std::endl;
    }
}
