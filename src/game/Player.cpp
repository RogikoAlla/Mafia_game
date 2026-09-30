#include "game/Player.hpp"

#include <iostream>
#include <string>

namespace mafia {
namespace {

const Phase kPhases[] = {Phase::DayTalk, Phase::DayVote, Phase::Night};

}  // namespace

Player::Player(SharedPtr<GameState> state, int playerId, bool interactive)
    : state_(std::move(state)), playerId_(playerId), interactive_(interactive) {}

void Player::talk() {
    if (!state_->alive(playerId_)) {
        state_->submitTalk(playerId_, "");
        return;
    }
    const std::string text = "я не мафия";
    if (state_->fullLog()) {
        std::cout << "игрок " << playerId_ << ": " << text << '\n';
    }
    state_->submitTalk(playerId_, text);
}

int Player::chooseVoteTarget() const {
    for (int id = 1; id <= state_->playerCount(); ++id) {
        if (id != playerId_ && state_->alive(id)) {
            return id;
        }
    }
    return 0;
}

void Player::vote() {
    if (!state_->alive(playerId_)) {
        state_->submitVote(playerId_, 0);
        return;
    }
    const int target = chooseVoteTarget();
    if (state_->fullLog()) {
        if (target == 0) {
            std::cout << "игрок " << playerId_ << ": голос не засчитан\n";
        } else {
            std::cout << "игрок " << playerId_ << " голосует за " << target << '\n';
        }
    }
    state_->submitVote(playerId_, target);
}

void Player::actNight() { state_->markActed(); }

void Player::run(int rounds) {
    for (int round = 1; round <= rounds; ++round) {
        for (Phase phase : kPhases) {
            state_->waitForPhase(phase, round);
            if (state_->phase() == Phase::Finished) {
                return;
            }
            std::cout << "игрок " << playerId_ << ": раунд " << round << ", " << phaseName(phase)
                      << '\n';
            switch (phase) {
            case Phase::DayTalk:
                talk();
                break;
            case Phase::DayVote:
                vote();
                break;
            case Phase::Night:
                actNight();
                break;
            case Phase::Finished:
                return;
            }
        }
    }
}

std::unique_ptr<Player> makePlayer(SharedPtr<GameState> state, int playerId, bool interactive) {
    switch (state->role(playerId)) {
    case Role::Mafia:
        return std::make_unique<Mafia>(state, playerId, interactive);
    case Role::Doctor:
        return std::make_unique<Doctor>(state, playerId, interactive);
    case Role::Commissioner:
        return std::make_unique<Commissioner>(state, playerId, interactive);
    case Role::Maniac:
        return std::make_unique<Maniac>(state, playerId, interactive);
    case Role::Civilian:
        return std::make_unique<Civilian>(state, playerId, interactive);
    }
    return std::make_unique<Civilian>(state, playerId, interactive);
}

}  // namespace mafia
