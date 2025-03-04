#include <iomanip>
#include <iostream>

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
    std::cout << "=== Services Status ===" << std::endl;

    if (runningServices.empty()) {
        std::cout << "No services are currently registered." << std::endl;
        return;
    }

    for (const auto& [name, _] : runningServices) {
        cleanupOldProcesses(name);
    }

    std::cout << std::left << std::setw(35) << "NAME" << std::setw(12) << "STATUS" << std::setw(30)
              << "INFO" << std::endl;
    std::cout << std::string(77, '-') << std::endl;

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

        std::cout << std::left << std::setw(35) << name;

        if (running > 0) {
            std::cout << std::setw(12) << "RUNNING";
            std::cout << running << " instance(s), ";
            std::cout << stopped << " stopped, " << failed << " failed";
        } else if (failed > 0) {
            std::cout << std::setw(12) << "FATAL";
            std::cout << "all processes have failed";
        } else {
            std::cout << std::setw(12) << "STOPPED";
            std::cout << "no processes running";
        }
        std::cout << std::endl;

        for (const auto& proc : info.processes) {
            if (proc.state == ProcessState::RUNNING) {
                std::cout << "  └─ pid " << proc.pid << ", running";
                std::cout << std::endl;
            }
        }
    }
}