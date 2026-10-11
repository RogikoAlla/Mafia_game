#include "game/RolesConfig.hpp"

#include <yaml-cpp/yaml.h>

#include <string>
#include <vector>

namespace mafia {
namespace {

Role roleByName(const std::string& name) {
    if (name == "journalist") {
        return Role::Journalist;
    }
    if (name == "ninja") {
        return Role::Ninja;
    }
    if (name == "elder") {
        return Role::Elder;
    }
    return Role::Civilian;
}

bool alreadyListed(const std::vector<Role>& roles, Role role) {
    for (Role listed : roles) {
        if (listed == role) {
            return true;
        }
    }
    return false;
}

}  // namespace

std::vector<Role> loadExtraRoles(const std::string& path) {
    const YAML::Node root = YAML::LoadFile(path);
    const YAML::Node roles = root["roles"];
    std::vector<Role> extras;
    if (!roles.IsSequence()) {
        return extras;
    }
    for (std::size_t i = 0; i < roles.size(); ++i) {
        const Role role = roleByName(roles[i].as<std::string>());
        if (role == Role::Civilian || alreadyListed(extras, role)) {
            continue;
        }
        extras.push_back(role);
    }
    return extras;
}

}  // namespace mafia
