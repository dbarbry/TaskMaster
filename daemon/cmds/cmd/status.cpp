#include <iomanip>
#include <iostream>
#include <sstream>

#include "../../logger.hpp"
#include "../cmds.hpp"
#include "../service_state.hpp"

std::string getStateString(ProcessState state) {
    switch (state) {
        case ProcessState::STARTING:
            return "STARTING";
        case ProcessState::RUNNING:
            return "RUNNING";
        case ProcessState::RESTARTING:
            return "RESTARTING";
        case ProcessState::STOPPED:
            return "STOPPED";
        case ProcessState::FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
    }
}

void statusCommand(const std::map<std::string, std::vector<std::string>>& cmd,
                   const std::map<std::string, ProgramConfig>&            programs) {
    
    if (runningServices.empty()) {
        Logger::info("No services are currently registered.");
        return;
    }

    for (const auto& [name, _] : runningServices) {
        cleanupOldProcesses(name);
    }

    std::stringstream header;
    header << std::left << std::setw(35) << "NAME" << std::setw(12) << "STATUS" << std::setw(30) << "INFO";
    Logger::info(header.str());
    Logger::info(std::string(77, '-'));

    for (const auto& [name, info] : runningServices) {
        int running = 0, stopped = 0, failed = 0;

        for (const auto& proc : info.processes) {
            if (proc.state == ProcessState::RUNNING)
                running++;
            else if (proc.state == ProcessState::STOPPED)
                stopped++;
            else if (proc.state == ProcessState::FATAL)
                failed++;
        }

        std::stringstream line;
        line << std::left << std::setw(35) << name;

        if (running > 0) {
            line << std::setw(12) << "RUNNING";
            line << running << " instance(s), ";
            line << stopped << " stopped, " << failed << " failed";
        } else if (failed > 0) {
            line << std::setw(12) << "FATAL";
            line << "all processes have failed";
        } else {
            line << std::setw(12) << "STOPPED";
            line << "no processes running";
        }
        Logger::info(line.str());

        for (const auto& proc : info.processes) {
            if (proc.state == ProcessState::RUNNING) {
                Logger::info("  └─ pid " + std::to_string(proc.pid) + ", running");
            }
        }
    }
}