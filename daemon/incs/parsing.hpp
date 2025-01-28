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