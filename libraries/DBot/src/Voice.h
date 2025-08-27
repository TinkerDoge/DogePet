#pragma once
#include <string>
#include <functional>

class Voice {
public:
    using Handler = std::function<void(const std::string&)>;

    void setHandler(Handler h);
    void play(const std::string& token);

private:
    Handler _handler;
};

