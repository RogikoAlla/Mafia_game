#pragma once

#include "smart_ptr/SharedPtr.hpp"
#include "game/GameState.hpp"

namespace mafia {

// Ведущий не наследник игрока. Раздаёт роли и открывает фазы.
// Запускать run через std::thread. Пока игроки не сходят, ведущий ждёт на каждой фазе.
class Host {
public:
    explicit Host(SharedPtr<GameState> state) : state_(std::move(state)) {}

    // k = 3: мафии max(1, N/k). Дальше по одному: доктор, комиссар, маньяк. Хвост — мирные.
    void dealRoles();

    void run(int rounds = 2);

private:
    SharedPtr<GameState> state_;
};

}  // namespace mafia
