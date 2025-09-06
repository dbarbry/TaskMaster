#include "logger.hpp"
#include "main.hpp"

#define BUFFER_SIZE 1024

void apply_runtime_settings(TaskmasterConfig &config) {
    if (config.environment.has_value()) {
        for (const auto &[key, value] : config.environment.value()) {
            if (setenv(key.c_str(), value.c_str(), 1) != 0) {
                Logger::error("Failed to set environment variable: " + key);
                std::exit(EXIT_FAILURE);
            }
        }
    }
    umask(config.umask);

    if (config.directory.has_value()) {
        if (chdir(config.directory->c_str()) < 0) {
            Logger::error("chdir failed: " + std::string(strerror(errno)));
            exit(EXIT_FAILURE);
        }
    } else {
        if (chdir("/") < 0) {
            Logger::error("chdir failed: " + std::string(strerror(errno)));
            exit(EXIT_FAILURE);
        }
    }

    if (config.user.has_value()) {
        struct passwd *pw = getpwnam(config.user->c_str());
        if (!pw) {
            Logger::error("Invalid user in config: " + *config.user);
            exit(EXIT_FAILURE);
        }
        if (setgid(pw->pw_gid) != 0 || initgroups(pw->pw_name, pw->pw_gid) != 0 ||
            setuid(pw->pw_uid) != 0) {
            Logger::error("Failed to drop privileges to user " + *config.user + ": " +
                          strerror(errno));
            exit(EXIT_FAILURE);
        }
        Logger::info("Running as user: " + *config.user);
    }
}

std::string get_logfile_name(TaskmasterConfig &config) {
    time_t      now       = time(nullptr);
    struct tm  *time_info = localtime(&now);
    std::string logfile_name;
    char        date_str[64];

    strftime(date_str, sizeof(date_str), "log-%Y_%m_%d-daemon.txt", time_info);
    logfile_name = config.logfile;
    if (!logfile_name.empty() && logfile_name.back() != '/') logfile_name += '/';
    logfile_name += date_str;

    Logger::info("Logfile located at: " + logfile_name);

    return logfile_name;
}

void daemonize(TaskmasterConfig &config) {
    pid_t       pid;
    int         log_fd;
    std::string logfile_name;

    pid = fork();
    if (pid < 0) {
        Logger::error("fork failed: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }
    if (pid > 0) exit(0);

    if (setsid() < 0) {
        Logger::error("setsid failed: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    logfile_name = get_logfile_name(config);
    pid          = fork();
    if (pid < 0) {
        Logger::error("fork failed: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }
    if (pid > 0) exit(0);

    unlink(config.pidfile.c_str());  // in case meh
    std::ofstream pidf(config.pidfile);
    if (!pidf) {
        Logger::error("Cannot write PID file: " + config.pidfile);
        exit(EXIT_FAILURE);
    }
    pidf << getpid() << std::endl;
    pidf.close();

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    log_fd = open(logfile_name.c_str(), O_WRONLY | O_CREAT | O_APPEND, config.chmod);
    if (log_fd < 0) {
        Logger::error("Failed to open log file: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }

    if (config.chown.has_value()) {
        if (chown(logfile_name.c_str(), config.get_socket_uid().value_or(getuid()),
                  config.get_socket_gid().value_or(getgid())) < 0) {
            Logger::error("chown failed: " + std::string(strerror(errno)));
            close(log_fd);
            exit(EXIT_FAILURE);
        }
    }

    dup2(log_fd, STDOUT_FILENO);
    dup2(log_fd, STDERR_FILENO);
    close(log_fd);
}

void handle_client(int client_fd, int server_fd, TaskmasterConfig &config) {
    char    buffer[BUFFER_SIZE];
    ssize_t read_len;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        read_len = read(client_fd, buffer, BUFFER_SIZE - 1);

        if (read_len < 0) {
            Logger::error("read failed: " + std::string(strerror(errno)));
            break;
        }
        if (!read_len) {
            Logger::info("Client disconnected");
            break;
        }
        Logger::debug("Received: " + std::string(buffer));
        std::map<std::string, std::vector<std::string>> parsedCommand = commandParsing(buffer);
        std::string                                     clean_buffer(buffer);
        std::string                                     response =
            handle_cmd(clean_buffer, server_fd, client_fd, parsedCommand, config);

        if (!response.empty()) {
            if (write(client_fd, response.c_str(), response.size()) <= 0) {
                Logger::error("write failed: " + std::string(strerror(errno)));
                break;
            }
        }
    }

    close(client_fd);
}

void run_server(TaskmasterConfig &config) {
    int                server_fd, client_fd;
    struct sockaddr_un address;
    const std::string  socket_path = config.file;

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        Logger::error("socket failed: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }

    unlink(socket_path.c_str());

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, socket_path.c_str(), sizeof(address.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        Logger::error("bind failed: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }

    if (chmod(config.file.c_str(), config.chmod) < 0) {
        Logger::error("chmod failed: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }
    if (config.chown.has_value()) {
        if (chown(config.file.c_str(), config.get_socket_uid().value_or(getuid()),
                  config.get_socket_gid().value_or(getgid())) < 0) {
            Logger::error("chown failed: " + std::string(strerror(errno)));
            exit(EXIT_FAILURE);
        }
    }

    if (listen(server_fd, 5) < 0) {
        Logger::error("listen failed: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }

    Logger::info("Server started, waiting for connections");

    while (true) {
        if ((client_fd = accept(server_fd, 0, 0)) < 0) {
            int err = errno;
            if (err == EINTR) {
                continue;
            }
            if (err == EBADF || err == EINVAL) {
                Logger::info("Server socket closed, stopping accept loop");
                break;
            }
            Logger::error("accept failed: " + std::string(strerror(err)));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        Logger::info("Client connected");
        handle_client(client_fd, server_fd, config);
    }

    if (server_fd >= 0) close(server_fd);
    unlink(socket_path.c_str());
    unlink(config.pidfile.c_str());
}
