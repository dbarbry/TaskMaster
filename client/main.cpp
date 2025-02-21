#include "main.hpp"

#define SOCKET_PATH "/tmp/taskmaster_socket"

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
        std::cerr << "No TaskMaster server found." << std::endl;
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
