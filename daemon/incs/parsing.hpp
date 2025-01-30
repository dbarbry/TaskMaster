#ifndef PARSING_HPP
#define PARSING_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <sstream>

class ProgramConfig {
private : 

    std::string cmd;
    int numprocs = -1;
    std::string umask = "022";
    std::string workingdir;
    bool autostart = false;
    std::string autorestart = "unexpected";
    std::vector<int> exitcodes;
    int startretries = -1;
    int starttime = -1;
    std::string stopsignal = "TERM";
    int stoptime = -1;
    std::string stdout_file = "/dev/null";
    std::string stderr_file = "/dev/null";
    std::map<std::string, std::string> env;

public:



    // Getters
    std::string getCmd() const { return cmd; }
    int getNumprocs() const { return numprocs; }
    std::string getUmask() const { return umask; }
    std::string getWorkingDir() const { return workingdir; }
    bool getAutostart() const { return autostart; }
    std::string getAutorestart() const { return autorestart; }
    const std::vector<int>& getExitcodes() const { return exitcodes; }
    int getStartretries() const { return startretries; }
    int getStarttime() const { return starttime; }
    std::string getStopsignal() const { return stopsignal; }
    int getStoptime() const { return stoptime; }
    std::string getStdoutFile() const { return stdout_file; }
    std::string getStderrFile() const { return stderr_file; }
    const std::map<std::string, std::string>& getEnv() const { return env; }

    // Setters
    void setCmd(const std::string& value) { cmd = value; }
    void setNumprocs(int value) { numprocs = value; }
    void setUmask(const std::string& value) { umask = value; }
    void setWorkingDir(const std::string& value) { workingdir = value; }
    void setAutostart(bool value) { autostart = value; }
    void setAutorestart(const std::string& value) { autorestart = value; }
    void setExitcodes(const std::vector<int>& value) { exitcodes = value; }
    void setStartretries(int value) { startretries = value; }
    void setStarttime(int value) { starttime = value; }
    void setStopsignal(const std::string& value) { stopsignal = value; }
    void setStoptime(int value) { stoptime = value; }
    void setStdoutFile(const std::string& value) { stdout_file = value; }
    void setStderrFile(const std::string& value) { stderr_file = value; }
    void setEnv(const std::map<std::string, std::string>& value) { env = value; }

    void logConfig() const {
        std::cout << "cmd: " << getCmd() << std::endl;
        std::cout << "numprocs: " << getNumprocs() << std::endl;
        std::cout << "umask: " << getUmask() << std::endl;
        std::cout << "workingdir: " << getWorkingDir() << std::endl;
        std::cout << "autostart: " << (getAutostart() ? "true" : "false") << std::endl;
        std::cout << "autorestart: " << getAutorestart() << std::endl;
        std::cout << "startretries: " << getStartretries() << std::endl;
        std::cout << "starttime: " << getStarttime() << std::endl;
        std::cout << "stopsignal: " << getStopsignal() << std::endl;
        std::cout << "stoptime: " << getStoptime() << std::endl;
        std::cout << "stdout_file: " << getStdoutFile() << std::endl;
        std::cout << "stderr_file: " << getStderrFile() << std::endl;

        std::cout << "exitcodes: ";
        for (const auto& exitcode : getExitcodes()) {
            std::cout << exitcode << " ";
        }
        std::cout << std::endl;

        std::cout << "env: " << std::endl;
        for (const auto& env : getEnv()) {
            std::cout << "  " << env.first << "=" << env.second << std::endl;
        }
    }

bool isValid() {
        bool valid = true;

        if (cmd.empty()) {
            std::cerr << "Erreur: command est obligatoire." << std::endl;
            valid = false;
        }
        if (workingdir.empty()) {
            std::cerr << "Erreur: workingdir est obligatoire." << std::endl;
            valid = false;
        }

        if (numprocs == -1) {
            std::cerr << "Avertissement: numprocs non défini, valeur par défaut = 1" << std::endl;
            numprocs = 1;
        } else if (numprocs <= 0) {
            std::cerr << "Avertissement: numprocs invalide, valeur par défaut = 1" << std::endl;
            numprocs = 1;
        }

        if (startretries == -1) {
            std::cerr << "Avertissement: startretries non défini, valeur par défaut = 3" << std::endl;
            startretries = 3;
        } else if (startretries <= 0) {
            std::cerr << "Avertissement: startretries invalide, valeur par défaut = 3" << std::endl;
            startretries = 3;
        }

        if (starttime == -1) {
            std::cerr << "Avertissement: starttime non défini, valeur par défaut = 1" << std::endl;
            starttime = 1;
        } else if (starttime <= 0) {
            std::cerr << "Avertissement: starttime invalide, valeur par défaut = 1" << std::endl;
            starttime = 1;
        }

        if (stoptime == -1) {
            std::cerr << "Avertissement: stoptime non défini, valeur par défaut = 10" << std::endl;
            stoptime = 10;
        } else if (stoptime <= 0) {
            std::cerr << "Avertissement: stoptime invalide, valeur par défaut = 10" << std::endl;
            stoptime = 10;
        }

        if (exitcodes.empty()) {
            std::cerr << "Avertissement: exitcodes non défini, valeur par défaut = {0, 2}" << std::endl;
            exitcodes = {0, 2};
        }

        if (env.empty()) {
            std::cerr << "Avertissement: Aucun environnement défini." << std::endl;
        }

        return valid;
    }
};



void parsing(char **args);

#endif