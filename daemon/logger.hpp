// Dans un fichier global (par exemple, logger.hpp)
#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <mutex>
#include <string>

class Logger {
   private:
    inline static std::mutex cout_mutex;

   public:
    template <typename T>
    static void log(const T& message) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << message << std::endl;
    }

    template <typename T>
    static void debug(const T& message) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[DEBUG] " << message << std::endl;
    }

    template <typename T>
    static void info(const T& message) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[INFO] " << message << std::endl;
    }

    template <typename T>
    static void error(const T& message) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "[ERROR] " << message << std::endl;
    }
};

#endif  // LOGGER_HPP