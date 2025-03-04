#ifndef SERVICE_STATE_HPP
#define SERVICE_STATE_HPP

#include "./service_types.hpp"
#include <map>
#include <string>
#include <mutex>

// Forward declaration
class ProgramConfig;

// Map globale qui stocke l'état de tous les services en cours d'exécution
extern std::map<std::string, ServiceInfo> runningServices;
extern std::mutex serviceMutex; // Pour protéger les accès concurrents

// Fonctions utilitaires pour manipuler l'état des services
void updateServiceState(const std::string& name, ProcessState state);
bool addServicePid(const std::string& name, pid_t pid);
bool removeServicePid(const std::string& name, pid_t pid);
void clearService(const std::string& name);
bool isServiceRunning(const std::string& name);
size_t getServiceInstanceCount(const std::string& name);
ServiceInfo getServiceInfo(const std::string& name);
void incrementRetries(const std::string& name);
bool shouldRestart(const std::string& name, const ProgramConfig& config, int exitCode);
bool updateProcessState(const std::string& name, pid_t pid, ProcessState state, int exitCode = 0);
void cleanupOldProcesses(const std::string& name, size_t maxStoppedToKeep = 10);

#endif