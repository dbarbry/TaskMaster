#include "main.hpp"

#define SOCKET_PATH "/tmp/taskmaster_socket"
#define BUFFER_SIZE 1024

class Shell {
   public:
    void run(int fd) {
        char *input;

        while (true) {
            input = readline("\033[34mtaskmaster\033[0m$ ");
            if (!input) {  // ctrl D
                std::cout << "Leaving..." << std::endl;
                break;
            }

            std::string cmd(input);
            free(input);

            if (!cmd.empty()) {
                add_history(cmd.c_str());
                std::string clean_cmd = parse_cmd(cmd);

                if (!analyze_cmd(fd, clean_cmd)) {
                    break;
                }
            }
        }
    }

   private:
    std::string parse_cmd(const std::string &cmd) {
        std::istringstream iss(cmd);
        std::ostringstream oss;
        std::string        word;
        bool               first = true;

        while (iss >> word) {
            if (!first) oss << " ";
            oss << word;
            first = false;
        }

        return oss.str();
    }

    bool send_cmd(int fd, const std::string &cmd) {
        char        buffer[BUFFER_SIZE] = {0};
        std::string message             = cmd + "\n";
        ssize_t     bytes_read;

        if (write(fd, message.c_str(), message.size()) <= 0) {
            perror("write failed");
            return false;
        }

        bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read < 0) {
            perror("read failed");
            return false;
        } else if (bytes_read == 0) {
            std::cerr << "Server closed the connection.\n";
            return false;
        }

        std::cout << "Server: " << buffer;
        return true;
    }

    int attach_pty(void) {
        int   master_fd = open_pty();
        int   slave_fd;
        pid_t pid;

        if (master_fd == -1) return -1;
        slave_fd = open_slave(master_fd);
        if (slave_fd == -1) {
            close(master_fd);
            return 1;
        }

        pid = fork();
        if (pid < 0) {
            perror("fork failed");
            return 1;
        }
        if (pid == 0) {
            attach_terminal(slave_fd);

            while (true) {
                std::cout << "Hey there" << std::endl;
                sleep(2);
            }
        } else {
            while (true) {
                read_from_pty(master_fd);
            }
        }

        close(master_fd);
        close(slave_fd);

        return 0;
    }

    bool analyze_cmd(int fd, const std::string &cmd) {
        std::string cleaned_cmd = parse_cmd(cmd);

        if (cleaned_cmd.empty()) return true;

        if (cmd == "help") {
            std::cout << "Server commands:" << std::endl;
            std::cout << "  status all - get the status of all services" << std::endl;
            std::cout << "  status <serviceName> - get the status of a service" << std::endl;
            std::cout << "  start <serviceName> - start a service" << std::endl;
            std::cout << "  stop <serviceName> - stop a service" << std::endl;
            std::cout << "  restart <serviceName> - restart a service" << std::endl;
            std::cout << "  reload <pathToConfigFile> - reload the configfile" << std::endl;
            std::cout << "  shutdown - shutdown taskmaster server" << std::endl;
            std::cout << std::endl << "Client commands:" << std::endl;
            std::cout << "  exit - exit the client" << std::endl << std::endl;
            return true;
        } else if (cmd == "exit") {
            std::cout << "Leaving..." << std::endl;
            return false;
        } else if (cmd == "attach") {
            if (attach_pty()) std::cout << "Attach failed" << std::endl;
            return true;
        }

        if (!send_cmd(fd, cmd)) {
            std::cout << "Daemon crashed" << std::endl;
            return false;
        }
        return true;
    }
};

void run_server(int fd) {
    struct sockaddr_un address;

    if (fd < 0) {
        perror("socket failed");
        exit(1);
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("connect failed");
        exit(1);
    }
}

int main(void) {
    int   fd = socket(AF_UNIX, SOCK_STREAM, 0);
    Shell shell;

    run_server(fd);

    std::cout << "Welcome to taskmaster client." << std::endl;
    std::cout << "Type 'help' for help." << std::endl;
    setup_signal_handlers();
    shell.run(fd);

    close(fd);
    return 0;
}
