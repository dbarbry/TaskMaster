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
    int numprocs;
    std::string umask;
    std::string workingdir;
    bool autostart;
    std::string autorestart;
    std::vector<int> exitcodes;
    int startretries;
    int starttime;
    std::string stopsignal;
    int stoptime;
    std::string stdout_file;
    std::string stderr_file;
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

};

std::map<std::string, ProgramConfig>  parsing(char **args);

#endif