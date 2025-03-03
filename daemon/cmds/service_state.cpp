#include "service_state.hpp"
#include <algorithm>
#include <iostream>
#include "../launch/launch.hpp" 

// Définition de la map globale
std::map<std::string, ServiceInfo> runningServices;
std::mutex serviceMutex;

void updateServiceState(const std::string& name, ProcessState state) {
    // std::lock_guard<std::mutex> lock(serviceMutex);
    
    runningServices[name].state = state;
    
    // Log de changement d'état
    std::cout << "[STATE] Service " << name << " state changed to ";
    switch (state) {
        case ProcessState::STARTING:   std::cout << "STARTING"; break;
        case ProcessState::RUNNING:    std::cout << "RUNNING"; break;
        case ProcessState::RESTARTING: std::cout << "RESTARTING"; break;
        case ProcessState::STOPPED:    std::cout << "STOPPED"; break;
        case ProcessState::FATAL:      std::cout << "FATAL"; break;
        default:                       std::cout << "UNKNOWN"; break;
    }
    std::cout << std::endl;
}

bool addServicePid(const std::string& name, pid_t pid) {
    // std::lock_guard<std::mutex> lock(serviceMutex);
    
    // Vérification que le PID n'existe pas déjà
    auto& pids = runningServices[name].pids;
    if (std::find(pids.begin(), pids.end(), pid) != pids.end()) {
        return false;
    }
    
    pids.push_back(pid);
    runningServices[name].lastStartTime = std::time(nullptr);
    std::cout << "[PID] Added PID " << pid << " to service " << name << std::endl;
    return true;
}

bool removeServicePid(const std::string& name, pid_t pid) {
    // std::lock_guard<std::mutex> lock(serviceMutex);
    
    if (runningServices.count(name) == 0) return false;
    
    auto& pids = runningServices[name].pids;
    auto it = std::find(pids.begin(), pids.end(), pid);
    
    if (it != pids.end()) {
        pids.erase(it);
        std::cout << "[PID] Removed PID " << pid << " from service " << name << std::endl;
        
        // Si c'était le dernier processus, le service est arrêté
        if (pids.empty()) {
            updateServiceState(name, ProcessState::STOPPED);
        }
        
        return true;
    }
    return false;
}

void clearService(const std::string& name) {
    // std::lock_guard<std::mutex> lock(serviceMutex);
    
    if (runningServices.count(name) > 0) {
        std::cout << "[SERVICE] Cleared service " << name << std::endl;
        runningServices.erase(name);
    }
}

bool isServiceRunning(const std::string& name) {
    std::lock_guard<std::mutex> lock(serviceMutex);
    
    return runningServices.count(name) > 0 && 
           !runningServices[name].pids.empty() && 
           runningServices[name].state == ProcessState::RUNNING;
}

size_t getServiceInstanceCount(const std::string& name) {
    // std::lock_guard<std::mutex> lock(serviceMutex);
    
    if (runningServices.count(name) > 0) {
        return runningServices[name].pids.size();
    }
    return 0;
}

ServiceInfo getServiceInfo(const std::string& name) {
    // std::lock_guard<std::mutex> lock(serviceMutex);
    
    if (runningServices.count(name) > 0) {
        return runningServices[name];
    }
    return ServiceInfo();
}

void incrementRetries(const std::string& name) {
    // std::lock_guard<std::mutex> lock(serviceMutex);
    
    if (runningServices.count(name) > 0) {
        runningServices[name].retries++;
    }
}

bool shouldRestart(const std::string& name, const ProgramConfig& config, int exitCode) {
    // std::lock_guard<std::mutex> lock(serviceMutex);
    
    // Vérifier si le service existe dans runningServices
    if (runningServices.count(name) == 0) return false;
    
    // Vérifier la configuration de redémarrage
    std::string autorestart = config.getAutorestart();
    
    if (autorestart == "never") {
        return false;
    }
    else if (autorestart == "always") {
        return true;
    }
    else if (autorestart == "unexpected") {
        // Vérifier si le code de sortie est dans la liste des codes attendus
        const auto& exitcodes = config.getExitcodes();
        return std::find(exitcodes.begin(), exitcodes.end(), exitCode) == exitcodes.end();
    }
    
    return false;
}