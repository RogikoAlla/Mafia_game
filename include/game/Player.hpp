#pragma once

#include "smart_ptr/SharedPtr.hpp"
#include "game/GameState.hpp"

namespace mafia {

// Тело потока игрока. На каждой фазе только отмечает, что ход сделан.
// Если ведущий открыл Finished, поток выходит.
void runPlayer(SharedPtr<GameState> state, int playerId, int rounds = 2);

}  // namespace mafia
