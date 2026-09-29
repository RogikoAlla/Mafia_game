#include "game/Host.hpp"

#include <algorithm>
#include <iostream>

namespace mafia {

void Host::dealRoles() {
    constexpr int kMafiaDivisor = 3;
    const int playerCount = state_->playerCount();
    const int mafiaCount = std::max(1, playerCount / kMafiaDivisor);
    int nextId = 1;
    for (int i = 0; i < mafiaCount; ++i, ++nextId) {
        state_->assignRole(nextId, Role::Mafia);
    }
    state_->assignRole(nextId++, Role::Doctor);
    state_->assignRole(nextId++, Role::Commissioner);
    state_->assignRole(nextId++, Role::Maniac);
}

void Host::run(int rounds) {
    const Phase phases[] = {Phase::DayTalk, Phase::DayVote, Phase::Night};

    for (int round = 1; round <= rounds; ++round) {
        for (Phase phase : phases) {
            std::cout << "ведущий: раунд " << round << ", фаза " << phaseName(phase) << '\n';
            state_->beginPhase(phase, round);
            state_->waitUntilAllActed();
        }
    }

    std::cout << "ведущий: партия закончена\n";
    state_->beginPhase(Phase::Finished, rounds);
}

}  // namespace mafia
