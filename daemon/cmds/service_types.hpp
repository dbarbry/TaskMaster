#ifndef SERVICE_TYPES_HPP
#define SERVICE_TYPES_HPP

#include <ctime>
#include <string>
#include <vector>

enum class ProcessState { STARTING, RUNNING, RESTARTING, STOPPED, FATAL, UNKNOWN };
struct ProcessInfo {
    pid_t        pid;
    ProcessState state;
    int          retries;
    std::time_t  startTime;
    int          exitCode;
};

struct ServiceInfo {
    std::string              name;
    std::vector<ProcessInfo> processes;
    ProcessState             overallState;
    int                      startsecs;
};

class ProgramConfig;

#endif  // SERVICE_TYPES_HPP