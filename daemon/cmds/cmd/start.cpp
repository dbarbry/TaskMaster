#include <libgen.h>

#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "../../logger.hpp"
#include "../cmds.hpp"
#include "launch/launch.hpp"

extern std::map<std::string, ServiceInfo> runningServices;

void monitoring(std::shared_ptr<std::vector<pid_t>> pids);

void startCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                  const std::map<std::string, ProgramConfig>            &programs) {
    Logger::info("Starting command logic");

    if (!cmd.count("args") || cmd.at("args").empty()) {
        Logger::error("No program specified to start.");
        return;
    }

    std::string requestedProgram = cmd.at("args")[0];
    Logger::debug("Requested program: " + requestedProgram);

    auto it = programs.find(requestedProgram);
    if (it == programs.end()) {
        Logger::error("Program " + requestedProgram + " not found in configuration.");
        return;
    }
    const ProgramConfig &configFromFile = it->second;
    int                  maxInstances   = configFromFile.getNumprocs();

    if (getServiceInstanceCount(requestedProgram) >= static_cast<size_t>(maxInstances)) {
        Logger::error("The program " + requestedProgram + " already has " + 
                    std::to_string(maxInstances) + " instance(s) running!");
        return;
    }

    setup_signal_handlers();

    auto pids = std::make_shared<std::vector<pid_t>>();

    updateServiceState(requestedProgram, ProcessState::STARTING);

    const int max_retries         = configFromFile.getStartretries();
    int       remaining_instances = maxInstances - getServiceInstanceCount(requestedProgram);

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

            Logger::info("Started program " + requestedProgram + " with PID " + std::to_string(pid));
        } else {
            Logger::error("Failed to start program " + requestedProgram + " after " + 
                        std::to_string(max_retries) + " attempts.");

            if (getServiceInstanceCount(requestedProgram) == 0) {
                updateServiceState(requestedProgram, ProcessState::FATAL);
            }
        }
    }

    if (!pids->empty()) {
        std::thread monitor_thread(monitoring, pids);
        monitor_thread.detach();
    }
}