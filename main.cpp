#include "constants.h"
#include <wiringPi.h>
#include <httplib.h>
#include <expected>
#include <iostream>
#include <memory>

#define nl "\n"

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
    bool mock{false};

    EventLoop(bool mockRun = true) : pin{SWITCH_PIN}, mock{mockRun}
    {
        wiringPiSetup();
        pinMode(pin, INPUT);
        pullUpDnControl(pin, PUD_UP);
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
            if (mock)
                std::cout << "new state is: " << static_cast<int>(newState) << nl;

            if (currentState == State::OPEN && newState == State::CLOSED)
            {
                if (mock)
                    std::cout << "send pause command!" << std::endl;
                else
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

auto main(int argc, char *argv[]) -> int
{

    bool mock{argc > 1 && std::string(argv[1]) == "test"};

    auto el = std::make_unique<EventLoop>(mock);

    el->run();

    return 0;
}
