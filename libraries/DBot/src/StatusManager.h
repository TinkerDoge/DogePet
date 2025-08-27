#pragma once
#include <functional>
#include <chrono>

class StatusManager {
public:
    enum class Status { Idle, Happy, Angry, Furious, Tired, Sleep, Curious, Notified };

    using ChangeHandler = std::function<void(Status)>;

    void set(Status s, std::chrono::milliseconds hold = std::chrono::milliseconds(0));
    Status current() const { return _status; }

    void update();
    void onChange(ChangeHandler h);

private:
    Status _status = Status::Idle;
    std::chrono::steady_clock::time_point _until{};
    ChangeHandler _handler;
};

