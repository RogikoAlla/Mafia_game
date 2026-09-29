#include "game/Host.hpp"
#include "game/Player.hpp"

#include "smart_ptr/SharedPtr.hpp"

#include <clocale>
#include <iostream>
#include <memory>
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
    mafia::Host host(state);
    host.dealRoles();

    std::vector<std::unique_ptr<mafia::Player>> roster;
    roster.reserve(kPlayers);
    for (int playerId = 1; playerId <= kPlayers; ++playerId) {
        roster.push_back(mafia::makePlayer(state, playerId));
    }

    std::vector<std::thread> players;
    players.reserve(kPlayers);
    for (const std::unique_ptr<mafia::Player>& player : roster) {
        players.emplace_back(&mafia::Player::run, player.get(), kRounds);
    }

    std::thread hostThread(&mafia::Host::run, &host, kRounds);

    hostThread.join();
    for (std::thread& player : players) {
        player.join();
    }
    return 0;
}
