#ifndef CONFIG_PROGRAM_HPP
#define CONFIG_PROGRAM_HPP

#ifdef __APPLE__
#include <util.h>
#else
#include <pty.h>
#endif

#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "../cmds/service_types.hpp"
#include "../logger.hpp"

enum class EAutorestart {
    ALWAYS,     // "true"
    NEVER,      // "false"
    UNEXPECTED  // "unexpected"
};

enum class EStopsignal {
    TERM = SIGTERM,
    HUP  = SIGHUP,
    INT  = SIGINT,
    QUIT = SIGQUIT,
    KILL = SIGKILL,
    USR1 = SIGUSR1,
    USR2 = SIGUSR2
};

// Fonctions utilitaires pour la conversion entre enum et string
inline std::string autorestart_to_string(EAutorestart value) {
    switch (value) {
        case EAutorestart::ALWAYS:
            return "true";
        case EAutorestart::NEVER:
            return "false";
        case EAutorestart::UNEXPECTED:
            return "unexpected";
        default:
            return "unexpected";
    }
}

inline EAutorestart string_to_autorestart(const std::string& value) {
    if (value == "true" || value == "always")
        return EAutorestart::ALWAYS;
    else if (value == "false" || value == "never")
        return EAutorestart::NEVER;
    else
        return EAutorestart::UNEXPECTED;
}

inline std::string stopsignal_to_string(EStopsignal value) {
    switch (value) {
        case EStopsignal::TERM:
            return "TERM";
        case EStopsignal::HUP:
            return "HUP";
        case EStopsignal::INT:
            return "INT";
        case EStopsignal::QUIT:
            return "QUIT";
        case EStopsignal::KILL:
            return "KILL";
        case EStopsignal::USR1:
            return "USR1";
        case EStopsignal::USR2:
            return "USR2";
        default:
            return "TERM";
    }
}

inline EStopsignal string_to_stopsignal(const std::string& value) {
    if (value == "HUP")
        return EStopsignal::HUP;
    else if (value == "INT")
        return EStopsignal::INT;
    else if (value == "QUIT")
        return EStopsignal::QUIT;
    else if (value == "KILL")
        return EStopsignal::KILL;
    else if (value == "USR1")
        return EStopsignal::USR1;
    else if (value == "USR2")
        return EStopsignal::USR2;
    else
        return EStopsignal::TERM;
}

inline int to_raw_signal(EStopsignal sig) {
    return static_cast<int>(sig);
}

class ProgramConfig {
   private:
    std::string                        command;
    int                                numprocs = 1;
    mode_t                             umask    = 022;
    std::string                        workingdir;
    bool                               autostart   = true;
    EAutorestart                       autorestart = EAutorestart::UNEXPECTED;
    std::vector<int>                   exitcodes = {0};  // Changé à {0} pour suivre Supervisor 4.0+
    int                                startretries   = 3;
    int                                startsecs      = 1;
    EStopsignal                        stopsignal     = EStopsignal::TERM;
    int                                stopwaitsecs   = 10;
    std::string                        stdout_logfile = "/dev/null";
    std::string                        stderr_logfile = "/dev/null";
    std::map<std::string, std::string> environment;

   public:
    // Getters
    std::string  getCommand() const { return command; }
    int          getNumprocs() const { return numprocs; }
    mode_t       getUmask() const { return umask; }
    std::string  getWorkingDir() const { return workingdir; }
    bool         getAutostart() const { return autostart; }
    EAutorestart getAutorestart() const { return autorestart; }
    std::string  getAutorerestartString() const { return autorestart_to_string(autorestart); }
    const std::vector<int>& getExitcodes() const { return exitcodes; }
    int                     getStartretries() const { return startretries; }
    int                     getStartsecs() const { return startsecs; }
    EStopsignal             getStopsignal() const { return stopsignal; }
    std::string             getStopsignalString() const { return stopsignal_to_string(stopsignal); }
    int                     getStopwaitsecs() const { return stopwaitsecs; }
    std::string             getStdoutLogfile() const { return stdout_logfile; }
    std::string             getStderrLogfile() const { return stderr_logfile; }
    const std::map<std::string, std::string>& getEnvironment() const { return environment; }

    // Setters
    void setCommand(const std::string& value) { command = value; }
    void setNumprocs(int value) { numprocs = value; }
    void setUmask(const mode_t& value) { umask = value; }
    void setWorkingDir(const std::string& value) { workingdir = value; }
    void setAutostart(bool value) { autostart = value; }
    void setAutorestart(EAutorestart value) { autorestart = value; }
    void setAutorestart(const std::string& value) { autorestart = string_to_autorestart(value); }
    void setExitcodes(const std::vector<int>& value) { exitcodes = value; }
    void setStartretries(int value) { startretries = value; }
    void setStartsecs(int value) { startsecs = value; }
    void setStopsignal(EStopsignal value) { stopsignal = value; }
    void setStopsignal(const std::string& value) { stopsignal = string_to_stopsignal(value); }
    void setStopwaitsecs(int value) { stopwaitsecs = value; }
    void setStdoutLogfile(const std::string& value) { stdout_logfile = value; }
    void setStderrLogfile(const std::string& value) { stderr_logfile = value; }
    void setEnvironment(const std::map<std::string, std::string>& value) { environment = value; }

    void logConfig() const {
        Logger::info("command: " + command);
        Logger::info("numprocs: " + numprocs);
        Logger::info("umask: " + getUmask());
        Logger::info("workingdir: " + workingdir);
        Logger::info("autostart: " + getAutostart());
        Logger::info("autorestart: " + getAutorerestartString());
        Logger::info("startretries: " + startretries);
        Logger::info("startsecs: " + startsecs);
        Logger::info("stopsignal: " + getStopsignalString());
        Logger::info("stopwaitsecs: " + stopwaitsecs);
        Logger::info("stdout_logfile: " + stdout_logfile);
        Logger::info("stderr_logfile: " + stderr_logfile);
        Logger::info("exitcodes: ");
        for (const auto& exitcode : exitcodes) Logger::info(exitcode + " ");
        Logger::info("environment: ");
        for (const auto& env : environment) Logger::info("  " + env.first + "=" + env.second);
    }

    bool isValid() {
        bool valid = true;

        if (command.empty()) {
            Logger::error("Command is required.");
            valid = false;
        }
        if (workingdir.empty()) {
            Logger::error("Workingdir is required.");
            valid = false;
        }
        if (environment.empty()) {
            Logger::warn("No environment defined.");
        }

        return valid;
    }
};

void  exec_programs(const std::map<std::string, ProgramConfig>& programs);
void  log_config(const std::map<std::string, ProgramConfig>& config);
void  setup_signal_handlers();
pid_t launch_program(const std::string& name, const ProgramConfig& config);
std::map<std::string, ProgramConfig> parsing(std::string filename);

#endif  // CONFIG_PROGRAM_HPP