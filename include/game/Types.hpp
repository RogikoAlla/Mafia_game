#pragma once

namespace mafia {

// Фаза партии. Её переключает ведущий, игроки только ждут свою.
enum class Phase {
    DayTalk,
    DayVote,
    Night,
    Finished,
};

}  // namespace mafia
