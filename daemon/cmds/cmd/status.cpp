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

std::string statusCommand(const std::map<std::string, std::vector<std::string>>& cmd,
                          const std::map<std::string, ProgramConfig>&            programs) {
    std::ostringstream response;

    if (runningServices.empty()) {
        Logger::info("No services are currently registered.");
        response << "No services are currently registered." << std::endl;
        return response.str();
    }

    for (const auto& [name, _] : runningServices) {
        cleanupOldProcesses(name);
    }

    std::stringstream header;
    header << std::left << std::setw(35) << "NAME" << std::setw(12) << "STATUS" << std::setw(30)
           << "INFO";

    std::string headerStr = header.str();
    std::string separator = std::string(77, '-');

    Logger::info(headerStr);
    Logger::info(separator);

    response << headerStr << std::endl;
    response << separator << std::endl;

    for (const auto& [name, info] : runningServices) {
        int running = 0, starting = 0, restarting = 0, stopped = 0, failed = 0;

        for (const auto& proc : info.processes) {
            switch (proc.state) {
                case ProcessState::RUNNING:
                    running++;
                    break;
                case ProcessState::STARTING:
                    starting++;
                    break;
                case ProcessState::RESTARTING:
                    restarting++;
                    break;
                case ProcessState::STOPPED:
                    stopped++;
                    break;
                case ProcessState::FATAL:
                    failed++;
                    break;
                default:
                    break;
            }
        }

        std::stringstream line;
        line << std::left << std::setw(35) << name;

        if (running > 0 || starting > 0 || restarting > 0) {
            std::string statusStr = running > 0      ? "RUNNING"
                                    : restarting > 0 ? "RESTARTING"
                                                     : "STARTING";
            line << std::setw(12) << statusStr;
            line << running << " running, " << starting << " starting, " << restarting
                 << " restarting, " << stopped << " stopped, " << failed << " failed";
        } else if (failed > 0) {
            line << std::setw(12) << "FATAL";
            line << "all processes have failed";
        } else {
            line << std::setw(12) << "STOPPED";
            line << "no processes running";
        }

        std::string lineStr = line.str();
        Logger::info(lineStr);
        response << lineStr << std::endl;

        for (const auto& proc : info.processes) {
            std::string stateStr = getStateString(proc.state);
            std::string procInfo = "  └─ pid " + std::to_string(proc.pid) + ", " + stateStr;
            Logger::info(procInfo);
            response << procInfo << std::endl;
        }
    }

    return response.str();
}
