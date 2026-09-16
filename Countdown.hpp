#pragma once
#include <Briefcase/ModApi.h>
#include <nlohmann/json.hpp>
#include <unordered_set>
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
    int32_t duration = 90;
    bool diagnostics = false;
    static Config parse(const std::string &text) {
        auto j = nlohmann::json::parse(text);
        const auto &v = j.at("durationSeconds");
        if (!v.is_number_integer() || v < 1 || v > 3600 || !j.at("diagnostics").is_boolean())
            throw std::runtime_error("Invalid Pregame Timer configuration");
        return {v.get<int32_t>(), j.at("diagnostics").get<bool>()};
    }
};
class Countdown {
    std::unordered_set<BcHandle> applied_;

  public:
    bool eligible(BcHandle self, int64_t phase, int64_t pregame_phase, int64_t remaining,
                  bool is_default = false) const {
        return self && !is_default && phase == pregame_phase && remaining > 0 && !applied_.contains(self) &&
               applied_.size() < 64;
    }
    void mark(BcHandle self) { applied_.insert(self); }
    bool forget(BcHandle self) { return applied_.erase(self) != 0; }
    const auto &handles() const { return applied_; }
    void clear() { applied_.clear(); }
};
} // namespace pregame
