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
    int pin{SWITCH_PIN};

    void
    run()
    {
        while (true)
        {
        }
    }

    // POST /api/printer/command HTTP/1.1
    // Host: example.com
    // Content-Type: application/json
    // X-Api-Key: abcdef...

    // {
    // "command": "M106"
    // }
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
    auto ptr = std::make_unique<EventLoop>();

    ptr->run();

    return 0;
}
