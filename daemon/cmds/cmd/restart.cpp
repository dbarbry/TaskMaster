#include "../../logger.hpp"
#include "../cmds.hpp"

std::string restartCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                           const std::map<std::string, ProgramConfig>            &programs) {
    std::ostringstream response;

    if (!cmd.count("args") || cmd.at("args").empty()) {
        Logger::error("No program specified to restart.");
        response << "Error: No program specified to restart.";
        return response.str();
    }

    std::string programToRestart = cmd.at("args")[0];

    if (programs.find(programToRestart) == programs.end()) {
        Logger::error(programToRestart + ": ERROR (no such program)");
        response << programToRestart + ": ERROR (no such program)";
        return response.str();
    }

    std::map<std::string, std::vector<std::string>> cmdCopy = cmd;

    Logger::info(programToRestart + ": restarting process");
    response << programToRestart + ": restarting process" << std::endl;

    // Récupération de la réponse de stopCommand
    std::string stopResponse = stopCommand(cmdCopy, programs);
    response << stopResponse;

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    Logger::info(programToRestart + ": stopped");
    Logger::info(programToRestart + ": starting");
    response << programToRestart + ": stopped" << std::endl;
    response << programToRestart + ": starting" << std::endl;

    // Récupération de la réponse de startCommand
    std::string startResponse = startCommand(cmdCopy, programs);
    response << startResponse;

    Logger::info(programToRestart + ": started");
    response << programToRestart + ": started";

    return response.str();
}
