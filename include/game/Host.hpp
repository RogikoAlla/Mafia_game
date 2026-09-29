#pragma once

#include "smart_ptr/SharedPtr.hpp"
#include "game/GameState.hpp"

namespace mafia {

// Тело потока ведущего. Два раунда: обсуждение, голосование, ночь.
// После последнего раунда открывает Finished.
// Запускать через std::thread. Пока игроки не вызывают markActed, ведущий ждёт на каждой фазе.
void runHost(SharedPtr<GameState> state, int rounds = 2);

}  // namespace mafia
