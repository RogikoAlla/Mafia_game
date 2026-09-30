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
};

class Civilian : public Player {
public:
    using Player::Player;
};

class Commissioner : public Player {
public:
    using Player::Player;
    void actNight() override;
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

}  // namespace mafia
