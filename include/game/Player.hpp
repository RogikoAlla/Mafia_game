#pragma once

#include "smart_ptr/SharedPtr.hpp"
#include "game/GameState.hpp"

#include <memory>

namespace mafia {

// Базовый игрок. Бот говорит и выбирает цель сам. Человек читает ход из stdin.
class Player {
public:
    Player(SharedPtr<GameState> state, int playerId, bool interactive = false);
    virtual ~Player() = default;

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    // Ждёт фазы ведущего, пока партия не закончена.
    void run();

    virtual void talk();
    virtual void vote();
    virtual void actNight();

    int playerId() const { return playerId_; }
    bool interactive() const { return interactive_; }

protected:
    // Случайный живой id, кроме себя. 0, если цели нет.
    virtual int chooseVoteTarget() const;
    std::string readLine() const;
    int readTargetId() const;

    SharedPtr<GameState> state_;
    int playerId_ = 0;
    bool interactive_ = false;
};

class Mafia : public Player {
public:
    using Player::Player;
    void actNight() override;

protected:
    // Случайный живой не из мафии.
    int chooseVoteTarget() const override;

private:
    // Повторяет вопрос, пока цель не станет живым не из мафии. Конец ввода — 0.
    int readKillTarget() const;
};

class Journalist : public Player {
public:
    using Player::Player;
    void actNight() override;

private:
    int readOther(int exceptId) const;
};

class Ninja : public Mafia {
public:
    using Mafia::Mafia;
};

class Elder : public Player {
public:
    using Player::Player;
};

class Civilian : public Player {
public:
    using Player::Player;
};

class Commissioner : public Player {
public:
    using Player::Player;
    void actNight() override;

protected:
    // Живая мафия, которую он проверил или не смог застрелить. Иначе случайный живой.
    int chooseVoteTarget() const override;
};

class Doctor : public Player {
public:
    using Player::Player;
    void actNight() override;
};

class Maniac : public Player {
public:
    using Player::Player;
    void actNight() override;
};

// Роль уже должна быть раздана. interactive не меняет класс роли.
std::unique_ptr<Player> makePlayer(SharedPtr<GameState> state, int playerId, bool interactive = false);

// У типа должны быть те же ходы, что у игрока. Проверка при компиляции.
template<typename T>
concept PlayerLike = requires(T& player) {
    player.talk();
    player.vote();
    player.actNight();
};

template<PlayerLike T>
void takeNightAction(T& player) {
    player.actNight();
}

}  // namespace mafia
