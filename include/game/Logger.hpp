#pragma once

#include <filesystem>
#include <string>

namespace mafia {

// Пишет одну партию в каталог. При создании стирает файлы прошлых партий.
// Консоль при этом не трогает.
class Logger {
public:
    explicit Logger(std::filesystem::path directory);

    void appendRound(int round, const std::string& text);
    void writeSummary(const std::string& text);

private:
    std::filesystem::path directory_;
};

}  // namespace mafia
