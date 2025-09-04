#include "../../logger.hpp"
#include "../cmds.hpp"
#include "launch/config_program.hpp"

extern std::map<std::string, ServiceInfo> runningServices;

void monitoring(std::shared_ptr<std::vector<pid_t>> pids);

std::string startCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                         const std::map<std::string, ProgramConfig>            &programs) {
    Logger::info("Starting command logic");
    std::ostringstream response;
    std::string        requestedProgram;

    if (!cmd.count("args") || cmd.at("args").empty()) {
        Logger::error("No program specified to start.");
        response << "Error: No program specified to start.";
        return response.str();
    }

    requestedProgram = cmd.at("args")[0];
    Logger::debug("Requested program: " + requestedProgram);

    auto it = programs.find(requestedProgram);
    if (it == programs.end()) {
        Logger::error("Program " + requestedProgram + " not found in configuration.");
        response << "Error: Program " + requestedProgram + " not found in configuration.";
        return response.str();
    }

    const ProgramConfig &configFromFile = it->second;
    int                  maxInstances   = configFromFile.getNumprocs();

    if (getServiceInstanceCount(requestedProgram) >= static_cast<size_t>(maxInstances)) {
        Logger::error("The program " + requestedProgram + " already has " +
                      std::to_string(maxInstances) + " instance(s) running!");
        response << "The program " + requestedProgram + " already has " +
                        std::to_string(maxInstances) + " instance(s) running!";
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
            if (pid <= 0) {
                Logger::error("Execution failed for program " + requestedProgram + " (attempt " +
                              std::to_string(retries + 1) + ")");
                retries++;
                incrementRetries(requestedProgram, -1);
                continue;
            }
            addServicePid(requestedProgram, pid, configFromFile.getStartsecs());
            pids->push_back(pid);

            Logger::info("Started program " + requestedProgram + " with PID " +
                         std::to_string(pid));
            response << "Started program " + requestedProgram + " with PID " + std::to_string(pid)
                     << std::endl;
            successful_starts++;
            break;
        }

        if (pid <= 0 && retries >= max_retries) {
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
                     << " started successfully.";
        } else {
            response << successful_starts << " of " << remaining_instances
                     << " instance(s) started successfully.";
        }
    } else {
        response << "Failed to start any instances of " << requestedProgram;
    }

    return response.str();
}
