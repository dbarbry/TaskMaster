#include "../cmds/service_state.hpp"
#include "launch.hpp"

std::map<std::string, int> active_programs;
volatile sig_atomic_t      child_exited = 0;

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

void redirect_output(int pty_fd, const std::string &stdout_file, const std::string &stderr_file) {
    if (pty_fd >= 0) {
        return;
    }

    int stdout_fd = open(stdout_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    int stderr_fd = open(stderr_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (stdout_fd < 0) std::cerr << "stdout logging file failed to open." << std::endl;
    if (stderr_fd < 0) std::cerr << "stderr logging file failed to open." << std::endl;

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
    int                                  master_fd, slave_fd;

    if (openpty(&master_fd, &slave_fd, nullptr, nullptr, nullptr) == -1) {
        std::cerr << "Failed to create PTY for: " << name << std::endl;
        return -1;
    }

    pid = fork();
    if (pid < 0) {
        std::cerr << "Fork failed for: " << name << std::endl;
        return -1;
    }
    if (pid == 0) {
        std::cout << "Launching: " << name << " (" << config.getCmd() << ")" << std::endl;

        close(master_fd);
        setsid();
        ioctl(slave_fd, TIOCSCTTY, 0);

        if (!config.getWorkingDir().empty() && chdir(config.getWorkingDir().c_str()) != 0) {
            std::cerr << "Failed to change directory to " << config.getWorkingDir() << std::endl;
            _exit(1);
        }

        redirect_output(slave_fd, config.getStdoutFile(), config.getStderrFile());
        close(slave_fd);

        set_environment(config.getEnv(), env_storage, envp);
        parse_command(config.getCmd(), storage, av);

        if (av.empty()) {
            std::cerr << "Empty command for: " << name << std::endl;
            _exit(1);
        }

        std::cout << "[PID " << getpid() << "] Executing: " << av[0] << std::endl;
        execvpe_compat(av[0], av.data(), envp.data());
        std ::cout << "[PID " << getpid() << "] Executing: " << av[0] << av.data() << std::endl;

        int err = errno;
        std::cerr << "Execution failed for: " << config.getCmd() << " (Error: " << strerror(err)
                  << ")\n";
        _exit(1);
    }
    close(slave_fd);
    {
        // std::lock_guard<std::mutex> lock(serviceMutex);
        if (active_programs.count(name) > 0) {
            std::cout << "[PTY] Closing previous PTY for " << name << std::endl;
            close(active_programs[name]);
            active_programs.erase(name);
        }
    }
    std::cout << "[SERVER] Storing PTY FD for: " << name << " (FD: " << master_fd << ")"
              << std::endl;
    active_programs[name] = master_fd;

    return pid;
}

void monitoring(std::shared_ptr<std::vector<pid_t>> pids) {
    // Map pour associer les PIDs aux noms de services et configurations
    std::map<pid_t, std::string>                 pidToService;
    std::map<std::string, const ProgramConfig *> serviceConfig;

    {
        for (const auto &[serviceName, serviceInfo] : runningServices) {
            for (const auto &process : serviceInfo.processes) {
                pid_t pid = process.pid;
                if (std::find(pids->begin(), pids->end(), pid) != pids->end()) {
                    pidToService[pid] = serviceName;
                }
            }
        }
    }

    std::cout << "[MONITOR] Started monitoring " << pids->size() << " processes" << std::endl;

    while (!pids->empty()) {
        int   status;
        pid_t pid;

        for (auto it = pids->begin(); it != pids->end();) {
            pid = waitpid(*it, &status, WNOHANG);

            if (pid > 0) {
                // Processus terminé
                int exitCode = 0;
                if (WIFEXITED(status)) {
                    exitCode = WEXITSTATUS(status);
                    std::cout << "[PID " << pid << "] exited with code: " << exitCode << std::endl;
                } else if (WIFSIGNALED(status)) {
                    int signal = WTERMSIG(status);
                    std::cout << "[PID " << pid << "] killed by signal: " << signal << std::endl;
                    exitCode = 128 + signal;  // Convention pour les signaux
                }

                // Mettre à jour l'état du service
                if (pidToService.count(pid) > 0) {
                    std::string serviceName = pidToService[pid];

                    // Nettoyage du PTY associé à ce processus
                    {
                        // std::lock_guard<std::mutex> lock(serviceMutex);
                        if (active_programs.count(serviceName) > 0) {
                            close(active_programs[serviceName]);
                            active_programs.erase(serviceName);
                            std::cout << "[PTY] Closed PTY for service: " << serviceName
                                      << std::endl;
                        }
                    }

                    updateProcessState(serviceName, pid, ProcessState::STOPPED, exitCode);
                    std::cout << "[MONITOR] Service " << serviceName << " has "
                              << getServiceInstanceCount(serviceName) << " instances remaining"
                              << std::endl;
                }

                it = pids->erase(it);
            } else if (pid < 0 && errno != EINTR) {
                // Erreur avec waitpid
                std::cerr << "[MONITOR] Error in waitpid: " << strerror(errno) << std::endl;
                it = pids->erase(it);
            } else {
                ++it;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[MONITOR] Monitoring thread finished" << std::endl;
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
                if (pid < 0) break;
                break;
            }
            pids->push_back(pid);
        }
    }

    std::thread monitor_thread(monitoring, pids);
    monitor_thread.detach();
}
