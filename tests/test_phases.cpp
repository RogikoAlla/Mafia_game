#include "game/Host.hpp"
#include "game/Player.hpp"

#include "smart_ptr/SharedPtr.hpp"

#include <iostream>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

}  // namespace

int main() {
#ifdef _WIN32
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

    expect(state->phase() == mafia::Phase::Finished, "после двух раундов фаза — конец");
    expect(state->round() == kRounds, "ведущий дошёл до второго раунда");
    expect(state.get() != nullptr, "партия жива после join");

    if (failures != 0) {
        std::cerr << failures << " проверок не прошли\n";
        return 1;
    }
    std::cout << "фазы: все проверки прошли\n";
    return 0;
}
