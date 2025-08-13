#include "logger.hpp"
#include "main.hpp"

#define LOG_PATH "/home/dhaya/taskmaster/log"
#define SOCKET_PATH "/tmp/taskmaster_socket"
#define BUFFER_SIZE 1024

void daemonize(void) {
    char        log_filename[256];
    std::string path_filename;
    struct tm  *time_info;
    int         log_fd;
    pid_t       pid;
    time_t      now;

    pid = fork();
    if (pid < 0) {
        Logger::error("fork failed: " + std::string(strerror(errno)));
        exit(1);
    }
    if (pid > 0) exit(0);

    if (setsid() < 0) {
        Logger::error("setsid failed: " + std::string(strerror(errno)));
        exit(1);
    }

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0) {
        Logger::error("fork failed: " + std::string(strerror(errno)));
        exit(1);
    }
    if (pid > 0) {
        exit(0);
    }

    umask(0);

    if (chdir("/") < 0) {
        Logger::error("chdir failed: " + std::string(strerror(errno)));
        exit(1);
    }

    for (int fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--) {
        close(fd);
    }

    now       = time(nullptr);
    time_info = localtime(&now);
    strftime(log_filename, sizeof(log_filename), "/log-%Y_%m_%d-daemon.txt", time_info);
    path_filename.append(LOG_PATH);
    path_filename.append(log_filename);
    log_fd = open(path_filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (log_fd < 0) exit(1);

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

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        Logger::error("socket failed: " + std::string(strerror(errno)));
        exit(1);
    }

    unlink(SOCKET_PATH);

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        Logger::error("bind failed: " + std::string(strerror(errno)));
        exit(EXIT_FAILURE);
    }
    chmod(SOCKET_PATH, 0777);

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
    unlink(SOCKET_PATH);
}
