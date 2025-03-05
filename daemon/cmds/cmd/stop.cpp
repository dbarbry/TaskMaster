#include <signal.h>

#include <iostream>

#include "../cmds.hpp"
#include "../service_state.hpp"

void stopCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                 const std::map<std::string, ProgramConfig>            &programs) {
    std::cout << "Stop command logic" << std::endl;

    if (!cmd.count("args") || cmd.at("args").empty()) {
        std::cerr << "No program specified to stop." << std::endl;
        return;
    }

    std::string requestedProgram = cmd.at("args")[0];
    std::cout << "[DEBUG] Requested program to stop: " << requestedProgram << std::endl;

    if (!isServiceRunning(requestedProgram)) {
        std::cerr << "Program " << requestedProgram << " is not running." << std::endl;
        return;
    }

    auto it = programs.find(requestedProgram);
    if (it == programs.end()) {
        std::cerr << "Program " << requestedProgram << " not found in configuration." << std::endl;
        return;
    }

    const ProgramConfig &config      = it->second;
    std::string          signalName  = config.getStopsignal();
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
        std::cout << "Sending signal " << signalName << " to PID " << pid << std::endl;
        if (kill(pid, signalValue) != 0) {
            std::cerr << "Failed to send signal to PID " << pid << ": " << strerror(errno)
                      << std::endl;
        }
    }

    int stoptime = config.getStoptime();
    std::cout << "Waiting up to " << stoptime << " seconds for processes to terminate..."
              << std::endl;

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
        std::cout << "Force killing " << pidsToStop.size() << " remaining processes" << std::endl;
        for (pid_t pid : pidsToStop) {
            if (kill(pid, SIGKILL) != 0) {
                std::cerr << "Failed to kill PID " << pid << ": " << strerror(errno) << std::endl;
            }
        }
    }
}