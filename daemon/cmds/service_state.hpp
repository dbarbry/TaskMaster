#ifndef SERVICE_STATE_HPP
#define SERVICE_STATE_HPP

#include <algorithm>
#include <map>
#include <mutex>
#include <string>

#include "./service_types.hpp"

// Forward declaration
class ProgramConfig;

extern std::map<std::string, ServiceInfo> runningServices;
extern std::mutex                         serviceMutex;
extern bool                               monitoringStarted;

// Fonctions utilitaires pour manipuler l'état des services
void   updateServiceState(const std::string& name, ProcessState state);
bool   addServicePid(const std::string& name, pid_t pid, int startsecs);
bool   removeServicePid(const std::string& name, pid_t pid);
bool   isServiceRunning(const std::string& name);
size_t getServiceInstanceCount(const std::string& name);
void   incrementRetries(const std::string& name, pid_t pid);
bool   updateProcessState(const std::string& name, pid_t pid, ProcessState state, int exitCode = 0);
void   cleanupOldProcesses(const std::string& name, size_t maxStoppedToKeep = 10);

#endif  // SERVICE_STATE_HPP