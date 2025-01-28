#ifndef PARSING_HPP
#define PARSING_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <sstream>

class ProgramConfig {
public:
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

    bool isValid() const {
        if (cmd.empty()) {
            std::cerr << "Error: cmd is not set." << std::endl;
            return false;
        }
        if (numprocs <= 0) {
            std::cerr << "Error: numprocs should be a positive integer." << std::endl;
            return false;
        }
        if (umask.empty()) {
            std::cerr << "Error: umask is not set." << std::endl;
            return false;
        }
        if (workingdir.empty()) {
            std::cerr << "Error: workingdir is not set." << std::endl;
            return false;
        }
        if (autorestart.empty()) {
            std::cerr << "Error: autorestart is not set." << std::endl;
            return false;
        }
        if (startretries < 0) {
            std::cerr << "Error: startretries should be a non-negative integer." << std::endl;
            return false;
        }
        if (starttime < 0) {
            std::cerr << "Error: starttime should be a non-negative integer." << std::endl;
            return false;
        }
        if (stopsignal.empty()) {
            std::cerr << "Error: stopsignal is not set." << std::endl;
            return false;
        }
        if (stoptime < 0) {
            std::cerr << "Error: stoptime should be a non-negative integer." << std::endl;
            return false;
        }
        if (stdout_file.empty()) {
            std::cerr << "Error: stdout file is not set." << std::endl;
            return false;
        }
        if (stderr_file.empty()) {
            std::cerr << "Error: stderr file is not set." << std::endl;
            return false;
        }
        
        return true;
    }

    void printConfig() const {
        std::cout << "cmd: " << cmd << std::endl;
        std::cout << "numprocs: " << numprocs << std::endl;
        std::cout << "umask: " << umask << std::endl;
        std::cout << "workingdir: " << workingdir << std::endl;
        std::cout << "autostart: " << (autostart ? "true" : "false") << std::endl;
        std::cout << "autorestart: " << autorestart << std::endl;
        std::cout << "startretries: " << startretries << std::endl;
        std::cout << "starttime: " << starttime << std::endl;
        std::cout << "stopsignal: " << stopsignal << std::endl;
        std::cout << "stoptime: " << stoptime << std::endl;
        std::cout << "stdout: " << stdout_file << std::endl;
        std::cout << "stderr: " << stderr_file << std::endl;
        for (const auto& env_var : env) {
            std::cout << "env: " << env_var.first << "=" << env_var.second << std::endl;
        }
    }

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

};

void parsing(char **args);

#endif