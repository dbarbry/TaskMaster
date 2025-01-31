#include <iostream>

int main() {
    std::cout << "This program will segfault!" << std::endl;
    int *ptr = nullptr;
    *ptr = 42;  // Crash ici !
    return 0;
}