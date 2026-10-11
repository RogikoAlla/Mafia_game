#pragma once

#include "game/Types.hpp"

#include <string>
#include <vector>

namespace mafia {

// Список дополнительных ролей из YAML. Неизвестные имена пропускаются.
std::vector<Role> loadExtraRoles(const std::string& path);

}  // namespace mafia
