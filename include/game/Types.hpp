#pragma once

namespace mafia {

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
