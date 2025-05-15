#include <signal.h>

#include <iostream>
#include <string>

#include "../../logger.hpp"
#include "../cmds.hpp"
#include "../service_state.hpp"

void stopCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                 const std::map<std::string, ProgramConfig>            &programs) {
    Logger::info("Stop command logic");

    if (!cmd.count("args") || cmd.at("args").empty()) {
        Logger::error("No program specified to stop.");
        return;
    }

    std::string requestedProgram = cmd.at("args")[0];
    Logger::debug("Requested program to stop: " + requestedProgram);

    if (!isServiceRunning(requestedProgram)) {
        Logger::error(requestedProgram + ": not running");
        return;
    }

    auto it = programs.find(requestedProgram);
    if (it == programs.end()) {
        Logger::error(requestedProgram + ": not found in configuration");
        return;
    }

    const ProgramConfig &config      = it->second;
    std::string          signalName = config.getStopsignalString();
    int                  signalValue = SIGTERM;

    // Convertir le nom du signal en valeur numérique
    if (signalName == "TERM")
        signalValue = SIGTERM;
    else if (signalName == "INT")
        signalValue = SIGINT;
    else if (signalName == "QUIT")
        signalValue = SIGQUIT;
    else if (signalName == "KILL")
        signalValue = SIGKILL;
    else if (signalName == "HUP")
        signalValue = SIGHUP;
    else if (signalName == "USR1")
        signalValue = SIGUSR1;
    else if (signalName == "USR2")
        signalValue = SIGUSR2;

    std::vector<pid_t> pidsToStop;
    {
        for (const auto &process : runningServices[requestedProgram].processes) {
            pidsToStop.push_back(process.pid);
        }
    }

    updateServiceState(requestedProgram, ProcessState::STOPPED);

    // Envoyer le signal à tous les processus du service
    for (pid_t pid : pidsToStop) {
        Logger::info(requestedProgram + ": sending signal " + signalName + " to PID " +
                     std::to_string(pid));
        if (kill(pid, signalValue) != 0) {
            Logger::error(requestedProgram + ": failed to send signal to PID " +
                          std::to_string(pid) + ": " + strerror(errno));
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
        Logger::info(requestedProgram + ": force killing " + std::to_string(pidsToStop.size()) +
                     " remaining processes");
        for (pid_t pid : pidsToStop) {
            if (kill(pid, SIGKILL) != 0) {
                Logger::error(requestedProgram + ": failed to kill PID " + std::to_string(pid) +
                              ": " + strerror(errno));
            }
        }
    } else {
        Logger::info(requestedProgram + ": stopped");
    }
}