#include "../../logger.hpp"
#include "../cmds.hpp"

std::string restartCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                           const std::map<std::string, ProgramConfig>            &programs) {
    std::ostringstream response;

    if (!cmd.count("args") || cmd.at("args").empty()) {
        Logger::error("No program specified to restart.");
        return "Error: No program specified to restart.";
    }

    std::string programToRestart = cmd.at("args")[0];

    if (programs.find(programToRestart) == programs.end()) {
        Logger::error(programToRestart + ": ERROR (no such program)");
        return programToRestart + ": ERROR (no such program)";
    }

    Logger::info(programToRestart + ": restarting process");
    response << programToRestart + ": restarting process" << std::endl;

    std::string stopResp = stopCommand(cmd, programs);
    response << stopResp;

    int maxWaitMs = 1000 * 10;  // max 10 seconds
    int waitedMs  = 0;
    int interval  = 100;  // check every 100ms

    while (getServiceInstanceCount(programToRestart) > 0 && waitedMs < maxWaitMs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        waitedMs += interval;
    }

    std::string startResp = startCommand(cmd, programs);
    response << startResp << std::endl;

    Logger::info(programToRestart + ": restarted successfully");
    response << programToRestart + ": restarted successfully";

    return response.str();
}