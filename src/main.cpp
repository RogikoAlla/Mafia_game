#include "game/Host.hpp"
#include "game/Player.hpp"

#include "smart_ptr/SharedPtr.hpp"

#include <clocale>
#include <iostream>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

int main() {
#ifdef _WIN32
    // Исходники и литералы в UTF-8, консоль Windows по умолчанию в OEM (866).
    std::setlocale(LC_CTYPE, ".UTF-8");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    constexpr int kPlayers = 5;
    constexpr int kRounds = 2;

    mafia::SharedPtr<mafia::GameState> state(new mafia::GameState(kPlayers));

    std::vector<std::thread> players;
    players.reserve(kPlayers);
    for (int playerId = 1; playerId <= kPlayers; ++playerId) {
        players.emplace_back(mafia::runPlayer, state, playerId, kRounds);
    }

    std::thread host(mafia::runHost, state, kRounds);

    host.join();
    for (std::thread& player : players) {
        player.join();
    }
    return 0;
}
