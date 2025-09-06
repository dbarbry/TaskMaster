#include "service_state.hpp"

#include "../launch/config_program.hpp"
#include "../logger.hpp"

std::map<std::string, ServiceInfo> runningServices;
std::mutex                         serviceMutex;
bool                               monitoringStarted = false;

bool addServicePid(const std::string& name, pid_t pid, int startsecs) {
    auto& processes = runningServices[name].processes;
    for (const auto& process : processes) {
        if (process.pid == pid) {
            return false;
        }
    }

    ProcessInfo newProcess;
    newProcess.pid       = pid;
    newProcess.state     = ProcessState::STARTING;
    newProcess.retries   = 0;
    newProcess.startTime = std::time(nullptr);
    newProcess.exitCode  = 0;

    processes.push_back(newProcess);

    if (runningServices[name].name.empty()) {
        runningServices[name].name      = name;
        runningServices[name].startsecs = startsecs;
    }

    Logger::info(name + ": added process " + std::to_string(pid));
    return true;
}

bool isServiceRunning(const std::string& name) {
    if (runningServices.count(name) == 0) return false;
    const auto& processes = runningServices[name].processes;
    for (const auto& p : processes) {
        if (p.state == ProcessState::RUNNING || p.state == ProcessState::STARTING ||
            p.state == ProcessState::RESTARTING) {
            return true;
        }
    }
    return false;
}

void incrementRetries(const std::string& name, pid_t pid) {
    std::lock_guard<std::mutex> lock(serviceMutex);

    if (runningServices.count(name) > 0) {
        auto& processes = runningServices[name].processes;
        for (auto& process : processes) {
            if (process.pid == pid) {
                process.retries++;
                Logger::info(name + ": incremented retry count for pid " + std::to_string(pid));
                return;
            }
        }
    }
}

bool removeServicePid(const std::string& name, pid_t pid) {
    std::lock_guard<std::mutex> lock(serviceMutex);

    if (runningServices.count(name) == 0) return false;

    auto& processes = runningServices[name].processes;
    for (auto it = processes.begin(); it != processes.end(); ++it) {
        if (it->pid == pid) {
            it->state = ProcessState::STOPPED;
            Logger::info(name + ": marked process " + std::to_string(pid) + " as STOPPED");
            return true;
        }
    }
    return false;
}

size_t getServiceInstanceCount(const std::string& name) {
    if (runningServices.count(name) > 0) {
        size_t count = 0;
        for (const auto& process : runningServices[name].processes) {
            if (process.state == ProcessState::RUNNING || process.state == ProcessState::STARTING ||
                process.state == ProcessState::RESTARTING) {
                count++;
            }
        }
        return count;
    }
    return 0;
}

void updateServiceState(const std::string& name, ProcessState state) {
    // std::lock_guard<std::mutex> lock(serviceMutex);

    runningServices[name].overallState = state;

    // Log de changement d'état
    std::string stateStr;
    switch (state) {
        case ProcessState::STARTING:
            stateStr = "STARTING";
            break;
        case ProcessState::RUNNING:
            stateStr = "RUNNING";
            break;
        case ProcessState::RESTARTING:
            stateStr = "RESTARTING";
            break;
        case ProcessState::STOPPED:
            stateStr = "STOPPED";
            break;
        case ProcessState::FATAL:
            stateStr = "FATAL";
            break;
        default:
            stateStr = "UNKNOWN";
            break;
    }

    Logger::info(name + ": state changed to " + stateStr);
}

bool updateProcessState(const std::string& name, pid_t pid, ProcessState state, int exitCode) {
    std::lock_guard<std::mutex> lock(serviceMutex);

    if (runningServices.count(name) == 0) return false;

    auto& service   = runningServices[name];
    auto& processes = service.processes;
    for (auto& process : processes) {
        if (process.pid == pid) {
            process.state    = state;
            process.exitCode = exitCode;

            if (state == ProcessState::STOPPED) {
                bool anyRunning = false;
                for (const auto& p : processes) {
                    if (p.state == ProcessState::RUNNING || p.state == ProcessState::STARTING ||
                        p.state == ProcessState::RESTARTING) {
                        anyRunning = true;
                        break;
                    }
                }
                if (!anyRunning) {
                    service.overallState = ProcessState::STOPPED;
                    Logger::info(name + ": state changed to STOPPED (all processes stopped)");
                }
            }

            return true;
        }
    }
    return false;
}

void cleanupOldProcesses(const std::string& name, size_t maxStoppedToKeep) {
    if (runningServices.count(name) == 0) return;

    auto& processes = runningServices[name].processes;

    std::vector<size_t> stoppedIndices;
    for (size_t i = 0; i < processes.size(); i++) {
        if (processes[i].state == ProcessState::STOPPED) {
            stoppedIndices.push_back(i);
        }
    }

    if (stoppedIndices.size() > maxStoppedToKeep) {
        std::sort(stoppedIndices.begin(), stoppedIndices.end(), [&processes](size_t a, size_t b) {
            return processes[a].startTime < processes[b].startTime;
        });

        size_t             toRemove = stoppedIndices.size() - maxStoppedToKeep;
        std::vector<pid_t> pidsToRemove;

        for (size_t i = 0; i < toRemove; i++) {
            pidsToRemove.push_back(processes[stoppedIndices[i]].pid);
        }

        std::sort(stoppedIndices.begin(), stoppedIndices.begin() + toRemove,
                  std::greater<size_t>());
        for (size_t i = 0; i < toRemove; i++) {
            processes.erase(processes.begin() + stoppedIndices[i]);
        }

        Logger::info(name + ": removed " + std::to_string(toRemove) + " old stopped processes");
    }
}
