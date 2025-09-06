#include "../../logger.hpp"
#include "../cmds.hpp"
#include "launch/config_program.hpp"

extern std::map<std::string, ServiceInfo> runningServices;
extern std::mutex                         serviceMutex;

extern void monitoring(std::shared_ptr<std::vector<pid_t>>         pids,
                       const std::map<std::string, ProgramConfig> *programs);

std::string startCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                         const std::map<std::string, ProgramConfig>            &programs) {
    Logger::info("Starting command logic");
    std::ostringstream response;

    if (!cmd.count("args") || cmd.at("args").empty()) {
        Logger::error("No program specified to start.");
        return "Error: No program specified to start.";
    }

    std::string requestedProgram = cmd.at("args")[0];
    Logger::debug("Requested program: " + requestedProgram);

    auto it = programs.find(requestedProgram);
    if (it == programs.end()) {
        Logger::error("Program " + requestedProgram + " not found in configuration.");
        return "Error: Program " + requestedProgram + " not found in configuration.";
    }

    {
        std::lock_guard<std::mutex> lock(serviceMutex);
        auto                       &processes = runningServices[requestedProgram].processes;
        processes.erase(std::remove_if(processes.begin(), processes.end(),
                                       [](const ProcessInfo &p) {
                                           return p.state == ProcessState::STOPPED ||
                                                  p.state == ProcessState::FATAL;
                                       }),
                        processes.end());
    }

    const ProgramConfig &config       = it->second;
    int                  maxInstances = config.getNumprocs();

    int currentInstances = getServiceInstanceCount(requestedProgram);
    if (currentInstances >= maxInstances) {
        Logger::warn(requestedProgram + " already has " + std::to_string(currentInstances) +
                     " instance(s), max is " + std::to_string(maxInstances));
        return "The program " + requestedProgram + " already has " +
               std::to_string(currentInstances) + " instance(s) running!";
    }

    int remainingInstances = maxInstances - currentInstances;
    int successfulStarts   = 0;

    Logger::info("Attempting to start " + std::to_string(remainingInstances) + " instance(s) of " +
                 requestedProgram);

    updateServiceState(requestedProgram, ProcessState::STARTING);
    auto newPids = std::make_shared<std::vector<pid_t>>();

    for (int i = 0; i < remainingInstances; i++) {
        int   retries = 0;
        pid_t pid     = -1;

        while (retries < config.getStartretries()) {
            pid = launch_program(requestedProgram, config);
            if (pid > 0) break;

            retries++;
            Logger::warn(requestedProgram + ": retry " + std::to_string(retries) + "/" +
                         std::to_string(config.getStartretries()));
        }

        if (pid > 0) {
            {
                std::lock_guard<std::mutex> lock(serviceMutex);
                addServicePid(requestedProgram, pid, config.getStartsecs());
            }

            newPids->push_back(pid);
            Logger::info("Started " + requestedProgram + " with PID " + std::to_string(pid));
            response << "Started " << requestedProgram << " with PID " << pid << "\n";
            successfulStarts++;
        } else {
            Logger::error("Failed to start " + requestedProgram + " after " +
                          std::to_string(config.getStartretries()) + " retries.");
            response << "Failed to start " << requestedProgram
                     << " after " + std::to_string(config.getStartretries()) + " retries.\n";

            if (getServiceInstanceCount(requestedProgram) == 0) {
                updateServiceState(requestedProgram, ProcessState::FATAL);
                response << requestedProgram << " is now in FATAL state.\n";
            }
        }
    }
    if (!monitoringStarted) {
        auto allPids = std::make_shared<std::vector<pid_t>>();
        {
            std::lock_guard<std::mutex> lock(serviceMutex);
            for (auto &[svcName, svc] : runningServices) {
                for (auto &proc : svc.processes) {
                    allPids->push_back(proc.pid);
                }
            }
        }

        std::thread monitor_thread(monitoring, allPids, &programs);
        monitor_thread.detach();

        monitoringStarted = true;
        Logger::info("Monitoring thread started globally");
    }

    if (successfulStarts > 0) {
        if (successfulStarts == remainingInstances) {
            response << "All " << successfulStarts << " instance(s) of " << requestedProgram
                     << " started successfully.";
        } else {
            response << successfulStarts << " of " << remainingInstances
                     << " instance(s) started successfully.";
        }
    } else {
        response << "No new instances of " << requestedProgram << " were started.";
    }

    return response.str();
}
