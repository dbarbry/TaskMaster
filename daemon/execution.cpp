#include "./incs/execution.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

void redirect_output(const std::string &name, const std::string &stdout_file,
                     const std::string &stderr_file) {
    int stdout_fd = open(stdout_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    int stderr_fd = open(stderr_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (stdout_fd < 0) std::cerr << name << " stdout logging file failed to open.";
    if (stderr_fd < 0) std::cerr << name << " stderr logging file failed to open.";

    dup2(stdout_fd, STDOUT_FILENO);
    dup2(stderr_fd, STDERR_FILENO);

    close(stdout_fd);
    close(stderr_fd);
}

void set_environment(const std::map<std::string, std::string> &env) {
    for (const auto &[key, value] : env) {
        setenv(key.c_str(), value.c_str(), 1);
    }
}

void monitoring(std::vector<pid_t> &pids) {
    while (!pids.empty()) {
        for (auto it = pids.begin(); it != pids.end();) {
            int   status;
            pid_t result = waitpid(*it, &status, WNOHANG);

            if (result > 0) {
                if (WIFEXITED(status)) {
                    std::cout << "[PID " << *it << "] exited with code: " << WEXITSTATUS(status)
                              << std::endl;
                } else if (WIFSIGNALED(status)) {
                    std::cout << "[PID " << *it << "] killed by signal: " << WEXITSTATUS(status)
                              << std::endl;
                }
                it = pids.erase(it);
            } else {
                ++it;
            }
        }
        sleep(1);
    }
}

void exec_programs(const std::map<std::string, ProgramConfig> &programs) {
    std::vector<pid_t> pids;

    for (const auto &[name, config] : programs) {
        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "Fork failed for: " << name << std::endl;
            continue;
        }

        if (pid == 0) {
            std::cout << "Launching: " << name << " (" << config.getCmd() << ")" << std::endl;

            std::vector<std::string> args;
            std::string              cmd = config.getCmd();

            std::istringstream iss(cmd);
            std::string        arg;
            while (iss >> arg) {
                args.push_back(arg);
            }

            if (args.empty()) {
                std::cerr << "Empty command for: " << name << std::endl;
                _exit(EXIT_FAILURE);
            }

            std::vector<char *> argv;
            for (auto &s : args) argv.push_back(s.data());
            argv.push_back(nullptr);

            std::cout << "Arguments executed: " << std::endl;
            for (size_t i = 0; i < args.size(); ++i) {
                std::cout << "[" << i << "] " << args[i] << std::endl;
            }

            std::cout << "[PID " << getpid() << "] Execution of: " << argv[0] << std::endl;

            execvp(argv[0], argv.data());

            std::cerr << "Execution failed for: " << config.getCmd() << std::endl;
            _exit(EXIT_FAILURE);
        } else {
            pids.push_back(pid);
        }
    }

    for (pid_t pid : pids) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            std::cout << "[PID " << pid << "] ended with code: " << WEXITSTATUS(status)
                      << std::endl;
        } else if (WIFSIGNALED(status)) {
            std::cout << "[PID " << pid << "] killed by signal: " << WTERMSIG(status) << std::endl;
        }
    }
}
