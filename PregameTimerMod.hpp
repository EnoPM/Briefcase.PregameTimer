#pragma once
#include "Countdown.hpp"
#include <Briefcase/Unreal.hpp>

namespace pregame {
// The instance has a stable address while callbacks are registered.
// Load schedules initialization; all Unreal work and Unload run on the game thread.
class PregameTimerMod {
    const BcApi *api_{};
    const BcUnrealApi *unreal_{};
    Config config_;
    Countdown countdown_;
    struct Functions {
        BcHandle deployment{}, setTime{}, getTime{};
    } functions_;
    BcHandle hook_{}, deletion_{};
    int64_t pregamePhase_{};
    uint32_t diagnostics_{};
    bool accepting_{}, armed_{};

    void Initialize() noexcept;
    void OnDeployed(const BcHookEvent *event) noexcept;
    void OnDestroyed(BcHandle stale) noexcept;
    void Clear() noexcept;
    void Log(std::string_view message) const noexcept;
    void Disable(std::string_view reason) noexcept;

    static void BC_CALL InitializeCallback(void *user) noexcept;
    static void BC_CALL DeployedCallback(const BcHookEvent *event, void *user) noexcept;
    static void BC_CALL DestroyedCallback(BcHandle stale, void *user) noexcept;

  public:
    PregameTimerMod() = default;
    PregameTimerMod(const PregameTimerMod &) = delete;
    PregameTimerMod &operator=(const PregameTimerMod &) = delete;
    BcResult Load(const BcApi *api) noexcept;
    void Unload() noexcept;
};
} // namespace pregame
