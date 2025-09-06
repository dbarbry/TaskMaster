#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

class Logger {
   private:
    inline static std::mutex cout_mutex;

    static std::string getTimestamp() {
        auto now  = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "," << std::setfill('0')
           << std::setw(3) << ms.count();
        return ss.str();
    }

   public:
    inline static bool silent = false;

    template <typename T>
    static void log(const T& message) {
        if (silent) return;
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << getTimestamp() << " " << message << std::endl;
    }

    template <typename T>
    static void debug(const T& message) {
        if (silent) return;
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << getTimestamp() << " [DEBUG] " << message << std::endl;
    }

    template <typename T>
    static void info(const T& message) {
        if (silent) return;
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << getTimestamp() << " [INFO] " << message << std::endl;
    }

    template <typename T>
    static void error(const T& message) {
        if (silent) return;
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << getTimestamp() << " [ERROR] " << message << std::endl;
    }

    template <typename T>
    static void warn(const T& message) {
        if (silent) return;
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << getTimestamp() << " [WARN] " << message << std::endl;
    }
};

#endif  // LOGGER_HPP