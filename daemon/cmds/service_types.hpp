#ifndef SERVICE_TYPES_HPP
#define SERVICE_TYPES_HPP

#include <vector>
#include <ctime>

enum class ProcessState {
    STARTING,
    RUNNING,
    RESTARTING,
    STOPPED,
    FATAL,
    UNKNOWN
};

struct ServiceInfo {
    std::vector<pid_t> pids;
    ProcessState state;
    int retries;
    std::time_t lastStartTime;
};

class ProgramConfig;

#endif