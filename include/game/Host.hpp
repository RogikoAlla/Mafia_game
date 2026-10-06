#pragma once

#include "smart_ptr/SharedPtr.hpp"
#include "game/GameState.hpp"

#include <random>
#include <string>
#include <vector>

namespace mafia {

// Маньяк, доктор, комиссар и мирный — лагерь мирных. Мафия — свой лагерь.
inline const char* campName(Role role) {
    return role == Role::Mafia ? "мафия" : "мирные";
}

inline std::string eliminatedLine(int playerId, Role role, bool open) {
    const std::string who = "ведущий: исключён игрок " + std::to_string(playerId) + ", ";
    if (open) {
        return who + roleName(role);
    }
    return who + "лагерь " + campName(role);
}

inline std::string alliesLine(const std::vector<int>& allies) {
    if (allies.empty()) {
        return "соратников нет";
    }
    std::string line = "соратники";
    for (int id : allies) {
        line += ' ';
        line += std::to_string(id);
    }
    return line;
}

inline std::string nightVictimLine(int playerId, Role role, bool open) {
    if (open) {
        return "ведущий: ночью убит игрок " + std::to_string(playerId) + ", " + roleName(role);
    }
    return "ведущий: ночью выбыл игрок " + std::to_string(playerId) + ", лагерь " + campName(role);
}

// Ведущий не наследник игрока. Раздаёт роли и открывает фазы.
// Запускать run через std::thread. Пока игроки не сходят, ведущий ждёт на каждой фазе.
class Host {
public:
    explicit Host(SharedPtr<GameState> state) : state_(std::move(state)) {}

    // k = 3: мафии max(1, N/k). Дальше по одному: доктор, комиссар, маньяк. Хвост — мирные.
    // Id перемешиваются генератором, состав ролей от этого не меняется.
    void dealRoles(std::mt19937& generator);

    // Крутит день и ночь, пока checkWinner не назовёт сторону.
    void run();

private:
    void promptHuman(Phase phase) const;

    SharedPtr<GameState> state_;
};

}  // namespace mafia
