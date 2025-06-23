#include "../../logger.hpp"
#include "../cmds.hpp"
#include "launch/config_program.hpp"

extern std::map<std::string, ServiceInfo> runningServices;

void monitoring(std::shared_ptr<std::vector<pid_t>> pids);

std::string startCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                         const std::map<std::string, ProgramConfig>            &programs) {
    Logger::info("Starting command logic");
    std::ostringstream response;

    if (!cmd.count("args") || cmd.at("args").empty()) {
        Logger::error("No program specified to start.");
        response << "Error: No program specified to start." << std::endl;
        return response.str();
    }

    std::string requestedProgram = cmd.at("args")[0];
    Logger::debug("Requested program: " + requestedProgram);

    auto it = programs.find(requestedProgram);
    if (it == programs.end()) {
        Logger::error("Program " + requestedProgram + " not found in configuration.");
        response << "Error: Program " + requestedProgram + " not found in configuration."
                 << std::endl;
        return response.str();
    }

    const ProgramConfig &configFromFile = it->second;
    int                  maxInstances   = configFromFile.getNumprocs();

    if (getServiceInstanceCount(requestedProgram) >= static_cast<size_t>(maxInstances)) {
        Logger::error("The program " + requestedProgram + " already has " +
                      std::to_string(maxInstances) + " instance(s) running!");
        response << "Error: The program " + requestedProgram + " already has " +
                        std::to_string(maxInstances) + " instance(s) running!"
                 << std::endl;
        return response.str();
    }

    setup_signal_handlers();

    auto pids = std::make_shared<std::vector<pid_t>>();

    updateServiceState(requestedProgram, ProcessState::STARTING);
    response << "Starting program: " << requestedProgram << std::endl;

    const int max_retries         = configFromFile.getStartretries();
    int       remaining_instances = maxInstances - getServiceInstanceCount(requestedProgram);
    int       successful_starts   = 0;

    for (int i = 0; i < remaining_instances; i++) {
        int   retries = 0;
        pid_t pid     = -1;

        while (retries < max_retries) {
            pid = launch_program(requestedProgram, configFromFile);
            if (pid > 0) break;
            retries++;
            incrementRetries(requestedProgram);
        }

        if (pid > 0) {
            pids->push_back(pid);
            addServicePid(requestedProgram, pid);
            updateServiceState(requestedProgram, ProcessState::RUNNING);

            Logger::info("Started program " + requestedProgram + " with PID " +
                         std::to_string(pid));
            response << "Started program " + requestedProgram + " with PID " + std::to_string(pid)
                     << std::endl;
            successful_starts++;
        } else {
            Logger::error("Failed to start program " + requestedProgram + " after " +
                          std::to_string(max_retries) + " attempts.");
            response << "Failed to start program " + requestedProgram + " after " +
                            std::to_string(max_retries) + " attempts."
                     << std::endl;

            if (getServiceInstanceCount(requestedProgram) == 0) {
                updateServiceState(requestedProgram, ProcessState::FATAL);
                response << "Program " << requestedProgram << " is in FATAL state." << std::endl;
            }
        }
    }

    if (!pids->empty()) {
        std::thread monitor_thread(monitoring, pids);
        monitor_thread.detach();

        if (successful_starts == remaining_instances) {
            response << "All " << successful_starts << " instance(s) of " << requestedProgram
                     << " started successfully." << std::endl;
        } else {
            response << successful_starts << " of " << remaining_instances
                     << " instance(s) started successfully." << std::endl;
        }
    } else {
        response << "Failed to start any instances of " << requestedProgram << std::endl;
    }

    return response.str();
}
