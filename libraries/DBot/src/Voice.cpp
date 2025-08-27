#include "Voice.h"
#include <algorithm>

void Voice::setHandler(Handler h) { _handler = std::move(h); }

void Voice::play(const std::string& token) {
    std::string t = token;
    std::transform(t.begin(), t.end(), t.begin(), [](unsigned char c){ return std::tolower(c); });
    if (_handler) {
        _handler(t);
    }
}

