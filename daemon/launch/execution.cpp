#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../cmds/service_state.hpp"
#include "../logger.hpp"
#include "./config_program.hpp"

volatile sig_atomic_t child_exited = 0;

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
    // Fonction inchangée
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
    // Fonction inchangée car elle n'utilise pas directement les méthodes renommées
    std::map<pid_t, std::string>                 pidToService;
    std::map<std::string, const ProgramConfig *> serviceConfig;

    for (const auto &[serviceName, serviceInfo] : runningServices) {
        for (const auto &process : serviceInfo.processes) {
            pid_t pid = process.pid;
            if (std::find(pids->begin(), pids->end(), pid) != pids->end()) {
                pidToService[pid] = serviceName;
            }
        }
    }

    Logger::info("[MONITOR] Started monitoring " + std::to_string(pids->size()) + " processes");

    while (!pids->empty()) {
        int   status;
        pid_t pid;

        for (auto it = pids->begin(); it != pids->end();) {
            pid = waitpid(*it, &status, WNOHANG);

            // avoid busy-wait
            if (!child_exited) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            child_exited = 0;

            if (pid > 0) {
                int exitCode = 0;
                if (WIFEXITED(status)) {
                    exitCode = WEXITSTATUS(status);
                    Logger::info("[PID " + std::to_string(pid) +
                                 "] exited with code: " + std::to_string(exitCode));
                } else if (WIFSIGNALED(status)) {
                    int signal = WTERMSIG(status);
                    Logger::info("[PID " + std::to_string(pid) +
                                 "] killed by signal: " + std::to_string(signal));
                    exitCode = 128 + signal;
                }

                if (pidToService.count(pid) > 0) {
                    std::string serviceName = pidToService[pid];

                    updateProcessState(serviceName, pid, ProcessState::STOPPED, exitCode);
                    Logger::info("[MONITOR] Service " + serviceName + " has " +
                                 std::to_string(getServiceInstanceCount(serviceName)) +
                                 " instances remaining");
                }

                it = pids->erase(it);
            } else if (pid < 0 && errno != EINTR) {  // waitpid error
                Logger::error("[MONITOR] Error in waitpid: " + std::string(strerror(errno)));
                it = pids->erase(it);
            } else {
                ++it;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Logger::info("[MONITOR] Monitoring thread finished");
}

void exec_programs(const std::map<std::string, ProgramConfig> &programs) {
    auto pids = std::make_shared<std::vector<pid_t>>();

    setup_signal_handlers();
    for (const auto &[name, config] : programs) {
        const int max_retries   = config.getStartretries();
        int       nbr_instances = config.getNumprocs();

        for (int i = 0; i < nbr_instances; i++) {
            int   retries = 0;
            pid_t pid;
            while (retries < max_retries) {
                pid = launch_program(name, config);
                if (pid > 0) break;
                retries++;
            }
            if (pid > 0) {
                pids->push_back(pid);
            }
        }
    }

    std::thread monitor_thread(monitoring, pids);
    monitor_thread.detach();
}
