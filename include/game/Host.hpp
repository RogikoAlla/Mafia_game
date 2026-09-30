#pragma once

#include "smart_ptr/SharedPtr.hpp"
#include "game/GameState.hpp"

#include <random>

namespace mafia {

// Ведущий не наследник игрока. Раздаёт роли и открывает фазы.
// Запускать run через std::thread. Пока игроки не сходят, ведущий ждёт на каждой фазе.
class Host {
public:
    explicit Host(SharedPtr<GameState> state) : state_(std::move(state)) {}

    // k = 3: мафии max(1, N/k). Дальше по одному: доктор, комиссар, маньяк. Хвост — мирные.
    // Id перемешиваются генератором, состав ролей от этого не меняется.
    void dealRoles(std::mt19937& generator);

    void run(int rounds = 2);

private:
    SharedPtr<GameState> state_;
};

}  // namespace mafia
