#include "../cmds.hpp"
#include "../service_state.hpp"
#include <iostream>
#include <iomanip>

std::string getStateString(ProcessState state) {
    switch (state) {
        case ProcessState::STARTING:   return "STARTING";
        case ProcessState::RUNNING:    return "RUNNING";
        case ProcessState::RESTARTING: return "RESTARTING";
        case ProcessState::STOPPED:    return "STOPPED";
        case ProcessState::FATAL:      return "FATAL";
        default:                       return "UNKNOWN";
    }
}

void statusCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                   const std::map<std::string, ProgramConfig> &programs) {
    std::cout << "=== Services Status ===" << std::endl;
    
    // std::lock_guard<std::mutex> lock(serviceMutex);
    
    if (runningServices.empty()) {
        std::cout << "No services are currently registered." << std::endl;
        return;
    }
    
    // Format d'affichage avec des colonnes alignées
    std::cout << std::left 
              << std::setw(20) << "SERVICE NAME"
              << std::setw(12) << "STATE"
              << std::setw(10) << "INSTANCES"
              << std::setw(25) << "PIDS"
              << std::setw(10) << "RETRIES"
              << std::endl;
    std::cout << std::string(77, '-') << std::endl;
    
    for (const auto& [name, info] : runningServices) {
        std::string pids_str;
        for (pid_t pid : info.pids) {
            if (!pids_str.empty()) pids_str += ", ";
            pids_str += std::to_string(pid);
        }
        
        std::cout << std::left 
                  << std::setw(20) << name
                  << std::setw(12) << getStateString(info.state)
                  << std::setw(10) << info.pids.size()
                  << std::setw(25) << pids_str
                  << std::setw(10) << info.retries
                  << std::endl;
    }
}