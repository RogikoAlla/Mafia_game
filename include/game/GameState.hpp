#pragma once

#include "game/Types.hpp"

#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

namespace mafia {

// Итог ночи. Цели — кого назвали, убитые — кого доктор не спас.
struct NightResult {
    int mafiaTarget = 0;
    int maniacTarget = 0;
    int doctorTarget = 0;
    int commissionerTarget = 0;
    bool commissionerShot = false;
    int mafiaKilled = 0;
    int maniacKilled = 0;
    int commissionerKilled = 0;
};

// Одна партия на всех. Ведущий и игроки держат её через SharedPtr.
// Поля меняются только под mutex_, сон идёт через condition_.
class GameState {
public:
    // Индекс 0 не используется: роль игрока i лежит в roles_[i].
    // До раздачи ведущим все роли — мирные, все игроки живы.
    explicit GameState(int playerCount)
        : playerCount_(playerCount),
          roles_(playerCount + 1, Role::Civilian),
          alive_(playerCount + 1, 1),
          talks_(playerCount + 1),
          votes_(playerCount + 1, 0),
          inspected_(playerCount + 1, 0),
          inspectedAs_(playerCount + 1, Role::Civilian) {}

    GameState(const GameState&) = delete;  // не копируем
    GameState& operator=(const GameState&) = delete; // не присваиваем другому объекту GameState                                    

    // Ведущий открывает фазу, обнуляет счётчик ходов и будит игроков.
    void beginPhase(Phase phase, int round) {
        std::lock_guard<std::mutex> lock(mutex_);
        phase_ = phase;
        round_ = round;
        acted_ = 0;
        for (int id = 1; id <= playerCount_; ++id) {
            talks_[id].clear();
            votes_[id] = 0;
        }
        mafiaTarget_ = 0;
        maniacTarget_ = 0;
        doctorTarget_ = 0;
        commissionerTarget_ = 0;
        commissionerShoots_ = false;
        if (phase == Phase::Finished) {
            finished_ = true;
        }
        condition_.notify_all();
    }

