#include "game/Host.hpp"
#include "game/Log.hpp"

#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

namespace mafia {

void Host::dealRoles(std::mt19937& generator) {
    constexpr int kMafiaDivisor = 3;
    const int playerCount = state_->playerCount();
    const int mafiaCount = std::max(1, playerCount / kMafiaDivisor);

    std::vector<int> ids(playerCount);
    std::iota(ids.begin(), ids.end(), 1);
    std::shuffle(ids.begin(), ids.end(), generator);

    int next = 0;
    for (int i = 0; i < mafiaCount; ++i) {
        state_->assignRole(ids[next++], Role::Mafia);
    }
    state_->assignRole(ids[next++], Role::Doctor);
    state_->assignRole(ids[next++], Role::Commissioner);
    state_->assignRole(ids[next++], Role::Maniac);
}

void Host::promptHuman(Phase phase) const {
    const int human = state_->interactivePlayer();
    if (human == 0 || !state_->alive(human)) {
        return;
    }
    if (phase == Phase::DayTalk) {
        logLine("ведущий: игрок " + std::to_string(human) + ", введите реплику");
        return;
    }
    if (phase == Phase::Night && state_->role(human) == Role::Mafia) {
        logLine("ведущий: " + alliesLine(state_->mafiaAllies(human)));
    }
    if (phase == Phase::DayVote ||
        (phase == Phase::Night && state_->nightChoiceRequired(human))) {
        logLine("ведущий: игрок " + std::to_string(human) + ", введите номер живого игрока");
    }
}

void Host::run() {
    const Phase phases[] = {Phase::DayTalk, Phase::DayVote, Phase::Night};
    int round = 1;
    while (true) {
        for (Phase phase : phases) {
            logLine("ведущий: раунд " + std::to_string(round) + ", фаза " + phaseName(phase));
            promptHuman(phase);
            state_->beginPhase(phase, round);
            state_->waitUntilAllActed();
            if (phase == Phase::DayVote) {
                const int eliminated = state_->resolveDayVote();
                if (eliminated == 0) {
                    logLine("ведущий: ничья, никто не выбыл");
                } else {
                    logLine(eliminatedLine(eliminated, state_->role(eliminated),
                                           state_->openAnnouncements()));
                }
            }
            if (phase == Phase::Night) {
                const NightResult night = state_->resolveNight();
                const bool tellNight = state_->openAnnouncements() || state_->fullLog();
                if (tellNight) {
                    if (night.mafiaTarget != 0) {
                        logLine("ведущий: мафия выбирает игрока " + std::to_string(night.mafiaTarget));
                    }
                    if (night.maniacTarget != 0) {
                        logLine("ведущий: маньяк выбирает игрока " + std::to_string(night.maniacTarget));
                    }
                    if (night.doctorTarget != 0) {
                        logLine("ведущий: доктор лечит игрока " + std::to_string(night.doctorTarget));
                    }
                    if (night.commissionerTarget != 0) {
                        logLine(std::string("ведущий: комиссар ") +
                                (night.commissionerShot ? "стреляет в игрока " : "проверяет игрока ") +
                                std::to_string(night.commissionerTarget));
                    }
                }
                const int killed[] = {night.mafiaKilled, night.maniacKilled, night.commissionerKilled};
                const bool open = state_->openAnnouncements();
                bool any = false;
                for (int index = 0; index < 3; ++index) {
                    const int id = killed[index];
                    if (id == 0) {
                        continue;
                    }
                    bool already = false;
                    for (int earlier = 0; earlier < index; ++earlier) {
                        if (killed[earlier] == id) {
                            already = true;
                        }
                    }
                    if (!already) {
                        logLine(nightVictimLine(id, state_->role(id), open));
                        any = true;
                    }
                }
                if (!any) {
                    logLine("ведущий: ночь без убийств");
                }
            }
            const Winner winner = state_->checkWinner();
            if (winner != Winner::None) {
                state_->setWinner(winner);
                logLine(std::string("ведущий: победа — ") + winnerName(winner));
                state_->beginPhase(Phase::Finished, round);
                return;
            }
        }
        ++round;
    }
}

}  // namespace mafia
