#include "game/Host.hpp"
#include "game/Log.hpp"
#include "game/Logger.hpp"

#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

namespace mafia {
namespace {

std::string talksText(const GameState& state) {
    std::string text = "обсуждение\n";
    for (int id = 1; id <= state.playerCount(); ++id) {
        const std::string talk = state.talkOf(id);
        if (!talk.empty()) {
            text += "игрок " + std::to_string(id) + ": " + talk + '\n';
        }
    }
    return text;
}

std::string votesText(const GameState& state, int eliminated) {
    std::string text = "голосование\n";
    for (int id = 1; id <= state.playerCount(); ++id) {
        const int target = state.voteOf(id);
        if (target != 0) {
            text += "игрок " + std::to_string(id) + " голосует за " + std::to_string(target) + '\n';
        }
    }
    if (eliminated == 0) {
        text += "ничья, никто не выбыл\n";
    } else {
        text += "исключён игрок " + std::to_string(eliminated) + ", " +
                roleName(state.role(eliminated)) + '\n';
    }
    return text;
}

std::string nightText(const GameState& state, const NightResult& night) {
    std::string text = "ночь\n";
    if (night.mafiaTarget != 0) {
        text += "мафия выбирает игрока " + std::to_string(night.mafiaTarget) + '\n';
    }
    if (night.maniacTarget != 0) {
        text += "маньяк выбирает игрока " + std::to_string(night.maniacTarget) + '\n';
    }
    if (night.doctorTarget != 0) {
        text += "доктор лечит игрока " + std::to_string(night.doctorTarget) + '\n';
    }
    if (night.commissionerTarget != 0) {
        text += std::string("комиссар ") +
                (night.commissionerShot ? "стреляет в игрока " : "проверяет игрока ") +
                std::to_string(night.commissionerTarget) + '\n';
    }
    if (night.journalistFirst != 0) {
        text += "журналист сравнивает " + std::to_string(night.journalistFirst) + " и " +
                std::to_string(night.journalistSecond) + ": " + campVerdict(night.journalistSame) +
                '\n';
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
            text += "убит игрок " + std::to_string(id) + ", " + roleName(state.role(id)) + '\n';
            any = true;
        }
    }
    if (!any) {
        text += "ночь без убийств\n";
    }
    for (int id = 1; id <= state.playerCount(); ++id) {
        text += "игрок " + std::to_string(id) + (state.alive(id) ? " жив\n" : " мёртв\n");
    }
    return text;
}

std::string summaryText(const GameState& state) {
    std::string text = std::string("победитель: ") + winnerName(state.winner()) + '\n';
    for (int id = 1; id <= state.playerCount(); ++id) {
        text += "игрок " + std::to_string(id) + ", " + roleName(state.role(id)) + ", " +
                (state.alive(id) ? "жив" : "мёртв") + ", голосов " +
                std::to_string(state.votesCast(id)) + ", целей ночи " +
                std::to_string(state.nightTargets(id)) + '\n';
    }
    return text;
}

}  // namespace

void Host::dealRoles(std::mt19937& generator, const std::vector<Role>& extras) {
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
    for (Role extra : extras) {
        if (next >= playerCount) {
            break;
        }
        state_->assignRole(ids[next++], extra);
    }
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
    if (phase == Phase::Night && mafiaCamp(state_->role(human))) {
        logLine("ведущий: " + alliesLine(state_->mafiaAllies(human)));
    }
    if (phase == Phase::Night && state_->role(human) == Role::Journalist) {
        logLine("ведущий: игрок " + std::to_string(human) +
                ", введите два номера других живых игроков");
        return;
    }
    if (phase == Phase::DayVote ||
        (phase == Phase::Night && state_->nightChoiceRequired(human))) {
        logLine("ведущий: игрок " + std::to_string(human) + ", введите номер живого игрока");
    }
}

void Host::run() {
    Logger logger("logs");
    const Phase phases[] = {Phase::DayTalk, Phase::DayVote, Phase::Night};
    int round = 1;
    while (true) {
        for (Phase phase : phases) {
            logLine("ведущий: раунд " + std::to_string(round) + ", фаза " + phaseName(phase));
            promptHuman(phase);
            state_->beginPhase(phase, round);
            state_->waitUntilAllActed();
            if (phase == Phase::DayTalk) {
                logger.appendRound(round, talksText(*state_));
            }
            if (phase == Phase::DayVote) {
                const int eliminated = state_->resolveDayVote();
                if (eliminated == 0) {
                    logLine("ведущий: ничья, никто не выбыл");
                } else {
                    logLine(eliminatedLine(eliminated, state_->role(eliminated),
                                           state_->openAnnouncements()));
                }
                logger.appendRound(round, votesText(*state_, eliminated));
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
                    if (night.journalistFirst != 0) {
                        logLine("ведущий: журналист сравнивает " +
                                std::to_string(night.journalistFirst) + " и " +
                                std::to_string(night.journalistSecond) + ": " +
                                campVerdict(night.journalistSame));
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
                logger.appendRound(round, nightText(*state_, night));
            }
            const Winner winner = state_->checkWinner();
            if (winner != Winner::None) {
                state_->setWinner(winner);
                logLine(std::string("ведущий: победа — ") + winnerName(winner));
                logger.writeSummary(summaryText(*state_));
                state_->beginPhase(Phase::Finished, round);
                return;
            }
        }
        ++round;
    }
}

}  // namespace mafia