    // Игрок спит, пока ведущий не откроет нужную фазу этого раунда.
    // Конец партии тоже будит: иначе поток останется ждать фазу, которой уже не будет.
    // Начальное Finished само по себе поток не будит: партия ещё не объявлена законченной.
    void waitForPhase(Phase phase, int round) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [&] {
            return (phase_ == phase && round_ == round) || finished_;
        });
    }

    // Игрок отмечает ход и будит ведущего.
    void markActed() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++acted_;
        condition_.notify_all();
    }

    // Ведущий спит, пока не сходят все игроки этой фазы.
    void waitUntilAllActed() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [&] { return acted_ >= playerCount_; });
    }
  
    Phase phase() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return phase_;
    }

    int round() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return round_;
    }

    int acted() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return acted_;
    }

    int playerCount() const { return playerCount_; }

    // Ведущий раздаёт роли до старта потоков. После этого роль не меняется.
    void assignRole(int playerId, Role role) { roles_[playerId] = role; }

    Role role(int playerId) const { return roles_[playerId]; }

    // Режим запуска задаётся до нитей: закрытые объявления и краткий лог.
    void setOpenAnnouncements(bool open) { openAnnouncements_ = open; }
    void setFullLog(bool full) { fullLog_ = full; }

    bool openAnnouncements() const { return openAnnouncements_; }
    bool fullLog() const { return fullLog_; }

    bool alive(int playerId) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return alive_[playerId] != 0;
    }

    // Мёртвый ход не записывает, но отмечает: барьер ведущего считает все места.
    void submitTalk(int playerId, const std::string& text) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (alive_[playerId] != 0) {
            talks_[playerId] = text;
        }
        ++acted_;
        condition_.notify_all();
    }

    // Голос принимается только за другого живого. Неверный голос остаётся 0.
    void submitVote(int playerId, int targetId) {
        std::lock_guard<std::mutex> lock(mutex_);
        const bool valid = alive_[playerId] != 0 && targetId >= 1 && targetId <= playerCount_ &&
                           alive_[targetId] != 0 && targetId != playerId;
        if (valid) {
            votes_[playerId] = targetId;
        }
        ++acted_;
        condition_.notify_all();
    }

    // Строгое большинство исключает цель. Ничья и пустой подсчёт никого не выбывают.
    int resolveDayVote() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<int> tally(playerCount_ + 1, 0);
        for (int voter = 1; voter <= playerCount_; ++voter) {
            const int target = votes_[voter];
            if (target >= 1 && target <= playerCount_) {
                ++tally[target];
            }
        }

        int bestId = 0;
        int bestCount = 0;
        bool tie = false;
        for (int id = 1; id <= playerCount_; ++id) {
            if (tally[id] > bestCount) {
                bestCount = tally[id];
                bestId = id;
                tie = false;
            } else if (tally[id] == bestCount && bestCount > 0) {
                tie = true;
            }
        }
        if (tie || bestId == 0) {
            return 0;
        }
        alive_[bestId] = 0;
        return bestId;
    }

    // Младший живой мафиози. 0, если мафии не осталось.
    int mafiaBoss() const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (int id = 1; id <= playerCount_; ++id) {
            if (alive_[id] != 0 && roles_[id] == Role::Mafia) {
                return id;
            }
        }
        return 0;
    }

    int lastHeal() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return lastDoctorTarget_;
    }

    bool inspected(int playerId) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return inspected_[playerId] != 0;
    }

    // Видимая комиссару роль. Маньяк для него мирный.
    Role inspectedAs(int playerId) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return inspectedAs_[playerId];
    }

    // Младший живой игрок, уже узнанный как мафия. 0, если такого нет.
    int knownMafia() const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (int id = 1; id <= playerCount_; ++id) {
            if (inspected_[id] != 0 && inspectedAs_[id] == Role::Mafia && alive_[id] != 0) {
                return id;
            }
        }
        return 0;
    }

    void submitMafiaKill(int playerId, int targetId) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (playerId == currentBoss() && livingTarget(playerId, targetId, false) &&
            roles_[targetId] != Role::Mafia) {
            mafiaTarget_ = targetId;
        }
        noteActed();
    }

    void submitManiacKill(int playerId, int targetId) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (roles_[playerId] == Role::Maniac && livingTarget(playerId, targetId, false)) {
            maniacTarget_ = targetId;
        }
        noteActed();
    }

    // Себя лечить можно. Ту же цель две ночи подряд — нельзя, ход всё равно отмечается.
    void submitHeal(int playerId, int targetId) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (roles_[playerId] == Role::Doctor && livingTarget(playerId, targetId, true) &&
            targetId != lastDoctorTarget_) {
            doctorTarget_ = targetId;
        }
        noteActed();
    }

    void submitCheck(int playerId, int targetId) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (roles_[playerId] == Role::Commissioner && livingTarget(playerId, targetId, false)) {
            commissionerTarget_ = targetId;
            commissionerShoots_ = false;
        }
        noteActed();
    }

    void submitShot(int playerId, int targetId) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (roles_[playerId] == Role::Commissioner && livingTarget(playerId, targetId, false)) {
            commissionerTarget_ = targetId;
            commissionerShoots_ = true;
        }
        noteActed();
    }

    // Доктор закрывает цель от мафии, маньяка и выстрела комиссара.
    NightResult resolveNight() {
        std::lock_guard<std::mutex> lock(mutex_);
        NightResult result;
        result.mafiaTarget = mafiaTarget_;
        result.maniacTarget = maniacTarget_;
        result.doctorTarget = doctorTarget_;
        result.commissionerTarget = commissionerTarget_;
        result.commissionerShot = commissionerShoots_;

        const int heal = doctorTarget_;
        result.mafiaKilled = killUnlessHealed(mafiaTarget_, heal);
        result.maniacKilled = killUnlessHealed(maniacTarget_, heal);
        if (commissionerShoots_) {
            result.commissionerKilled = killUnlessHealed(commissionerTarget_, heal);
        } else if (commissionerTarget_ != 0) {
            inspected_[commissionerTarget_] = 1;
            inspectedAs_[commissionerTarget_] =
                roles_[commissionerTarget_] == Role::Mafia ? Role::Mafia : Role::Civilian;
        }
        if (doctorTarget_ != 0) {
            lastDoctorTarget_ = doctorTarget_;
        }

        mafiaTarget_ = 0;
        maniacTarget_ = 0;
        doctorTarget_ = 0;
        commissionerTarget_ = 0;
        commissionerShoots_ = false;
        return result;
    }

private:
    void noteActed() {
        ++acted_;
        condition_.notify_all();
    }

    int currentBoss() const {
        for (int id = 1; id <= playerCount_; ++id) {
            if (alive_[id] != 0 && roles_[id] == Role::Mafia) {
                return id;
            }
        }
        return 0;
    }

    bool livingTarget(int actor, int target, bool allowSelf) const {
        if (actor < 1 || actor > playerCount_ || alive_[actor] == 0) {
            return false;
        }
        if (target < 1 || target > playerCount_ || alive_[target] == 0) {
            return false;
        }
        return allowSelf || target != actor;
    }

    int killUnlessHealed(int target, int heal) {
        if (target < 1 || target == heal || alive_[target] == 0) {
            return 0;
        }
        alive_[target] = 0;
        return target;
    }

    Phase phase_ = Phase::Finished;
    int round_ = 0;
    int acted_ = 0;
    int playerCount_ = 0;
    bool finished_ = false;
    bool openAnnouncements_ = false;
    bool fullLog_ = false;
    std::vector<Role> roles_;
    std::vector<char> alive_;
    std::vector<std::string> talks_;
    std::vector<int> votes_;
    int mafiaTarget_ = 0;
    int maniacTarget_ = 0;
    int doctorTarget_ = 0;
    int lastDoctorTarget_ = 0;
    int commissionerTarget_ = 0;
    bool commissionerShoots_ = false;
    std::vector<char> inspected_;
    std::vector<Role> inspectedAs_;

    mutable std::mutex mutex_;
    std::condition_variable condition_;
};

}  // namespace mafia
