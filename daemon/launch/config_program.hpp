#ifndef LAUNCH_HPP
#define LAUNCH_HPP

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

enum class EAutorestart {
    ALWAYS,     // "true"
    NEVER,      // "false"
    UNEXPECTED  // "unexpected"
};

enum class EStopsignal { TERM, HUP, INT, QUIT, KILL, USR1, USR2 };

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

class ProgramConfig {
   public:
    std::string                        command;
    int                                numprocs = 1;
    std::optional<std::string>         umask;
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
    std::string getUmask() const { return umask.value_or("022"); }
    std::string getAutorerestartString() const { return autorestart_to_string(autorestart); }
    std::string getStopsignalString() const { return stopsignal_to_string(stopsignal); }

    // Setters
    void setAutorestart(const std::string& value) { autorestart = string_to_autorestart(value); }
    void setStopsignal(const std::string& value) { stopsignal = string_to_stopsignal(value); }

    void logConfig() const {
        std::cout << "command: " << command << std::endl;
        std::cout << "numprocs: " << numprocs << std::endl;
        std::cout << "umask: " << getUmask() << std::endl;
        std::cout << "workingdir: " << workingdir << std::endl;
        std::cout << "autostart: " << (autostart ? "true" : "false") << std::endl;
        std::cout << "autorestart: " << getAutorerestartString() << std::endl;
        std::cout << "startretries: " << startretries << std::endl;
        std::cout << "startsecs: " << startsecs << std::endl;
        std::cout << "stopsignal: " << getStopsignalString() << std::endl;
        std::cout << "stopwaitsecs: " << stopwaitsecs << std::endl;
        std::cout << "stdout_logfile: " << stdout_logfile << std::endl;
        std::cout << "stderr_logfile: " << stderr_logfile << std::endl;
        std::cout << "exitcodes: ";
        for (const auto& exitcode : exitcodes) std::cout << exitcode << " " << std::endl;
        std::cout << "environment: " << std::endl;
        for (const auto& env : environment)
            std::cout << "  " << env.first << "=" << env.second << std::endl;
    }

    bool isValid() {
        bool valid = true;

        if (command.empty()) {
            std::cerr << "Erreur: command est obligatoire." << std::endl;
            valid = false;
        }
        if (workingdir.empty()) {
            std::cerr << "Erreur: workingdir est obligatoire." << std::endl;
            valid = false;
        }
        if (environment.empty()) {
            std::cerr << "Avertissement: Aucun environnement défini." << std::endl;
        }

        return valid;
    }
};

void  exec_programs(const std::map<std::string, ProgramConfig>& programs);
void  log_config(const std::map<std::string, ProgramConfig>& config);
void  setup_signal_handlers();
pid_t launch_program(const std::string& name, const ProgramConfig& config);
std::map<std::string, ProgramConfig> parsing(std::string filename);

#endif  // LAUNCH_HPP