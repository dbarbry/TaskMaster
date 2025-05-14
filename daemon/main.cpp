#include "main.hpp"

#include "config.hpp"
#include "logger.hpp"

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

void handle_client(int client_fd, int server_fd, std::map<std::string, ProgramConfig> programs) {
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
            handle_cmd(clean_buffer, server_fd, client_fd, parsedCommand, programs);

        if (!response.empty()) {
            if (write(client_fd, response.c_str(), response.size()) <= 0) {
                Logger::error("write failed: " + std::string(strerror(errno)));
                break;
            }
        }
    }

    close(client_fd);
}

void run_server(std::map<std::string, ProgramConfig> programs) {
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
            Logger::error("accept failed: " + std::string(strerror(errno)));
            continue;
        }
        Logger::info("Client connected");
        handle_client(client_fd, server_fd, programs);
    }

    close(server_fd);
    unlink(SOCKET_PATH);
}

int check_file(std::string filename) {
    std::ifstream file(filename);

    if (filename.size() < 5 || filename.substr(filename.size() - 5) != ".conf") {
        Logger::error("Must be a .conf extension file");
        return 1;
    }
    if (!std::filesystem::exists(filename)) {
        Logger::error(filename + " file doesn't exist");
        return 1;
    }
    if (!file.is_open()) {
        Logger::error("Can't open: " + filename);
        return 1;
    }

    return 0;
}

int main(int ac, char **av) {
    TaskmasterConfig config;

    if (ac != 2 || check_file(av[1])) {
        Logger::error("Usage: ./daemon.out <file.conf>");
        Logger::error("Or   :  make server <file.conf>");
        return 1;
    }
    Logger::info(".conf : " + std::string(av[1]));

    try {
        config = parse_taskmaster_conf(av[1]);
        Logger::info("Configuration file parsed successfully.");
    } catch (const std::exception &e) {
        Logger::error("Error parsing config file: " + std::string(e.what()));
        return 1;
    }

    std::map<std::string, ProgramConfig> programs = parsing(av[1]);
    log_config(programs);
    std::thread program_thread(exec_programs, programs);

    // daemonize();
    run_server(programs);

    program_thread.join();

    return 0;
}