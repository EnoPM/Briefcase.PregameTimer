#include "PregameTimerMod.hpp"
#include "SupportedBuild.hpp"
#include <array>
#include <format>

namespace pregame {
namespace {
static constexpr const char *deployed_signature = R"({"parameterSize":8,"flags":525313,"parameters":[
 {"name":"ToolLoadout","type":"object","offset":0}]})";
static constexpr const char *setter_signature = R"({"parameterSize":4,"flags":67240961,"parameters":[
 {"name":"SecondsLeft","type":"int32","offset":0}]})";
static constexpr const char *getter_signature = R"({"parameterSize":4,"flags":1409418241,"parameters":[
 {"name":"ReturnValue","type":"int32","offset":0,"return":true}]})";
} // namespace
void PregameTimerMod::Log(std::string_view message) const noexcept {
    if (api_ && api_->log)
        briefcase::Api(api_).log(message);
}
void PregameTimerMod::InitializeCallback(void *user) noexcept {
    static_cast<PregameTimerMod *>(user)->Initialize();
}
void PregameTimerMod::DeployedCallback(const BcHookEvent *event, void *user) noexcept {
    static_cast<PregameTimerMod *>(user)->OnDeployed(event);
}
void PregameTimerMod::DestroyedCallback(BcHandle stale, void *user) noexcept {
    static_cast<PregameTimerMod *>(user)->OnDestroyed(stale);
}
void PregameTimerMod::OnDestroyed(BcHandle stale) noexcept {
    countdown_.forget(stale); // The backend has already invalidated all references.
}
void PregameTimerMod::Clear() noexcept {
    armed_ = false;
    if (!api_ || !unreal_)
        return;
    for (auto *handle : {&hook_, &deletion_})
        if (*handle) {
            unreal_->unhook(api_->context, *handle);
            *handle = 0;
        }
    for (auto h : countdown_.handles())
        api_->release_handle(api_->context, h);
    countdown_.clear();
    for (auto *handle : {&functions_.deployment, &functions_.setTime, &functions_.getTime})
        if (*handle) {
            api_->release_handle(api_->context, *handle);
            *handle = 0;
        }
}
void PregameTimerMod::Disable(std::string_view reason) noexcept {
    Clear();
    try {
        Log(std::string("Pregame Timer DISABLED: ") + std::string(reason));
    } catch (...) {
    }
}
void PregameTimerMod::OnDeployed(const BcHookEvent *event) noexcept {
    if (!armed_ || !event || event->phase != BC_HOOK_POST)
        return;
    BcHandle retained{};
    try {
        briefcase::Unreal u(api_);
        BcObjectInfo info{};
        info.size = sizeof(info);
        briefcase::require(unreal_->object_info(api_->context, event->self, &info), "deployment object info");
        const auto phase = u.property(event->self, "GamePhase");
        const auto before = u.invoke(event->self, functions_.getTime);
        if (phase.kind != BC_VALUE_U8 || before.kind != BC_VALUE_I32)
            throw std::runtime_error("Unexpected phase/timer type");
        if (config_.diagnostics && diagnostics_++ < 12)
            Log(std::format("Deployment event: transport={}, phase={}, remaining={}, object={}",
                            event->transport == BC_TRANSPORT_EVENT ? "ProcessEvent" : "FuncThunk",
                            phase.data.integer, before.data.integer, info.path));
        if (!countdown_.eligible(event->self, phase.data.integer, pregamePhase_, before.data.integer,
                                 (info.flags & 48) != 0))
            return;
        u.retain(event->self);
        retained = event->self;
        const std::array<BcNamedValue, 1> args{{{"SecondsLeft", briefcase::i32(config_.duration)}}};
        u.invoke(event->self, functions_.setTime, args);
        const auto after = u.invoke(event->self, functions_.getTime);
        if (after.kind != BC_VALUE_I32 || after.data.integer != config_.duration)
            throw std::runtime_error(
                "Vanilla timer readback did not match configured duration; disabling timer hook");
        countdown_.mark(retained);
        retained = 0;
        Log(std::format(
            "Countdown applied once after HandleNewSpyLoadoutCompleted: before={}s, after={}s, phase={}",
            before.data.integer, after.data.integer, phase.data.integer));
    } catch (const std::exception &e) {
        if (retained)
            api_->release_handle(api_->context, retained);
        Disable(e.what());
    } catch (...) {
        if (retained)
            api_->release_handle(api_->context, retained);
        Disable("unexpected callback failure");
    }
}

void PregameTimerMod::Initialize() noexcept {
    if (!accepting_)
        return;
    try {
        briefcase::Unreal u(api_);
        briefcase::require(unreal_->enum_value(api_->context, "/Script/DeceiveInc.ESpyGamePhase",
                                               "ESpyGamePhase::PREGAME", &pregamePhase_),
                           "resolve PREGAME enum");
        functions_.deployment = u.resolve(
            "/Script/DeceiveInc.DeceiveIncMatchGameState:HandleNewSpyLoadoutCompleted", deployed_signature);
        functions_.setTime = u.resolve(
            "/Script/DeceiveInc.DeceiveIncGameStateBase:SetCurrentPhaseTimeLeftInSeconds", setter_signature);
        functions_.getTime = u.resolve(
            "/Script/DeceiveInc.DeceiveIncGameStateBase:GetCurrentPhaseTimeLeftInSeconds", getter_signature);
        deletion_ = u.deleted(DestroyedCallback, this);
        hook_ = u.hook(functions_.deployment, BC_HOOK_POST, DeployedCallback, this);
        armed_ = true;
        Log(std::format("Pregame Timer ARMED: duration={}s, PREGAME={}; waiting for vanilla first Deploy",
                        config_.duration, pregamePhase_));
    } catch (const std::exception &e) {
        Disable(e.what());
    } catch (...) {
        Disable("setup failure");
    }
}
BcResult PregameTimerMod::Load(const BcApi *api) noexcept {
    if (accepting_ || api_)
        return BC_INVALID_ARGUMENT;
    try {
        if (!server_mods::supported_build(api))
            return BC_VERSION_MISMATCH;
        api_ = api;
        const briefcase::Services services(api_);
        config_ = Config::parse(services.config(schema));
        unreal_ = &services.service<BcUnrealApi>(BC_UNREAL_SERVICE, offsetof(BcUnrealApi, write_property));
        diagnostics_ = 0;
        accepting_ = true;
        const auto result = api_->post_game_thread(api_->context, InitializeCallback, this);
        if (result != BC_OK) {
            accepting_ = false;
            api_ = nullptr;
            unreal_ = nullptr;
        }
        return result;
    } catch (const std::exception &e) {
        try {
            Log(std::string("Pregame Timer rejected: ") + e.what());
        } catch (...) {
        }
        accepting_ = false;
        api_ = nullptr;
        unreal_ = nullptr;
        return BC_INVALID_ARGUMENT;
    } catch (...) {
        accepting_ = false;
        api_ = nullptr;
        unreal_ = nullptr;
        return BC_INTERNAL;
    }
}
void PregameTimerMod::Unload() noexcept {
    accepting_ = false;
    if (!api_)
        return;
    Clear();
    Log("Pregame Timer unloaded: hook and deletion listener removed");
    api_ = nullptr;
    unreal_ = nullptr;
}
} // namespace pregame
