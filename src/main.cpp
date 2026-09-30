#include "game/Host.hpp"
#include "game/Player.hpp"

#include "smart_ptr/SharedPtr.hpp"

#include <clocale>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Исходники и литералы в UTF-8, консоль Windows по умолчанию в OEM (866).
    std::setlocale(LC_CTYPE, ".UTF-8");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // Без флагов: 5 игроков, все программные, объявления закрытые, лог краткий.
    int playerCount = 5;
    bool interactive = false;
    bool openAnnouncements = false;
    bool fullLog = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--players" && i + 1 < argc) {
            playerCount = std::atoi(argv[++i]);
        } else if (arg == "--interactive") {
            interactive = true;
        } else if (arg == "--open-announcements") {
            openAnnouncements = true;
        } else if (arg == "--full-log") {
            fullLog = true;
        }
    }

    constexpr int kRounds = 2;

    mafia::SharedPtr<mafia::GameState> state(new mafia::GameState(playerCount));
    state->setOpenAnnouncements(openAnnouncements);
    state->setFullLog(fullLog);

    mafia::Host host(state);
    std::random_device device;
    std::mt19937 generator(device());
    host.dealRoles(generator);

    std::cout << "игроков: " << playerCount;
    if (interactive) {
        std::cout << ", человек за игроком 1";
    } else {
        std::cout << ", все игроки программные";
    }
    std::cout << ", объявления " << (openAnnouncements ? "открытые" : "закрытые");
    std::cout << ", лог " << (fullLog ? "полный" : "краткий") << '\n';

    std::vector<std::unique_ptr<mafia::Player>> roster;
    roster.reserve(playerCount);
    for (int playerId = 1; playerId <= playerCount; ++playerId) {
        const bool human = interactive && playerId == 1;
        roster.push_back(mafia::makePlayer(state, playerId, human));
    }

    std::vector<std::thread> players;
    players.reserve(playerCount);
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
