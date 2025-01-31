#include "./incs/execution.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <csignal>
#include <vector>

volatile sig_atomic_t child_exited = 0;

std::vector<char *> parse_command(const std::string &cmd) {
    std::vector<std::string> args;
    std::istringstream       iss(cmd);
    std::string              arg;
    std::vector<char *>      av;

    while (iss >> arg) {
        args.push_back(arg);
    }

    for (auto &s : args) av.push_back(s.data());
    av.push_back(nullptr);

    return av;
}

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

void sigchld_handler(int sig) {
    (void)sig;
    child_exited = 1;
}

void setup_signal_handlers() {
    struct sigaction sa;

    sa.sa_handler = sigchld_handler;
    sa.sa_flags   = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, nullptr);
}

pid_t launch_program(const std::string &name, const ProgramConfig &config) {
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        std::cerr << "Fork failed for: " << name << std::endl;
        return -1;
    }
    if (pid == 0) {
        std::cout << "Launching: " << name << " (" << config.getCmd() << ")" << std::endl;

        if (!config.getWorkingDir().empty() && chdir(config.getWorkingDir().c_str()) != 0) {
            std::cerr << "Failed to change directory to " << config.getWorkingDir() << std::endl;
            exit(1);
        }

        redirect_output(name, config.getStdoutFile(), config.getStderrFile());
        set_environment(config.getEnv());

        std::vector<char *> av = parse_command(config.getCmd());

        if (av.empty()) {
            std::cerr << "Empty command for: " << name << std::endl;
            exit(1);
        }

        std::cout << "[PID " << getpid() << "] Executing: " << av[0] << std::endl;
        execvp(av[0], av.data());

        std::cerr << "Execution failed for: " << config.getCmd() << std::endl;
        exit(1);
    }

    return pid;
}

void monitoring(std::vector<pid_t> &pids) {
    while (!pids.empty()) {
        if (child_exited) {
            child_exited = 0;

            int   status;
            pid_t pid;
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                if (WIFEXITED(status)) {
                    std::cout << "[PID " << pid << "] exited with code: " << WEXITSTATUS(status)
                              << std::endl;
                } else if (WIFSIGNALED(status)) {
                    std::cout << "[PID " << pid << "] killed by signal: " << WTERMSIG(status)
                              << std::endl;
                }

                auto it = std::find(pids.begin(), pids.end(), pid);
                if (it != pids.end()) {
                    pids.erase(it);
                }
            }
        }
        pause();
    }
}

void exec_programs(const std::map<std::string, ProgramConfig> &programs) {
    std::vector<pid_t> pids;

    setup_signal_handlers();
    for (const auto &[name, config] : programs) {
        const int max_retries = config.getStartretries();
        int       retries     = 0;
        pid_t     pid;
        int       status;

        while (retries < max_retries) {
            pid = launch_program(name, config);
            if (pid < 0) break;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                std::cout << "[PID " << pid << "] ended with code: " << exit_code << std::endl;

                const std::vector<int> &valid_exit_codes = config.getExitcodes();
                if (std::find(valid_exit_codes.begin(), valid_exit_codes.end(), exit_code) !=
                    valid_exit_codes.end()) {
                    std::cout << "[PID " << pid << "] Exit code is allowed, no restart needed."
                              << std::endl;
                    break;
                }

                std::cout << "[PID " << pid << "] Unexpected exit code, restarting..." << std::endl;
            } else if (WIFSIGNALED(status)) {
                std::cout << "[PID " << pid << "] killed by signal: " << WTERMSIG(status)
                          << std::endl;
            }

            retries++;
            if (retries < max_retries) {
                std::cerr << "Restarting " << name << " (" << retries << "/" << max_retries << ")"
                          << std::endl;
            } else {
                std::cerr << "Max retries reached for " << name << ", giving up." << std::endl;
            }
        }

        pids.push_back(pid);
    }

    monitoring(pids);
}
