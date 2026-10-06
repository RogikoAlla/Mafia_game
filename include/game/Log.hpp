#pragma once

#include <iostream>
#include <mutex>
#include <string>

namespace mafia {

// Одна строка целиком. Пока поток печатает, остальные ждут, поэтому фразы не режутся.
inline void logLine(const std::string& line) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    std::cout << line << '\n';
}

}  // namespace mafia
