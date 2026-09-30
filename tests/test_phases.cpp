#include "game/Host.hpp"
#include "game/Player.hpp"

#include "smart_ptr/SharedPtr.hpp"

#include <iostream>
#include <memory>
#include <random>
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

bool isRoleClass(mafia::Player* player, mafia::Role role) {
    switch (role) {
    case mafia::Role::Mafia:
        return dynamic_cast<mafia::Mafia*>(player) != nullptr;
    case mafia::Role::Doctor:
        return dynamic_cast<mafia::Doctor*>(player) != nullptr;
    case mafia::Role::Commissioner:
        return dynamic_cast<mafia::Commissioner*>(player) != nullptr;
    case mafia::Role::Maniac:
        return dynamic_cast<mafia::Maniac*>(player) != nullptr;
    case mafia::Role::Civilian:
        return dynamic_cast<mafia::Civilian*>(player) != nullptr;
    }
    return false;
}

void expectRoleCounts(const mafia::GameState& state, int mafiaCount, const char* label) {
    int mafia = 0;
    int doctor = 0;
    int commissioner = 0;
    int maniac = 0;
    int civilian = 0;
    for (int id = 1; id <= state.playerCount(); ++id) {
        switch (state.role(id)) {
        case mafia::Role::Mafia:
            ++mafia;
            break;
        case mafia::Role::Doctor:
            ++doctor;
            break;
        case mafia::Role::Commissioner:
            ++commissioner;
            break;
        case mafia::Role::Maniac:
            ++maniac;
            break;
        case mafia::Role::Civilian:
            ++civilian;
            break;
        }
    }
    expect(mafia == mafiaCount, label);
    expect(doctor == 1, "в раздаче один доктор");
    expect(commissioner == 1, "в раздаче один комиссар");
    expect(maniac == 1, "в раздаче один маньяк");
    expect(civilian == state.playerCount() - mafiaCount - 3, "остальные — мирные");
}

}  // namespace

void testDayVote() {
    mafia::GameState majority(5);
    majority.submitVote(1, 2);
    majority.submitVote(2, 1);
    majority.submitVote(3, 1);
    majority.submitVote(4, 1);
    majority.submitVote(5, 1);
    expect(majority.resolveDayVote() == 1, "большинство исключает игрока 1");
    expect(!majority.alive(1), "игрок 1 мёртв после голосования");
    expect(majority.alive(2), "игрок 2 остаётся жив");

    mafia::GameState tie(4);
    tie.submitVote(1, 2);
    tie.submitVote(2, 1);
    tie.submitVote(3, 1);
    tie.submitVote(4, 2);
    expect(tie.resolveDayVote() == 0, "ничья, никто не выбывает");
    expect(tie.alive(1) && tie.alive(2), "при ничьей оба кандидата живы");

    mafia::GameState selfVote(3);
    selfVote.submitVote(1, 1);
    selfVote.submitVote(2, 1);
    selfVote.submitVote(3, 2);
    expect(selfVote.acted() == 3, "голос в себя отмечается");
    expect(selfVote.resolveDayVote() == 0, "голос в себя не идёт в подсчёт");
    expect(selfVote.alive(1), "игрок 1 жив, своего голоса нет в большинстве");

    mafia::GameState deadVote(3);
    deadVote.submitVote(2, 1);
    deadVote.submitVote(3, 1);
    deadVote.submitVote(1, 2);
    expect(deadVote.resolveDayVote() == 1, "перед ходом мёртвого игрок 1 исключён");
    deadVote.beginPhase(mafia::Phase::DayVote, 1);
    deadVote.submitVote(1, 2);
    deadVote.submitVote(2, 3);
    deadVote.submitVote(3, 2);
    expect(deadVote.acted() == 3, "ход мёртвого отмечается");
    expect(deadVote.resolveDayVote() == 0, "голос мёртвого не учитывается");
    expect(deadVote.alive(2) && deadVote.alive(3), "живые при ничьей остаются");
}

