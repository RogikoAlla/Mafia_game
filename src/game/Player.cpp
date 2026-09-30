#include "game/Player.hpp"

#include <iostream>
#include <string>
#include <vector>

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
    std::vector<int> candidates;
    for (int id = 1; id <= state_->playerCount(); ++id) {
        if (id != playerId_ && state_->alive(id)) {
            candidates.push_back(id);
        }
    }
    return state_->pickCandidate(candidates);
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

namespace {

void logNight(const SharedPtr<GameState>& state, int playerId, const char* action, int target) {
    if (!state->fullLog()) {
        return;
    }
    std::cout << "игрок " << playerId << " " << action << " " << target << '\n';
}

}  // namespace

int Mafia::chooseVoteTarget() const {
    std::vector<int> candidates;
    for (int id = 1; id <= state_->playerCount(); ++id) {
        if (id != playerId_ && state_->alive(id) && state_->role(id) != Role::Mafia) {
            candidates.push_back(id);
        }
    }
    return state_->pickCandidate(candidates);
}

void Mafia::actNight() {
    if (!state_->alive(playerId_) || state_->mafiaBoss() != playerId_) {
        state_->markActed();
        return;
    }
    std::vector<int> candidates;
    for (int id = 1; id <= state_->playerCount(); ++id) {
        if (state_->alive(id) && state_->role(id) != Role::Mafia) {
            candidates.push_back(id);
        }
    }
    const int target = state_->pickCandidate(candidates);
    logNight(state_, playerId_, "убивает", target);
    state_->submitMafiaKill(playerId_, target);
}

void Doctor::actNight() {
    if (!state_->alive(playerId_)) {
        state_->markActed();
        return;
    }
    const int previous = state_->lastHeal();
    std::vector<int> candidates;
    for (int id = 1; id <= state_->playerCount(); ++id) {
        if (state_->alive(id) && id != previous) {
            candidates.push_back(id);
        }
    }
    const int target = state_->pickCandidate(candidates);
    logNight(state_, playerId_, "лечит", target);
    state_->submitHeal(playerId_, target);
}

void Commissioner::actNight() {
    if (!state_->alive(playerId_)) {
        state_->markActed();
        return;
    }

    std::vector<int> knownMafia;
    std::vector<int> unchecked;
    for (int id = 1; id <= state_->playerCount(); ++id) {
        if (!state_->alive(id) || id == playerId_) {
            continue;
        }
        if (state_->inspected(id) && state_->inspectedAs(id) == Role::Mafia) {
            knownMafia.push_back(id);
        } else if (!state_->inspected(id)) {
            unchecked.push_back(id);
        }
    }
    if (!knownMafia.empty()) {
        const int target = state_->pickCandidate(knownMafia);
        logNight(state_, playerId_, "стреляет в", target);
        state_->submitShot(playerId_, target);
        return;
    }
    if (!unchecked.empty()) {
        const int target = state_->pickCandidate(unchecked);
        logNight(state_, playerId_, "проверяет", target);
        state_->submitCheck(playerId_, target);
        return;
    }

    std::vector<int> others;
    for (int id = 1; id <= state_->playerCount(); ++id) {
        if (id != playerId_ && state_->alive(id)) {
            others.push_back(id);
        }
    }
    const int target = state_->pickCandidate(others);
    logNight(state_, playerId_, "стреляет в", target);
    state_->submitShot(playerId_, target);
}

void Maniac::actNight() {
    if (!state_->alive(playerId_)) {
        state_->markActed();
        return;
    }
    std::vector<int> candidates;
    for (int id = 1; id <= state_->playerCount(); ++id) {
        if (id != playerId_ && state_->alive(id)) {
            candidates.push_back(id);
        }
    }
    const int target = state_->pickCandidate(candidates);
    logNight(state_, playerId_, "убивает", target);
    state_->submitManiacKill(playerId_, target);
}

void Player::run() {
    if (state_->fullLog()) {
        std::cout << "игрок " << playerId_ << ": роль — " << roleName(state_->role(playerId_))
                  << '\n';
    }
    int round = 1;
    while (true) {
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
        ++round;
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
