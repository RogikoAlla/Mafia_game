#pragma once

namespace mafia {

// Роль игрока. Id с 1. Мафии — max(1, N/3), затем доктор, комиссар, маньяк, остальные мирные.
enum class Role {
    Mafia,
    Doctor,
    Commissioner,
    Maniac,
    Civilian,
    Journalist,
    Ninja,
    Elder,
};

// Ниндзя — мафия сверх формулы N/3. Журналист и старейшина — мирные.
inline bool mafiaCamp(Role role) {
    return role == Role::Mafia || role == Role::Ninja;
}

inline const char* roleName(Role role) {
    switch (role) {
    case Role::Mafia:
        return "мафия";
    case Role::Doctor:
        return "доктор";
    case Role::Commissioner:
        return "комиссар";
    case Role::Maniac:
        return "маньяк";
    case Role::Civilian:
        return "мирный";
    case Role::Journalist:
        return "журналист";
    case Role::Ninja:
        return "ниндзя";
    case Role::Elder:
        return "старейшина";
    }
    return "неизвестная роль";
}

// Кто выиграл. None — партия ещё идёт.
enum class Winner {
    None,
    Town,
    Mafia,
    Maniac,
};

inline const char* winnerName(Winner winner) {
    switch (winner) {
    case Winner::None:
        return "никто";
    case Winner::Town:
        return "мирные";
    case Winner::Mafia:
        return "мафия";
    case Winner::Maniac:
        return "маньяк";
    }
    return "неизвестный победитель";
}

// Фаза партии. Её переключает ведущий, игроки только ждут свою.
enum class Phase {
    DayTalk,
    DayVote,
    Night,
    Finished,
};

inline const char* phaseName(Phase phase) {
    switch (phase) {
    case Phase::DayTalk:
        return "дневное обсуждение";
    case Phase::DayVote:
        return "дневное голосование";
    case Phase::Night:
        return "ночь";
    case Phase::Finished:
        return "конец";
    }
    return "неизвестная фаза";
}

}  // namespace mafia
