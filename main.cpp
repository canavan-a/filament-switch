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

struct SimpleLogger
{
    bool show{};

    SimpleLogger(bool showLoggerOutput) : show{showLoggerOutput}
    {
    }

    template <typename T>
    SimpleLogger &operator<<(const T &val)
    {
        if (show)
            std::cout << val;
        return *this;
    }
};

struct EventLoop
{

    enum class State
    {
        OPEN,
        CLOSED
    };

    const int msThreshold{DEBOUNCE_THRESHOLD_MS};
    const int pin{SWITCH_PIN};
    State currentState{State::CLOSED};
    Time::Timestamp debounceStamp;
    bool mock{false};
    int commandCounter{};
    SimpleLogger log;

    EventLoop(bool mockRun = true) : pin{SWITCH_PIN}, mock{mockRun}, debounceStamp{Time::now()}, log{mockRun}
    {
        log << "pin is: " << pin << nl;

        wiringPiSetup();
        pinMode(pin, INPUT);
        pullUpDnControl(pin, PUD_DOWN);
    }

    void run()
    {

        log << "starting loop" << nl;

        while (true)
        {

            auto newState{this->readPin()};

            if (currentState == State::OPEN && newState == State::CLOSED && this->debounceReady())
            {

                if (mock)
                {
                    log << "send pause command: " << ++commandCounter << nl;
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
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - debounceStamp).count();
        log << "elapsed time: " << elapsed << nl;
        return elapsed >= msThreshold;
    }
    void debounceReset()
    {
        log << "debouce resetting" << nl;
        this->debounceStamp = Time::now();
    }
};

auto main(int argc, char *argv[]) -> int
{

    bool mock{argc > 1 && std::string(argv[1]) == "mock"};

    auto el = std::make_unique<EventLoop>(mock);

    el->run();

    return 0;
}
