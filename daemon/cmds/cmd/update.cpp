#include "../cmds.hpp"
#include "../service_state.hpp"
#include "../../launch/launch.hpp"
#include "../../main.hpp"
#include "../../logger.hpp"
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <signal.h>
#include <errno.h>
#include <cstring>

// Forward declarations pour les fonctions helper
bool isProgramConfigurationChanged(const ProgramConfig& old_config, const ProgramConfig& new_config);
void startNewProgram(const std::string& name, const ProgramConfig& config, std::ostringstream& response);
void stopProgramProcesses(const std::string& name, const ProgramConfig& config);
int getStopSignal(const std::string& signal_name);

/**
 * @brief Converts signal name string to signal number
 */
int getStopSignal(const std::string& signal_name) {
    if (signal_name == "INT") return SIGINT;
    else if (signal_name == "QUIT") return SIGQUIT;
    else if (signal_name == "KILL") return SIGKILL;
    else if (signal_name == "HUP") return SIGHUP;
    else if (signal_name == "USR1") return SIGUSR1;
    else if (signal_name == "USR2") return SIGUSR2;
    else return SIGTERM; // Default to SIGTERM
}

/**
 * @brief Compares two program configurations to detect changes
 */
bool isProgramConfigurationChanged(const ProgramConfig& old_config, const ProgramConfig& new_config) {
    return (old_config.getCommand() != new_config.getCommand() ||
            old_config.getNumprocs() != new_config.getNumprocs() ||
            old_config.getWorkingDir() != new_config.getWorkingDir() ||
            old_config.getUmask() != new_config.getUmask() ||
            old_config.getStopsignalString() != new_config.getStopsignalString() ||
            old_config.getStopwaitsecs() != new_config.getStopwaitsecs() ||
            old_config.getStdoutLogfile() != new_config.getStdoutLogfile() ||
            old_config.getStderrLogfile() != new_config.getStderrLogfile() ||
            old_config.getEnvironment() != new_config.getEnvironment() ||
            old_config.getAutorerestartString() != new_config.getAutorerestartString() ||
            old_config.getExitcodes() != new_config.getExitcodes() ||
            old_config.getStartretries() != new_config.getStartretries() ||
            old_config.getStartsecs() != new_config.getStartsecs());
}

/**
 * @brief Stops all running processes for a specific program
 */
void stopProgramProcesses(const std::string& name, const ProgramConfig& config) {
    std::vector<pid_t> pids_to_stop;
    
    // Collect PIDs of running processes
    {
        if (runningServices.count(name)) {
            for (const auto& process : runningServices[name].processes) {
                if (process.state == ProcessState::RUNNING) {
                    pids_to_stop.push_back(process.pid);
                }
            }
        }
    }
    
    if (pids_to_stop.empty()) {
        Logger::info("No running processes found for program: " + name);
        return;
    }
    
    int signal_to_send = getStopSignal(config.getStopsignalString());
    
    for (pid_t pid : pids_to_stop) {
        if (kill(pid, signal_to_send) == 0) {
            Logger::info("Sent " + config.getStopsignalString() + " to PID " + std::to_string(pid));
            updateProcessState(name, pid, ProcessState::STOPPED, 0);
        } else {
            Logger::error("Failed to send signal to PID " + std::to_string(pid) + ": " + strerror(errno));
        }
    }
}

/**
 * @brief Starts a new program with the specified configuration
 */
void startNewProgram(const std::string& name, const ProgramConfig& config, std::ostringstream& response) {
    const int max_retries = config.getStartretries();
    int nbr_instances = config.getNumprocs();
    
    for (int i = 0; i < nbr_instances; i++) {
        int retries = 0;
        pid_t pid = -1;
        
        while (retries < max_retries && pid <= 0) {
            pid = launch_program(name, config);
            if (pid > 0) {
                updateProcessState(name, pid, ProcessState::RUNNING, 0);
                Logger::info("Started instance " + std::to_string(i+1) + " of " + name + 
                           " (PID: " + std::to_string(pid) + ")");
            } else {
                retries++;
                if (retries < max_retries) {
                    Logger::info("Failed to start " + name + " instance " + std::to_string(i+1) + 
                                  ", retry " + std::to_string(retries) + "/" + std::to_string(max_retries));
                }
            }
        }
        
        if (pid <= 0) {
            Logger::error("Failed to start " + name + " instance " + std::to_string(i+1) + 
                         " after " + std::to_string(max_retries) + " retries");
            response << name << ": failed to start instance " << (i+1) << " after " 
                     << max_retries << " retries" << std::endl;
        }
    }
}

/**
 * @brief Updates the configuration by reloading the config file and applying changes
 */
std::string updateCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                         std::map<std::string, ProgramConfig> &programs) {
    // ... le reste de votre fonction updateCommand reste identique
}