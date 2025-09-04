#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../cmds/service_state.hpp"
#include "../logger.hpp"
#include "./config_program.hpp"

volatile sig_atomic_t                     child_exited = 0;
extern std::mutex                         serviceMutex;
extern std::map<std::string, ServiceInfo> runningServices;

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

int execvpe_compat(const char *file, char *const argv[], char *const envp[]) {
    if (strchr(file, '/')) {
        execve(file, argv, envp);
        return -1;
    }

    const char *path = getenv("PATH");
    if (!path) path = "/usr/bin:/bin:/usr/sbin:/sbin";

    std::istringstream pathStream(path);
    std::string        dir;
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

void redirect_output(const std::string &stdout_file, const std::string &stderr_file) {
    // Ensure child does not read from daemon's stdin (be deterministic on macOS/Linux)
    int stdin_fd = open("/dev/null", O_RDONLY);
    if (stdin_fd >= 0) {
        dup2(stdin_fd, STDIN_FILENO);
        close(stdin_fd);
    }
    int stdout_fd = open(stdout_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    int stderr_fd = open(stderr_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (stdout_fd < 0) Logger::error("stdout logging file failed to open.");
    if (stderr_fd < 0) Logger::error("stderr logging file failed to open.");

    if (stdout_fd >= 0) {
        dup2(stdout_fd, STDOUT_FILENO);
        close(stdout_fd);
    }

    if (stderr_fd >= 0) {
        dup2(stderr_fd, STDERR_FILENO);
        close(stderr_fd);
    }
}

pid_t launch_program(const std::string &name, const ProgramConfig &config) {
    std::vector<std::unique_ptr<char[]>> storage, env_storage;
    std::vector<char *>                  av, envp;
    pid_t                                pid;

    pid = fork();
    if (pid < 0) {
        Logger::error("Fork failed for: " + name);
        return -1;
    }
    if (pid == 0) {  // child
        Logger::info("Launching: " + name + " (" + config.getCommand() + ")");

        mode_t mask = config.getUmask();
        umask(mask);

        if (!config.getWorkingDir().empty() && chdir(config.getWorkingDir().c_str()) != 0) {
            Logger::error("Failed to change directory to " + config.getWorkingDir());
            _exit(1);
        }

        int fd = open("/dev/null", O_RDONLY);
        if (fd >= 0) {
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        redirect_output(config.getStdoutLogfile(), config.getStderrLogfile());
        set_environment(config.getEnvironment(), env_storage, envp);
        parse_command(config.getCommand(), storage, av);

        if (av.empty()) {
            Logger::error("Empty command for: " + name);
            _exit(1);
        }

        Logger::info("[PID " + std::to_string(getpid()) + "] Executing: " + std::string(av[0]));
        execvpe_compat(av[0], av.data(), envp.data());

        int err = errno;
        Logger::error("Execution failed for: " + config.getCommand() + " (Error: " + strerror(err) +
                      ")");
        _exit(1);
    }

    return pid;
}

void monitoring(std::shared_ptr<std::vector<pid_t>> pids) {
    Logger::info("[MONITOR] Monitoring thread started");

    while (true) {
        pid_t pid;
        int   status;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            std::lock_guard<std::mutex> lock(serviceMutex);

            auto it = std::find(pids->begin(), pids->end(), pid);
            if (it != pids->end()) pids->erase(it);

            int exitCode = WIFEXITED(status)     ? WEXITSTATUS(status)
                           : WIFSIGNALED(status) ? 128 + WTERMSIG(status)
                                                 : -1;

            for (auto &[serviceName, service] : runningServices) {
                for (auto &process : service.processes) {
                    if (process.pid == pid) {
                        process.state    = ProcessState::STOPPED;
                        process.exitCode = exitCode;
                        Logger::info("[MONITOR] Service " + serviceName + " has " +
                                     std::to_string(getServiceInstanceCount(serviceName)) +
                                     " instances remaining");
                    }
                }
            }
        }

        if (pid < 0 && errno != EINTR && errno != ECHILD) {
            Logger::error("[MONITOR] waitpid error: " + std::string(strerror(errno)));
        }

        {
            std::lock_guard<std::mutex> lock(serviceMutex);
            for (auto &[serviceName, service] : runningServices) {
                for (auto &process : service.processes) {
                    if (process.state == ProcessState::STARTING) {
                        time_t elapsed = std::time(nullptr) - process.startTime;
                        if (elapsed >= service.startsecs) {
                            if (kill(process.pid, 0) == 0) {  // still alive
                                process.state = ProcessState::RUNNING;
                                Logger::info(serviceName + ": process " +
                                             std::to_string(process.pid) +
                                             " has reached RUNNING state after " +
                                             std::to_string(elapsed) + "s");
                            } else {
                                process.state = ProcessState::STOPPED;
                                Logger::info(serviceName + ": process " +
                                             std::to_string(process.pid) +
                                             " exited before reaching RUNNING");
                            }
                        }
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));  // slightly faster monitoring
    }
}

void exec_programs(const std::map<std::string, ProgramConfig> &programs) {
    auto pids = std::make_shared<std::vector<pid_t>>();

    setup_signal_handlers();
    for (const auto &[name, config] : programs) {
        const int max_retries   = config.getStartretries();
        int       nbr_instances = config.getNumprocs();

        for (const auto &[name, config] : programs) {
            int runningCount = getServiceInstanceCount(name);
            int remaining    = config.getNumprocs() - runningCount;

            if (remaining <= 0) {
                Logger::info(name + ": already has " + std::to_string(runningCount) +
                             " instance(s), skipping launch.");
                continue;

                Logger::info("Autostarting " + std::to_string(remaining) + " instance(s) of " +
                             name);

                for (int i = 0; i < remaining; i++) {
                    int   retries = 0;
                    pid_t pid     = -1;

                    while (retries < config.getStartretries()) {
                        pid = launch_program(name, config);
                        if (pid > 0) break;  // success
                        retries++;
                        Logger::warn(name + ": retrying launch (" + std::to_string(retries) + "/" +
                                     std::to_string(config.getStartretries()) + ")");
                    }

                    if (pid > 0) {
                        std::lock_guard<std::mutex> lock(serviceMutex);
                        addServicePid(name, pid, config.getStartsecs());
                        Logger::info("Launched " + name + " with PID " + std::to_string(pid));
                        pids->push_back(pid);
                        updateServiceState(name, ProcessState::STARTING);
                    } else {
                        Logger::error("Failed to start " + name + " after " +
                                      std::to_string(config.getStartretries()) + " retries.");
                        if (getServiceInstanceCount(name) == 0) {
                            updateServiceState(name, ProcessState::FATAL);
                        }
                    }
                }
            }

            std::thread monitor_thread(monitoring, pids);
            monitor_thread.detach();
        }
    }
}
