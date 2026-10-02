#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace pregame {
inline constexpr auto schema = R"({
 "type":"object","properties":{
  "durationSeconds":{"type":"integer","minimum":1,"maximum":3600,"default":90,
    "description":"Vanilla lobby countdown duration after the first Deploy. Restart required."},
  "diagnostics":{"type":"boolean","default":false,
    "description":"Log at most 12 observed deployment events per server run."}
 }
})";

struct Config {
    std::int32_t duration = 90;
    bool diagnostics = false;

    static Config parse(const std::string &text) {
        const auto value = nlohmann::json::parse(text);
        const auto &duration = value.at("durationSeconds");
        if (!duration.is_number_integer() || duration < 1 || duration > 3600 ||
            !value.at("diagnostics").is_boolean())
            throw std::runtime_error("Invalid Pregame Timer configuration");
        return {duration.get<std::int32_t>(), value.at("diagnostics").get<bool>()};
    }
};
} // namespace pregame
