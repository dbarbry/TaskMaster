#include <iostream>

int main(void) {
    std::string cmd;

    while(true) {
        std::cout << "taskmaster> ";
        std::getline(std::cin, cmd);
    }

    return 0;
}
