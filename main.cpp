#include "constants.h"
#include <wiringPi.h>
#include <httplib.h>
#include <expected>
#include <iostream>
#include <memory>
#include <chrono>

#define nl "\n"

template <typename T>
using Result = std::expected<T, std::string>;

using Err = std::unexpected<std::string>;

namespace Time
{
    using Timestamp = std::chrono::_V2::steady_clock::time_point;

    Timestamp now()
    {
        return std::chrono::steady_clock::now();
    }

}

struct EventLoop
{

    enum class State
    {
        OPEN,
        CLOSED
    };

    const int msThreshold{400};
    const int pin{SWITCH_PIN};
    State currentState{State::CLOSED};
    Time::Timestamp debounceStamp;
    bool mock{false};
    int commandCounter{};

    EventLoop(bool mockRun = true) : pin{SWITCH_PIN}, mock{mockRun}, debounceStamp{Time::now()}
    {
        if (mock)
            std::cout << "pin is: " << pin << nl;

        wiringPiSetup();
        pinMode(pin, INPUT);
        pullUpDnControl(pin, PUD_DOWN);
    }

    void run()
    {
        if (mock)
        {
            std::cout << "starting loop" << nl;
        }

        while (true)
        {

            auto newState{this->readPin()};

            if (currentState == State::OPEN && newState == State::CLOSED && this->debounceReady())
            {

                if (mock)
                {
                    std::cout << "send pause command: " << ++commandCounter << std::endl;
                }
                else
                    this->sendPauseCommand();

                debounceReset();
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

    bool debounceReady()
    {
        auto now = Time::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->debounceStamp).count();
        return elapsed > msThreshold;
    }
    void debounceReset()
    {
        this->debounceStamp = Time::now();
    }
};

auto main(int argc, char *argv[]) -> int
{

    bool mock{argc > 1 && std::string(argv[1]) == "test"};

    auto el = std::make_unique<EventLoop>(mock);

    el->run();

    return 0;
}
