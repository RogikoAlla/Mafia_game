#include "game/Host.hpp"

#include <iostream>

namespace mafia {

void runHost(SharedPtr<GameState> state, int rounds) {
    const Phase phases[] = {Phase::DayTalk, Phase::DayVote, Phase::Night};

    for (int round = 1; round <= rounds; ++round) {
        for (Phase phase : phases) {
            std::cout << "ведущий: раунд " << round << ", фаза " << phaseName(phase) << '\n';
            state->beginPhase(phase, round);
            state->waitUntilAllActed();
        }
    }

    std::cout << "ведущий: партия закончена\n";
    state->beginPhase(Phase::Finished, rounds);
}

}  // namespace mafia