void testNight() {
    mafia::GameState saved(5);
    saved.assignRole(1, mafia::Role::Mafia);
    saved.assignRole(2, mafia::Role::Mafia);
    saved.assignRole(3, mafia::Role::Doctor);
    saved.assignRole(4, mafia::Role::Commissioner);
    saved.assignRole(5, mafia::Role::Maniac);
    saved.submitMafiaKill(2, 3);
    saved.submitMafiaKill(1, 3);
    saved.submitHeal(3, 3);
    saved.submitShot(4, 3);
    saved.submitManiacKill(5, 3);
    const mafia::NightResult savedNight = saved.resolveNight();
    expect(savedNight.mafiaKilled == 0, "доктор спасает от мафии");
    expect(savedNight.maniacKilled == 0, "доктор спасает от маньяка");
    expect(savedNight.commissionerKilled == 0, "доктор спасает от комиссара");
    expect(saved.alive(3), "вылеченный доктор жив");

    mafia::GameState boss(4);
    boss.assignRole(1, mafia::Role::Mafia);
    boss.assignRole(2, mafia::Role::Mafia);
    boss.assignRole(3, mafia::Role::Civilian);
    boss.assignRole(4, mafia::Role::Civilian);
    boss.submitMafiaKill(2, 3);
    expect(boss.resolveNight().mafiaKilled == 0, "не босс не назначает убийство");
    expect(boss.alive(3), "цель не босса жива");
    boss.beginPhase(mafia::Phase::Night, 2);
    boss.submitMafiaKill(1, 2);
    expect(boss.resolveNight().mafiaKilled == 0, "мафия не убивает свою");
    expect(boss.alive(2), "второй мафиози жив");

    mafia::GameState check(5);
    check.assignRole(1, mafia::Role::Mafia);
    check.assignRole(2, mafia::Role::Doctor);
    check.assignRole(3, mafia::Role::Commissioner);
    check.assignRole(4, mafia::Role::Maniac);
    check.assignRole(5, mafia::Role::Civilian);
    check.submitCheck(3, 4);
    check.resolveNight();
    expect(check.inspected(4), "комиссар проверил маньяка");
    expect(check.inspectedAs(4) == mafia::Role::Civilian, "маньяк выглядит мирным");
    check.beginPhase(mafia::Phase::Night, 2);
    check.submitCheck(3, 1);
    check.resolveNight();
    expect(check.inspectedAs(1) == mafia::Role::Mafia, "комиссар узнаёт мафию");
    check.beginPhase(mafia::Phase::Night, 3);
    check.submitShot(3, 1);
    check.submitHeal(2, 5);
    check.submitManiacKill(4, 5);
    const mafia::NightResult shot = check.resolveNight();
    expect(shot.commissionerKilled == 1, "комиссар стреляет в найденную мафию");
    expect(!check.alive(1), "мафия мертва");
    expect(shot.maniacKilled == 0, "лечение спасает мирного от маньяка");
    expect(check.alive(5), "вылеченный мирный жив");

    mafia::GameState repeat(3);
    repeat.assignRole(1, mafia::Role::Mafia);
    repeat.assignRole(2, mafia::Role::Doctor);
    repeat.assignRole(3, mafia::Role::Civilian);
    repeat.submitHeal(2, 3);
    repeat.submitMafiaKill(1, 3);
    expect(repeat.resolveNight().mafiaKilled == 0, "первое лечение спасает");
    repeat.beginPhase(mafia::Phase::Night, 2);
    repeat.submitHeal(2, 3);
    repeat.submitMafiaKill(1, 3);
    expect(repeat.resolveNight().mafiaKilled == 3, "повторное лечение не спасает");
    expect(!repeat.alive(3), "игрок 3 убит на вторую ночь");

    mafia::GameState selfKill(2);
    selfKill.assignRole(1, mafia::Role::Maniac);
    selfKill.assignRole(2, mafia::Role::Civilian);
    selfKill.submitManiacKill(1, 1);
    expect(selfKill.resolveNight().maniacKilled == 0, "маньяк не убивает себя");
    expect(selfKill.alive(1), "маньяк жив");
    expect(selfKill.acted() == 1, "неверный ночной ход отмечается");
}

