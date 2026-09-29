#pragma once

#include "game/Types.hpp"

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace mafia {

// Одна партия на всех. Ведущий и игроки держат её через SharedPtr.
// Поля меняются только под mutex_, сон идёт через condition_.
class GameState {
public:
    // k = 3: мафии max(1, N/k). Дальше по одному: доктор, комиссар, маньяк. Хвост — мирные.
    // Индекс 0 не используется: роль игрока i лежит в roles_[i].
    explicit GameState(int playerCount) : playerCount_(playerCount), roles_(playerCount + 1, Role::Civilian) {
        constexpr int kMafiaDivisor = 3;
        const int mafiaCount = std::max(1, playerCount_ / kMafiaDivisor);
        int nextId = 1;
        for (int i = 0; i < mafiaCount; ++i, ++nextId) {
            roles_[nextId] = Role::Mafia;
        }
        roles_[nextId++] = Role::Doctor;
        roles_[nextId++] = Role::Commissioner;
        roles_[nextId++] = Role::Maniac;
    }

    GameState(const GameState&) = delete;  // не копируем
    GameState& operator=(const GameState&) = delete; // не присваиваем другому объекту GameState                                    

    // Ведущий открывает фазу, обнуляет счётчик ходов и будит игроков.
    void beginPhase(Phase phase, int round) {
        std::lock_guard<std::mutex> lock(mutex_);
        phase_ = phase;
        round_ = round;
        acted_ = 0;
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

    // Роль не меняется после раздачи, поэтому замок не нужен — как у playerCount().
    Role role(int playerId) const { return roles_[playerId]; }

private:
    Phase phase_ = Phase::Finished;
    int round_ = 0;
    int acted_ = 0;
    int playerCount_ = 0;
    bool finished_ = false;
    std::vector<Role> roles_;

    mutable std::mutex mutex_;
    std::condition_variable condition_;
};

}  // namespace mafia
