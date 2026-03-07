#include "constants.h"
#include <wiringPi.h>
#include <httplib.h>
#include <expected>
#include <iostream>
#include <memory>

template <typename T>
using Result = std::expected<T, std::string>;

using Err = std::unexpected<std::string>;

struct EventLoop
{

    enum class State
    {
        OPEN,
        CLOSED
    };

    int pin{SWITCH_PIN};

    State currentState{State::CLOSED};

    EventLoop() : pin{SWITCH_PIN}
    {
        pinMode(pin, INPUT);
    }

    void run()
    {
        while (true)
        {
            auto newState{this->readPin()};
            if (currentState == State::OPEN && newState == State::CLOSED)
            {
                this->sendPauseCommand();
            }
            currentState = newState;
        }
    }

    State readPin()
    {
        return static_cast<State>(digitalRead(pin));
    }

    Result<void> sendPauseCommand()
    {
        httplib::Client cli(OCTOPRINT_ENDPOINT);

        httplib::Headers headers = {
            {"X-Api-Key", OCTOPRINT_API_KEY}};

        std::string command{"{\"command\":\"M0\" }"};

        auto res = cli.Post("/api/printer/command", headers, command, "application/json");

        if (res->status >= 400)
        {
            return Err{res->body};
        }

        return {};
    }
};

auto main() -> int
{
    auto el = std::make_unique<EventLoop>();

    el->run();

    return 0;
}
