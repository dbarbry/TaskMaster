#include "incs/start_command.hpp"
#include "incs/execution.hpp"
#include <iostream>
#include <map>
#include <vector>
#include <string>


bool isAlreadyRunning(const std::string &programPath) {
    std::string programName = basename(const_cast<char*>(programPath.c_str())); 
    std::cout << "Checking if " << programName << " is already running" << std::endl;
    std::string command = "pgrep -f " + programName + " > /dev/null";
    int result = system(command.c_str());
    return (result == 0);
}

void startCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                  const std::map<std::string, ProgramConfig> &programs)
{
    std::cout << "Starting command logic" << std::endl;

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
    toLaunch[requestedProgram] = it->second;

    //Make verification here to check if the program is already running

        const ProgramConfig &configFromFile = it->second;

    if (isAlreadyRunning(configFromFile.getCmd())) {
        std::cerr << "The program " << requestedProgram << " is already running!" << std::endl;
        return;
    }

    exec_programs(toLaunch);
}
