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
        response << requestedProgram + ": not running";
        return response.str();
    }

    auto it = programs.find(requestedProgram);
    if (it == programs.end()) {
        Logger::error(requestedProgram + ": not found in configuration");
        response << requestedProgram + ": not found in configuration";
        return response.str();
    }

    const ProgramConfig &config      = it->second;
    std::string          signalName  = config.getStopsignalString();
    EStopsignal          signalValue = EStopsignal::TERM;

    signalValue = string_to_stopsignal(signalName);

    std::vector<pid_t> pidsToStop;

    for (const auto &process : runningServices[requestedProgram].processes) {
        if (process.state == ProcessState::RUNNING || process.state == ProcessState::STARTING ||
            process.state == ProcessState::RESTARTING) {
            pidsToStop.push_back(process.pid);
        }
    }

    if (pidsToStop.empty()) {
        Logger::info(requestedProgram + ": no active processes to stop");
        response << requestedProgram << ": no active processes to stop";
        updateServiceState(requestedProgram, ProcessState::STOPPED);
        return response.str();
    }

    response << requestedProgram << ": stopping..." << std::endl;

    // Envoyer le signal à tous les processus du service
    for (pid_t pid : pidsToStop) {
        // Si le processus n'existe déjà plus, inutile d'envoyer le signal
        if (kill(pid, 0) != 0 && errno == ESRCH) {
            Logger::info(requestedProgram + ": PID " + std::to_string(pid) + " already exited");
            continue;
        }

        Logger::info(requestedProgram + ": sending signal " + signalName + " to PID " +
                     std::to_string(pid));

        if (kill(pid, to_raw_signal(signalValue)) != 0) {
            if (errno == ESRCH) {
                Logger::info(requestedProgram + ": PID " + std::to_string(pid) + " already exited");
            } else {
                std::string errorMsg = requestedProgram + ": failed to send signal to PID " +
                                       std::to_string(pid) + ": " + strerror(errno);
                Logger::error(errorMsg);
                response << errorMsg << std::endl;
            }
        }
    }
    int stoptime = config.getStopwaitsecs();
    Logger::info(requestedProgram + ": waiting up to " + std::to_string(stoptime) +
                 " seconds for processes to terminate");

    time_t start_time = time(nullptr);
    while (!pidsToStop.empty() && (time(nullptr) - start_time) < stoptime) {
        for (auto it = pidsToStop.begin(); it != pidsToStop.end();) {
            if (kill(*it, 0) != 0 && errno == ESRCH) {
                auto &processes = runningServices[requestedProgram].processes;
                processes.erase(
                    std::remove_if(processes.begin(), processes.end(),
                                   [pid = *it](const ProcessInfo &p) { return p.pid == pid; }),
                    processes.end());
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
            if (kill(pid, SIGKILL) != 0 && errno != ESRCH) {
                std::string killErrorMsg = requestedProgram + ": failed to kill PID " +
                                           std::to_string(pid) + ": " + strerror(errno);
                Logger::error(killErrorMsg);
                response << killErrorMsg << std::endl;
            }
            // Always remove from memory
            auto &processes = runningServices[requestedProgram].processes;
            processes.erase(std::remove_if(processes.begin(), processes.end(),
                                           [pid](const ProcessInfo &p) { return p.pid == pid; }),
                            processes.end());
        }
    }
    std::string stoppedMsg = requestedProgram + ": stopped";
    Logger::info(stoppedMsg);
    response << stoppedMsg;
    updateServiceState(requestedProgram, ProcessState::STOPPED);

    return response.str();
}
