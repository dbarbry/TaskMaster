#include "./incs/execution.hpp"

#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <csignal>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

volatile sig_atomic_t child_exited = 0;

int execvpe_compat(const char *file, char *const argv[], char *const envp[]) {
    if (strchr(file, '/')) {
        execve(file, argv, envp);
        return -1;
    }

    const char *path = getenv("PATH");
    if (!path) path = "/usr/bin:/bin:/usr/sbin:/sbin";

    std::istringstream pathStream(path);
    std::string dir;
    while (std::getline(pathStream, dir, ':')) {
        std::string fullPath = dir + "/" + file;
        execve(fullPath.c_str(), argv, envp);
    }

    return -1;
}

void parse_command(const std::string &cmd, std::vector<std::unique_ptr<char[]>> &storage,
                   std::vector<char *> &av) {
    storage.clear();
    av.clear();

    std::vector<std::string> args;
    std::istringstream       iss(cmd);
    std::string              token;
    std::string              current_arg;

    while (iss >> std::ws) {
        char c = iss.peek();

        if (c == '"' || c == '\'') {
            char quote = iss.get();
            current_arg.clear();
            while (iss.get(c)) {
                if (c == quote) break;
                current_arg += c;
            }
            args.push_back(current_arg);
        } else {
            iss >> token;
            args.push_back(token);
        }
    }

    for (auto &s : args) {
        storage.push_back(std::make_unique<char[]>(s.size() + 1));
        std::strcpy(storage.back().get(), s.c_str());
        av.push_back(storage.back().get());
    }
    av.push_back(nullptr);
}

void redirect_output(const std::string &name, const std::string &stdout_file,
                     const std::string &stderr_file) {
    int stdout_fd = open(stdout_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    int stderr_fd = open(stderr_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (stdout_fd < 0) std::cerr << name << " stdout logging file failed to open.";
    if (stderr_fd < 0) std::cerr << name << " stderr logging file failed to open.";

    if (stdout_fd >= 0 && dup2(stdout_fd, STDOUT_FILENO) < 0) {
        std::cerr << "dup2 failed for stdout: " << strerror(errno) << std::endl;
    }
    if (stderr_fd >= 0 && dup2(stderr_fd, STDERR_FILENO) < 0) {
        std::cerr << "dup2 failed for stderr: " << strerror(errno) << std::endl;
    }
    close(stdout_fd);
    close(stderr_fd);
}

void set_environment(const std::map<std::string, std::string> &env,
                     std::vector<std::unique_ptr<char[]>> &storage, std::vector<char *> &envp) {
    storage.clear();
    envp.clear();

    for (const auto &[key, value] : env) {
        std::string env_entry = key + "=" + value;
        storage.push_back(std::make_unique<char[]>(env_entry.size() + 1));
        std::strcpy(storage.back().get(), env_entry.c_str());
        envp.push_back(storage.back().get());
    }
    envp.push_back(nullptr);
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
    pid_t                                pid;
    std::vector<std::unique_ptr<char[]>> storage;
    std::vector<std::unique_ptr<char[]>> env_storage;
    std::vector<char *>                  av;
    std::vector<char *>                  envp;

    pid = fork();
    if (pid < 0) {
        std::cerr << "Fork failed for: " << name << std::endl;
        return -1;
    }
    if (pid == 0) {
        std::cout << "Launching: " << name << " (" << config.getCmd() << ")" << std::endl;

        if (!config.getWorkingDir().empty() && chdir(config.getWorkingDir().c_str()) != 0) {
            std::cerr << "Failed to change directory to " << config.getWorkingDir() << std::endl;
            _exit(1);
        }

        redirect_output(name, config.getStdoutFile(), config.getStderrFile());

        set_environment(config.getEnv(), env_storage, envp);

        parse_command(config.getCmd(), storage, av);
        if (av.empty()) {
            std::cerr << "Empty command for: " << name << std::endl;
            _exit(1);
        }


        std::cout << "[PID " << getpid() << "] Executing: " << av[0] << std::endl;
        execvpe_compat(av[0], av.data(), envp.data());
        std ::cout << "[PID " << getpid() << "] Executing: " << av[0] << av.data()  << std::endl;

        int err = errno;
        std::cerr << "Execution failed for: " << config.getCmd() << " (Error: " << strerror(err)
                  << ")\n";
        _exit(1);
    }

    return pid;
}

void monitoring(std::shared_ptr<std::vector<pid_t>> pids) {
    while (!pids->empty()) {
        int status;
        pid_t pid;
        for (auto it = pids->begin(); it != pids->end();) {
            pid = waitpid(*it, &status, WNOHANG);
            if (pid > 0) {
                if (WIFEXITED(status)) {
                    std::cout << "[PID " << pid << "] exited with code: " << WEXITSTATUS(status) << std::endl;
                } else if (WIFSIGNALED(status)) {
                    std::cout << "[PID " << pid << "] killed by signal: " << WTERMSIG(status) << std::endl;
                }
                it = pids->erase(it);
            } else {
                ++it;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void exec_programs(const std::map<std::string, ProgramConfig> &programs) {
    auto pids = std::make_shared<std::vector<pid_t>>();

    setup_signal_handlers();
    for (const auto &[name, config] : programs) {
        const int max_retries   = config.getStartretries();
        int       nbr_instances = config.getNumprocs();

        for (int i = 0; i < nbr_instances; i++) {
            int retries = 0;
            pid_t pid;
            while (retries < max_retries) {
                pid = launch_program(name, config);
                if (pid < 0) break;
                break;
            }
            pids->push_back(pid);
        }
    }

    std::thread monitor_thread(monitoring, pids);
    monitor_thread.detach();
}


