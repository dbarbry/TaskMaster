#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>

#include "../../logger.hpp"
#include "../cmds.hpp"

void restartCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                    const std::map<std::string, ProgramConfig>            &programs) {
    if (!cmd.count("args") || cmd.at("args").empty()) {
        Logger::error("No program specified to restart.");
        return;
    }

    std::string programToRestart = cmd.at("args")[0];

    if (programs.find(programToRestart) == programs.end()) {
        Logger::error(programToRestart + ": ERROR (no such program)");
        return;
    }

    std::map<std::string, std::vector<std::string>> cmdCopy = cmd;

    Logger::info(programToRestart + ": restarting process");

    stopCommand(cmdCopy, programs);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    Logger::info(programToRestart + ": stopped");
    Logger::info(programToRestart + ": starting");

    startCommand(cmdCopy, programs);

    Logger::info(programToRestart + ": started");
}