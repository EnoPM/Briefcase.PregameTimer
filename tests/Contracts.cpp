#include "../Countdown.hpp"
#include "ConfigDefaults.hpp"
#include <iostream>
static int checks;
void check(bool b) {
    ++checks;
    if (!b)
        throw std::runtime_error("Pregame contract #" + std::to_string(checks));
}
template <class F> void rejects(F f) {
    bool failed = false;
    try {
        f();
    } catch (...) {
        failed = true;
    }
    check(failed);
}
int main() {
    try {
        auto schema = nlohmann::json::parse(pregame::schema);
        auto defaults = config_defaults(schema);
        const auto config = pregame::Config::parse(defaults.dump());
        check(config.duration == 90 && !config.diagnostics);
        for (int seconds = 1; seconds <= 3600; ++seconds) {
            auto j = defaults;
            j["durationSeconds"] = seconds;
            check(pregame::Config::parse(j.dump()).duration == seconds);
        }
        for (auto value : {nlohmann::json(0), nlohmann::json(3601), nlohmann::json(1.5), nlohmann::json(true),
                           nlohmann::json("90")}) {
            auto j = defaults;
            j["durationSeconds"] = value;

            rejects([&] { pregame::Config::parse(j.dump()); });
        }
        pregame::Countdown state;
        check(state.handles().empty());
        check(!state.eligible(1, 0, 1, 60)); // loading
        check(!state.eligible(1, 1, 1, 0));  // no running countdown
        check(!state.eligible(1, 1, 1, -1));
        check(!state.eligible(1, 3, 1, 60)); // playable match
        check(!state.eligible(1, 1, 1, 60, true));
        check(state.eligible(1, 1, 1, 60));
        state.mark(1);
        check(!state.eligible(1, 1, 1, 60)); // later deployments cannot restart the timer
        check(state.forget(1));
        check(state.eligible(2, 1, 1, 60)); // new generation after travel
        for (uint64_t i = 1; i <= 64; ++i)
            state.mark(i);
        check(!state.eligible(65, 1, 1, 60));
        state.clear();
        check(state.eligible(65, 1, 1, 60));
        std::cout << "PASS " << checks << " Pregame Timer checks\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
