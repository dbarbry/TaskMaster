#include "./incs/main.hpp"

#include <filesystem>

#include "./incs/parsing.hpp"
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
        perror("fork failed");
        exit(1);
    }
    if (pid > 0) exit(0);

    if (setsid() < 0) {
        perror("setsid failed");
        exit(1);
    }

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }
    if (pid > 0) {
        exit(0);
    }

    umask(0);

    if (chdir("/") < 0) {
        perror("chdir failed");
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

void handle_client(int client_fd, int server_fd) {
    char    buffer[BUFFER_SIZE];
    ssize_t read_len;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        read_len = read(client_fd, buffer, BUFFER_SIZE - 1);

        if (read_len < 0) {
            perror("read failed");
            break;
        }
        if (!read_len) {
            std::cout << "Client disconnected" << std::endl;
            break;
        }
        std::cout << "Received: " << buffer;
        std::string clean_buffer(buffer);

        std::string response = handle_cmd(clean_buffer, server_fd);
        if (write(client_fd, response.c_str(), response.size()) <= 0) {
            perror("write failed");
            break;
        }
    }

    close(client_fd);
}

void run_server(void) {
    int                server_fd, client_fd;
    struct sockaddr_un address;

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(1);
    }

    unlink(SOCKET_PATH);

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is running as a daemon and listening on " << SOCKET_PATH << std::endl;

    while (true) {
        if ((client_fd = accept(server_fd, 0, 0)) < 0) {
            perror("accept failed");
            continue;
        }

        std::cout << "New client connected" << std::endl;

        handle_client(client_fd, server_fd);
    }

    close(server_fd);
    unlink(SOCKET_PATH);
}

int check_file(std::string filename) {
    std::ifstream file(filename);

    if (filename.size() < 5 || filename.substr(filename.size() - 5) != ".conf") {
        std::cout << "Must be a .conf extension file." << std::endl;
        return 1;
    }
    if (!std::filesystem::exists(filename)) {
        std::cout << filename << " file doesn't exist." << std::endl;
        return 1;
    }
    if (!file.is_open()) {
        std::cout << "Can't open: " << filename << std::endl;
        return 1;
    }

    return 0;
}

int main(int ac, char **av) {
    if (ac != 2 || check_file(av[1])) {
        std::cerr << "Usage: ./daemon.out <file.conf>" << std::endl;
        std::cerr << "Or   :  make server <file.conf>" << std::endl;
        return 1;
    }

    std::cout << ".conf : " << av[1] << std::endl;
    std::map<std::string, ProgramConfig> programs = parsing(av[1]);
    log_config(programs);
    exec_programs(programs);

    // daemonize();
    run_server();

    return 0;
}
