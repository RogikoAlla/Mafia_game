#include "game/Host.hpp"

#include <algorithm>
#include <iostream>
#include <numeric>
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
        std::cout << "ведущий: игрок " << human << ", введите реплику\n";
        return;
    }
    if (phase == Phase::DayVote ||
        (phase == Phase::Night && state_->nightChoiceRequired(human))) {
        std::cout << "ведущий: игрок " << human << ", введите номер живого игрока\n";
    }
}

void Host::run() {
    const Phase phases[] = {Phase::DayTalk, Phase::DayVote, Phase::Night};
    int round = 1;
    while (true) {
        for (Phase phase : phases) {
            std::cout << "ведущий: раунд " << round << ", фаза " << phaseName(phase) << '\n';
            promptHuman(phase);
            state_->beginPhase(phase, round);
            state_->waitUntilAllActed();
            if (phase == Phase::DayVote) {
                const int eliminated = state_->resolveDayVote();
                if (eliminated == 0) {
                    std::cout << "ведущий: ничья, никто не выбыл\n";
                } else {
                    std::cout << eliminatedLine(eliminated, state_->role(eliminated),
                                                state_->openAnnouncements())
                              << '\n';
                }
            }
            if (phase == Phase::Night) {
                const NightResult night = state_->resolveNight();
                const bool tellNight = state_->openAnnouncements() || state_->fullLog();
                if (tellNight) {
                    if (night.mafiaTarget != 0) {
                        std::cout << "ведущий: мафия выбирает игрока " << night.mafiaTarget << '\n';
                    }
                    if (night.maniacTarget != 0) {
                        std::cout << "ведущий: маньяк выбирает игрока " << night.maniacTarget << '\n';
                    }
                    if (night.doctorTarget != 0) {
                        std::cout << "ведущий: доктор лечит игрока " << night.doctorTarget << '\n';
                    }
                    if (night.commissionerTarget != 0) {
                        std::cout << "ведущий: комиссар "
                                  << (night.commissionerShot ? "стреляет в игрока "
                                                             : "проверяет игрока ")
                                  << night.commissionerTarget << '\n';
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
                        std::cout << nightVictimLine(id, state_->role(id), open) << '\n';
                        any = true;
                    }
                }
                if (!any) {
                    std::cout << "ведущий: ночь без убийств\n";
                }
            }
            const Winner winner = state_->checkWinner();
            if (winner != Winner::None) {
                state_->setWinner(winner);
                std::cout << "ведущий: победа — " << winnerName(winner) << '\n';
                state_->beginPhase(Phase::Finished, round);
                return;
            }
        }
        ++round;
    }
}

}  // namespace mafia
