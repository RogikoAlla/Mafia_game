#include "game/Player.hpp"

#include <iostream>

namespace mafia {
namespace {

const Phase kPhases[] = {Phase::DayTalk, Phase::DayVote, Phase::Night};

}  // namespace

void runPlayer(SharedPtr<GameState> state, int playerId, int rounds) {
    for (int round = 1; round <= rounds; ++round) {
        for (Phase phase : kPhases) {
            state->waitForPhase(phase, round);
            if (state->phase() == Phase::Finished) {
                return;
            }
            std::cout << "игрок " << playerId << ": раунд " << round << ", " << phaseName(phase)
                      << '\n';
            state->markActed();
        }
    }
}

}  // namespace mafia
