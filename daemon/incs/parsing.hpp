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
};

void parsing(char **args);

#endif