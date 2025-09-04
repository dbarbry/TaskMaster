#include "../../logger.hpp"
#include "../cmds.hpp"

extern std::map<std::string, ServiceInfo> runningServices;
extern std::mutex                         serviceMutex;

std::string stopCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                        const std::map<std::string, ProgramConfig>            &programs) {
    Logger::info("Stop command logic");
    std::ostringstream response;

    if (!cmd.count("args") || cmd.at("args").empty()) {
        Logger::error("No program specified to stop.");
        return "Error: No program specified to stop.";
    }

    std::string requestedProgram = cmd.at("args")[0];
    Logger::debug("Requested program to stop: " + requestedProgram);

    auto it = programs.find(requestedProgram);
    if (it == programs.end()) {
        Logger::error(requestedProgram + ": not found in configuration");
        return requestedProgram + ": not found in configuration";
    }

    if (!isServiceRunning(requestedProgram)) {
        Logger::warn(requestedProgram + ": not running");
        return requestedProgram + ": not running";
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

    {
        std::lock_guard<std::mutex> lock(serviceMutex);
        for (const auto &process : runningServices[requestedProgram].processes) {
            if (process.state == ProcessState::RUNNING || process.state == ProcessState::STARTING ||
                process.state == ProcessState::RESTARTING) {
                pidsToStop.push_back(process.pid);
            }
        }
        updateServiceState(requestedProgram, ProcessState::STOPPED);
    }

    if (pidsToStop.empty()) {
        Logger::info(requestedProgram + ": no active processes to stop");
        updateServiceState(requestedProgram, ProcessState::STOPPED);
        return requestedProgram + ": no active processes to stop";
    }

    response << requestedProgram << ": stopping..." << std::endl;

    for (pid_t pid : pidsToStop) {
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
                {
                    std::lock_guard<std::mutex> lock(serviceMutex);
                    auto &processes = runningServices[requestedProgram].processes;
                    processes.erase(
                        std::remove_if(processes.begin(), processes.end(),
                                       [pid = *it](const ProcessInfo &p) { return p.pid == pid; }),
                        processes.end());
                }
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
        Logger::warn(forceKillMsg);
        response << forceKillMsg << std::endl;

        for (pid_t pid : pidsToStop) {
            if (kill(pid, SIGKILL) != 0 && errno != ESRCH) {
                std::string killErrorMsg = requestedProgram + ": failed to kill PID " +
                                           std::to_string(pid) + ": " + strerror(errno);
                Logger::error(killErrorMsg);
                response << killErrorMsg << "\n";
            }
            {
                std::lock_guard<std::mutex> lock(serviceMutex);
                auto                       &processes = runningServices[requestedProgram].processes;
                processes.erase(
                    std::remove_if(processes.begin(), processes.end(),
                                   [pid](const ProcessInfo &p) { return p.pid == pid; }),
                    processes.end());
            }
        }
    }
    updateServiceState(requestedProgram, ProcessState::STOPPED);
    Logger::info(requestedProgram + ": stopped");
    response << requestedProgram << ": stopped";

    return response.str();
}
