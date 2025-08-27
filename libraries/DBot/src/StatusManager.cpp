#include "StatusManager.h"

using Clock = std::chrono::steady_clock;

void StatusManager::set(Status s, std::chrono::milliseconds hold) {
    _status = s;
    _until = hold.count() ? Clock::now() + hold : Clock::time_point{};
    if (_handler) _handler(_status);
}

void StatusManager::update() {
    if (_until.time_since_epoch().count() && Clock::now() >= _until) {
        _status = Status::Idle;
        _until = Clock::time_point{};
        if (_handler) _handler(_status);
    }
}

void StatusManager::onChange(ChangeHandler h) { _handler = std::move(h); }

