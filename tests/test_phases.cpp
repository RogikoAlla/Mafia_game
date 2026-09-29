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

void testDealForTenPlayers() {
    mafia::GameState state(10);
    expect(state.role(1) == mafia::Role::Mafia, "при N=10 мафия — игроки 1–3");
    expect(state.role(2) == mafia::Role::Mafia, "при N=10 игрок 2 — мафия");
    expect(state.role(3) == mafia::Role::Mafia, "при N=10 игрок 3 — мафия");
    expect(state.role(4) == mafia::Role::Doctor, "при N=10 игрок 4 — доктор");
    expect(state.role(5) == mafia::Role::Commissioner, "при N=10 игрок 5 — комиссар");
    expect(state.role(6) == mafia::Role::Maniac, "при N=10 игрок 6 — маньяк");
    expect(state.role(7) == mafia::Role::Civilian, "при N=10 хвост — мирные");
    expect(state.role(10) == mafia::Role::Civilian, "при N=10 игрок 10 — мирный");
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    testDealForTenPlayers();

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

    expect(state->role(1) == mafia::Role::Mafia, "на 5 игроках один мафия — игрок 1");
    expect(state->role(2) == mafia::Role::Doctor, "игрок 2 — доктор");
    expect(state->role(3) == mafia::Role::Commissioner, "игрок 3 — комиссар");
    expect(state->role(4) == mafia::Role::Maniac, "игрок 4 — маньяк");
    expect(state->role(5) == mafia::Role::Civilian, "игрок 5 — мирный");

    if (failures != 0) {
        std::cerr << failures << " проверок не прошли\n";
        return 1;
    }
    std::cout << "фазы: все проверки прошли\n";
    return 0;
}
