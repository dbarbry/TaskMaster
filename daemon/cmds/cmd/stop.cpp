#include "../../logger.hpp"
#include "../cmds.hpp"

std::string stopCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                        const std::map<std::string, ProgramConfig>            &programs) {
    Logger::info("Stop command logic");
    std::ostringstream response;

    if (!cmd.count("args") || cmd.at("args").empty()) {
        Logger::error("No program specified to stop.");
        response << "Error: No program specified to stop." << std::endl;
        return response.str();
    }

    std::string requestedProgram = cmd.at("args")[0];
    Logger::debug("Requested program to stop: " + requestedProgram);

    if (!isServiceRunning(requestedProgram)) {
        Logger::error(requestedProgram + ": not running");
        response << requestedProgram + ": not running" << std::endl;
        return response.str();
    }

    auto it = programs.find(requestedProgram);
    if (it == programs.end()) {
        Logger::error(requestedProgram + ": not found in configuration");
        response << requestedProgram + ": not found in configuration" << std::endl;
        return response.str();
    }

    const ProgramConfig &config      = it->second;
    std::string          signalName  = config.getStopsignalString();
    EStopsignal          signalValue = EStopsignal::TERM;

    signalValue = string_to_stopsignal(signalName);

    std::vector<pid_t> pidsToStop;
    {
        for (const auto &process : runningServices[requestedProgram].processes) {
            pidsToStop.push_back(process.pid);
        }
    }

    updateServiceState(requestedProgram, ProcessState::STOPPED);
    response << requestedProgram << ": stopping..." << std::endl;

    // Envoyer le signal à tous les processus du service
    for (pid_t pid : pidsToStop) {
        Logger::info(requestedProgram + ": sending signal " + signalName + " to PID " +
                     std::to_string(pid));

        if (kill(pid, to_raw_signal(signalValue)) != 0) {
            std::string errorMsg = requestedProgram + ": failed to send signal to PID " +
                                   std::to_string(pid) + ": " + strerror(errno);
            Logger::error(errorMsg);
            response << errorMsg << std::endl;
        }
    }
    int stoptime = config.getStopwaitsecs();
    Logger::info(requestedProgram + ": waiting up to " + std::to_string(stoptime) +
                 " seconds for processes to terminate");

    time_t start_time = time(nullptr);
    while (!pidsToStop.empty() && (time(nullptr) - start_time) < stoptime) {
        for (auto it = pidsToStop.begin(); it != pidsToStop.end();) {
            if (kill(*it, 0) != 0 && errno == ESRCH) {
                // Le processus n'existe plus
                it = pidsToStop.erase(it);
            } else {
                ++it;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!pidsToStop.empty()) {
        std::string forceKillMsg = requestedProgram + ": force killing " +
                                   std::to_string(pidsToStop.size()) + " remaining processes";
        Logger::info(forceKillMsg);
        response << forceKillMsg << std::endl;

        for (pid_t pid : pidsToStop) {
            if (kill(pid, SIGKILL) != 0) {
                std::string killErrorMsg = requestedProgram + ": failed to kill PID " +
                                           std::to_string(pid) + ": " + strerror(errno);
                Logger::error(killErrorMsg);
                response << killErrorMsg << std::endl;
            }
        }
    } else {
        std::string stoppedMsg = requestedProgram + ": stopped";
        Logger::info(stoppedMsg);
        response << stoppedMsg << std::endl;
    }

    return response.str();
}
