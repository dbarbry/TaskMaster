#include "main.hpp"

#define DEFAULT_SOCKET_PATH "/tmp/taskmasterd.sock"

std::string ask_socket_path() {
    std::string input;
    std::cout << "Enter socket path [default: " << DEFAULT_SOCKET_PATH << "]: ";
    std::getline(std::cin, input);

    if (input.empty()) {
        std::cout << "Using default socket path: " << DEFAULT_SOCKET_PATH << "\n";
        return DEFAULT_SOCKET_PATH;
    }
    return input;
}

void run_server(int fd, const std::string &socket_path) {
    struct sockaddr_un address;

    if (fd < 0) {
        perror("socket failed");
        exit(1);
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, socket_path.c_str(), sizeof(address.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "No TaskMaster server found at " << socket_path << std::endl;
        std::cerr << "You may need to be added in the taskmaster group: " << std::endl;
        std::cerr << "  sudo usermod -aG taskmaster <your_name> " << std::endl;
        std::cerr << "  Then reload your terminal " << std::endl;
        exit(1);
    }
}

int main() {
    int         fd = socket(AF_UNIX, SOCK_STREAM, 0);
    Shell       shell;
    std::string socket_path = ask_socket_path();

    run_server(fd, socket_path);

    std::cout << "Welcome to taskmaster client." << std::endl;
    std::cout << "Type 'help' for help." << std::endl;
    setup_signal_handlers();
    shell.run(fd);

    close(fd);
    return 0;
}