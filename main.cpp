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

    int pressCount{};
    enum class State
    {
        OPEN,
        CLOSED
    };

    const int msThreshold{DEBOUNCE_THRESHOLD_MS};
    const int unpauseThreshold{600};
    const int pin{SWITCH_PIN};
    State currentState{State::CLOSED};
    Time::Timestamp debounceStamp;
    bool mock{false};
    int commandCounter{};
    SimpleLogger log;

    const std::string PAUSE{"M0"};
    const std::string UNPAUSE{"M24"};

    Time::Timestamp lastCommand;

    EventLoop(bool logRun, bool mockRun) : pin{SWITCH_PIN}, mock{mockRun}, debounceStamp{Time::now()}, log{logRun}, lastCommand{Time::now()}
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
                {

                    std::string_view command{PAUSE};
                    if (this->unpauseReady())
                    {
                        log << "unpause is ready" << nl;
                        command = UNPAUSE;
                    }

                    auto res = this->sendCommand(command);
                    if (!res)
                    {
                        log << res.error() << nl;
                    }
                    else
                    {
                        log << "successfully paused: " << ++pressCount << nl;
                    }
                }

                debounceReset();
            }
            currentState = newState;
        }
    }

    State readPin()
    {
        return static_cast<State>(digitalRead(pin));
    }

    // M24

    Result<void> sendCommand(const std::string_view command)
    {
        this->lastCommand = Time::now();

        log << "command sent is" << command << nl;

        try
        {
            httplib::Client cli(OCTOPRINT_ENDPOINT);

            httplib::Headers headers = {
                {"X-Api-Key", OCTOPRINT_API_KEY}};

            std::string command{"{\"command\":\"" + command + "\" }"};

            auto res = cli.Post("/api/printer/command", headers, command, "application/json");

            if (!res)
                return Err{"connection failed"};

            if (res->status >= 400)
            {
                return Err{res->body};
            }
        }
        catch (const std::exception &e)
        {
            return Err{e.what()};
        }
        catch (...)
        {
            return Err{"generic error"};
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

    bool unpauseReady()
    {
        auto now = Time::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCommand).count();
        log << "elapsed time (unpause): " << elapsed << nl;
        return elapsed <= unpauseThreshold;
    }
    void debounceReset()
    {
        log << "debouce resetting" << nl;
        this->debounceStamp = Time::now();
    }
};

auto main(int argc, char *argv[]) -> int
{

    bool log{};
    bool mock{argc > 1 && std::string(argv[1]) == "mock"};
    if (mock)
    {
        log = true;
    }
    else if (argc > 1 && std::string(argv[1]) == "log")
    {
        log = true;
    }

    auto el = std::make_unique<EventLoop>(log, mock);

    el->run();

    return 0;
}
