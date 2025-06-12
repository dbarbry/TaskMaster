#ifndef SERVICE_TYPES_HPP
#define SERVICE_TYPES_HPP

#include <vector>
#include <ctime>
#include <string>

enum class ProcessState {
    STARTING,
    RUNNING,
    RESTARTING,
    STOPPED,
    FATAL,
    UNKNOWN,
    STOPPING
};
struct ProcessInfo {
    pid_t pid;
    ProcessState state;
    int retries;
    std::time_t startTime;
    int exitCode;
};

struct ServiceInfo {
    std::string name;
    std::vector<ProcessInfo> processes;
    ProcessState overallState;
};

class ProgramConfig;

#endif