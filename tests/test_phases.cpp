#include "game/Host.hpp"
#include "game/Player.hpp"

#include "smart_ptr/SharedPtr.hpp"

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
    mafia::SharedPtr<mafia::GameState> state(new mafia::GameState(10));
    mafia::Host host(state);
    host.dealRoles();
    expect(state->role(1) == mafia::Role::Mafia, "при N=10 мафия — игроки 1–3");
    expect(state->role(2) == mafia::Role::Mafia, "при N=10 игрок 2 — мафия");
    expect(state->role(3) == mafia::Role::Mafia, "при N=10 игрок 3 — мафия");
    expect(state->role(4) == mafia::Role::Doctor, "при N=10 игрок 4 — доктор");
    expect(state->role(5) == mafia::Role::Commissioner, "при N=10 игрок 5 — комиссар");
    expect(state->role(6) == mafia::Role::Maniac, "при N=10 игрок 6 — маньяк");
    expect(state->role(7) == mafia::Role::Civilian, "при N=10 хвост — мирные");
    expect(state->role(10) == mafia::Role::Civilian, "при N=10 игрок 10 — мирный");
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
    mafia::Host host(state);
    host.dealRoles();

    std::vector<std::unique_ptr<mafia::Player>> roster;
    roster.reserve(kPlayers);
    for (int playerId = 1; playerId <= kPlayers; ++playerId) {
        roster.push_back(mafia::makePlayer(state, playerId));
    }
    expect(dynamic_cast<mafia::Mafia*>(roster[0].get()) != nullptr, "класс игрока 1 — Mafia");
    expect(dynamic_cast<mafia::Doctor*>(roster[1].get()) != nullptr, "класс игрока 2 — Doctor");
    expect(dynamic_cast<mafia::Commissioner*>(roster[2].get()) != nullptr,
           "класс игрока 3 — Commissioner");
    expect(dynamic_cast<mafia::Maniac*>(roster[3].get()) != nullptr, "класс игрока 4 — Maniac");
    expect(dynamic_cast<mafia::Civilian*>(roster[4].get()) != nullptr, "класс игрока 5 — Civilian");

    auto interactive = mafia::makePlayer(state, 1, true);
    expect(dynamic_cast<mafia::Mafia*>(interactive.get()) != nullptr,
           "интерактивный игрок остаётся классом своей роли");
    expect(interactive->interactive(), "флаг --interactive сохраняется на игроке");

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
