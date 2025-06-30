#include "service_state.hpp"

#include <algorithm>

#include "../launch/config_program.hpp"
#include "../logger.hpp"

std::map<std::string, ServiceInfo> runningServices;
std::mutex                         serviceMutex;

bool addServicePid(const std::string& name, pid_t pid) {
    auto& processes = runningServices[name].processes;
    for (const auto& process : processes) {
        if (process.pid == pid) {
            return false;
        }
    }

    ProcessInfo newProcess;
    newProcess.pid       = pid;
    newProcess.state     = ProcessState::RUNNING;
    newProcess.retries   = 0;
    newProcess.startTime = std::time(nullptr);
    newProcess.exitCode  = 0;

    processes.push_back(newProcess);

    if (runningServices[name].name.empty()) {
        runningServices[name].name = name;
    }

    Logger::info(name + ": added process " + std::to_string(pid));
    return true;
}

bool isServiceRunning(const std::string& name) {
    if (runningServices.count(name) > 0) {
        return !runningServices[name].processes.empty();
    }
    return false;
}

void incrementRetries(const std::string& name) {
    if (runningServices.count(name) > 0) {
        for (auto& process : runningServices[name].processes) {
            process.retries++;
        }
        Logger::info(name + ": incremented retry count");
    }
}

bool removeServicePid(const std::string& name, pid_t pid) {
    if (runningServices.count(name) == 0) return false;

    auto& processes = runningServices[name].processes;
    for (auto it = processes.begin(); it != processes.end(); ++it) {
        if (it->pid == pid) {
            it->state = ProcessState::STOPPED;
            Logger::info(name + ": removed process " + std::to_string(pid));
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

// Ajouter une nouvelle fonction pour mettre à jour l'état d'un processus spécifique
bool updateProcessState(const std::string& name, pid_t pid, ProcessState state, int exitCode) {
    // std::lock_guard<std::mutex> lock(serviceMutex);

    if (runningServices.count(name) == 0) return false;

    auto& processes = runningServices[name].processes;
    for (auto& process : processes) {
        if (process.pid == pid) {
            process.state    = state;
            process.exitCode = exitCode;
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
