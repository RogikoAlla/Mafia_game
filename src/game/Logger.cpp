#include "game/Logger.hpp"

#include <fstream>
#include <string>

namespace mafia {

Logger::Logger(std::filesystem::path directory) : directory_(std::move(directory)) {
    std::filesystem::remove_all(directory_);
    std::filesystem::create_directories(directory_);
}

void Logger::appendRound(int round, const std::string& text) {
    const std::filesystem::path file =
        directory_ / ("round_" + std::to_string(round) + ".txt");
    std::ofstream out(file, std::ios::app);
    out << text;
}

void Logger::writeSummary(const std::string& text) {
    std::ofstream out(directory_ / "summary.txt");
    out << text;
}

}  // namespace mafia
