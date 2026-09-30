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
            if (phase == Phase::DayVote) {
                const int eliminated = state_->resolveDayVote();
                if (eliminated == 0) {
                    std::cout << "ведущий: ничья, никто не выбыл\n";
                } else {
                    std::cout << "ведущий: исключён игрок " << eliminated << '\n';
                }
            }
            if (phase == Phase::Night) {
                const NightResult night = state_->resolveNight();
                if (state_->fullLog()) {
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
                        std::cout << "ведущий: ночью убит игрок " << id << '\n';
                        any = true;
                    }
                }
                if (!any) {
                    std::cout << "ведущий: ночь без убийств\n";
                }
            }
        }
    }

    std::cout << "ведущий: партия закончена\n";
    state_->beginPhase(Phase::Finished, rounds);
}

}  // namespace mafia