std::vector<mafia::Role> dealWithSeed(unsigned seed) {
    mafia::SharedPtr<mafia::GameState> state(new mafia::GameState(10));
    mafia::Host host(state);
    std::mt19937 generator(seed);
    host.dealRoles(generator);
    std::vector<mafia::Role> roles;
    roles.reserve(10);
    for (int id = 1; id <= 10; ++id) {
        roles.push_back(state->role(id));
    }
    return roles;
}

void testSameSeedDealsTheSameRoles() {
    expect(dealWithSeed(1) == dealWithSeed(1), "одно зерно даёт одну раздачу");
    expect(dealWithSeed(1) != dealWithSeed(2), "разные зёрна перемешивают id по-разному");
}

void testMafiaNightSkipsMafia() {
    mafia::SharedPtr<mafia::GameState> state(new mafia::GameState(4));
    state->assignRole(1, mafia::Role::Mafia);
    state->assignRole(2, mafia::Role::Mafia);
    state->assignRole(3, mafia::Role::Civilian);
    state->assignRole(4, mafia::Role::Civilian);
    state->seedChoices(3);
    mafia::Mafia boss(state, 1);
    int shots = 0;
    for (int round = 1; round <= 4; ++round) {
        state->beginPhase(mafia::Phase::Night, round);
        boss.actNight();
        const mafia::NightResult night = state->resolveNight();
        if (night.mafiaTarget == 0) {
            continue;
        }
        ++shots;
        expect(state->role(night.mafiaTarget) != mafia::Role::Mafia, "мафия не стреляет в мафию");
    }
    expect(shots > 0, "босс мафии сделал выстрел");
}

void testMafiaDoesNotVoteForMafia() {
    mafia::SharedPtr<mafia::GameState> state(new mafia::GameState(4));
    state->assignRole(1, mafia::Role::Mafia);
    state->assignRole(2, mafia::Role::Mafia);
    state->assignRole(3, mafia::Role::Civilian);
    state->assignRole(4, mafia::Role::Civilian);
    state->seedChoices(7);
    mafia::Mafia voter(state, 1);
    for (int round = 1; round <= 12; ++round) {
        state->beginPhase(mafia::Phase::DayVote, round);
        voter.vote();
        const int target = state->voteOf(1);
        expect(target == 3 || target == 4, "мафия голосует не за мафию");
    }
}

void testDealForTenPlayers() {
    mafia::SharedPtr<mafia::GameState> state(new mafia::GameState(10));
    mafia::Host host(state);
    std::mt19937 generator(1);
    host.dealRoles(generator);
    expectRoleCounts(*state, 3, "при N=10 мафий трое");
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    testDayVote();
    testMafiaDoesNotVoteForMafia();
    testMafiaNightSkipsMafia();
    testNight();
    testSameSeedDealsTheSameRoles();
    testDealForTenPlayers();

    constexpr int kPlayers = 5;
    constexpr int kRounds = 2;

    mafia::SharedPtr<mafia::GameState> state(new mafia::GameState(kPlayers));
    mafia::Host host(state);
    std::mt19937 generator(1);
    host.dealRoles(generator);
    expectRoleCounts(*state, 1, "при N=5 мафия одна");

    mafia::Role dealt[kPlayers + 1];
    for (int playerId = 1; playerId <= kPlayers; ++playerId) {
        dealt[playerId] = state->role(playerId);
    }

    std::vector<std::unique_ptr<mafia::Player>> roster;
    roster.reserve(kPlayers);
    for (int playerId = 1; playerId <= kPlayers; ++playerId) {
        roster.push_back(mafia::makePlayer(state, playerId));
    }
    for (int playerId = 1; playerId <= kPlayers; ++playerId) {
        expect(isRoleClass(roster[playerId - 1].get(), dealt[playerId]),
               "класс игрока совпадает с разданной ролью");
    }

    auto interactive = mafia::makePlayer(state, 1, true);
    expect(isRoleClass(interactive.get(), dealt[1]),
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

    for (int playerId = 1; playerId <= kPlayers; ++playerId) {
        expect(state->role(playerId) == dealt[playerId], "роли не меняются за партию");
    }

    if (failures != 0) {
        std::cerr << failures << " проверок не прошли\n";
        return 1;
    }
    std::cout << "фазы: все проверки прошли\n";
    return 0;
}
