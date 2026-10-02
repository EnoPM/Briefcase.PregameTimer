#pragma once
#include "PregameTimerConfig.hpp"
#include <Briefcase/ModApi.h>
#include <unordered_set>
namespace pregame {
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
