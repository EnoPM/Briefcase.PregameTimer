#include "../PregameTimerConfig.hpp"
#include "ModVersion.hpp"

#include <Briefcase/DeceiveInc/MatchState.hpp>
#include <Briefcase/DeceiveInc/Paths.hpp>
#include <DynamicOutput/Output.hpp>
#include <Helpers/String.hpp>
#include <Mod/CppUserModBase.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_set>

namespace {
using namespace RC;
using namespace RC::Unreal;
using briefcase::deceive::MatchState;

constexpr auto deployment_path =
    STR("/Script/DeceiveInc.DeceiveIncMatchGameState:HandleNewSpyLoadoutCompleted");

extern "C" __declspec(dllexport) RC::CppUserModBase *start_mod();

pregame::Config load_config() {
    const auto path = briefcase::deceive::configuration_file(
        reinterpret_cast<const void *>(&start_mod), "briefcase.pregame-timer", "config.json");
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Missing Data/config.json");
    const std::string text{std::istreambuf_iterator<char>(input), {}};
    return pregame::Config::parse(text);
}

class PregameTimerUe4ss final : public CppUserModBase {
  public:
    PregameTimerUe4ss() : config_(load_config()) {
        ModName = STR("Briefcase.PregameTimer");
        ModVersion = briefcase_mod_version;
        ModDescription = STR("Configurable Deceive Inc pregame timer");
        ModAuthors = STR("EnoPM");
    }

    ~PregameTimerUe4ss() override {
        if (function_)
            UObjectGlobals::UnregisterHook(function_, hook_ids_);
    }

    void on_unreal_init() override { try_install(); }

    void on_update() override {
        if (function_ || std::chrono::steady_clock::now() < next_attempt_)
            return;
        next_attempt_ = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        try_install();
    }

  private:
    pregame::Config config_;
    UFunction *function_{};
    std::pair<int, int> hook_ids_{};
    std::unordered_set<UObject *> configured_;
    std::uint8_t pregame_phase_{0xff};
    std::chrono::steady_clock::time_point next_attempt_{};

    void try_install() noexcept {
        try {
            auto *function = UObjectGlobals::StaticFindObject<UFunction *>(nullptr, nullptr, deployment_path);
            if (!function)
                return;
            pregame_phase_ = briefcase::deceive::resolve_match_phase(L"PREGAME");
            hook_ids_ = UObjectGlobals::RegisterHook(
                function, [](UnrealScriptFunctionCallableContext &, void *) {},
                [this](UnrealScriptFunctionCallableContext &context, void *) { on_deployed(context.Context); },
                nullptr);
            function_ = function;
            Output::send(STR("[Briefcase.PregameTimer] ready: duration={}s, diagnostics={}\n"),
                         config_.duration, config_.diagnostics);
        } catch (const std::exception &error) {
            Output::send<LogLevel::Warning>(STR("[Briefcase.PregameTimer] initialization failed: {}\n"),
                                            RC::to_wstring(error.what()));
        } catch (...) {
            Output::send<LogLevel::Warning>(STR("[Briefcase.PregameTimer] initialization failed\n"));
        }
    }

    void on_deployed(UObject *object) noexcept {
        try {
            MatchState state(object);
            if (!state || state.is_template() || static_cast<std::uint8_t>(state.phase()) != pregame_phase_)
                return;
            const auto before = state.remaining_seconds();
            if (before <= 0 || configured_.contains(object))
                return;
            state.set_remaining_seconds(config_.duration);
            const auto after = state.remaining_seconds();
            if (after != config_.duration)
                throw std::runtime_error("Timer readback mismatch");
            configured_.insert(object);
            Output::send(STR("[Briefcase.PregameTimer] countdown changed from {}s to {}s\n"), before, after);
        } catch (const std::exception &error) {
            Output::send<LogLevel::Warning>(STR("[Briefcase.PregameTimer] event rejected: {}\n"),
                                            RC::to_wstring(error.what()));
        } catch (...) {
            Output::send<LogLevel::Warning>(STR("[Briefcase.PregameTimer] event rejected\n"));
        }
    }
};
} // namespace

extern "C" __declspec(dllexport) RC::CppUserModBase *start_mod() {
    try {
        return new PregameTimerUe4ss();
    } catch (const std::exception &error) {
        RC::Output::send<RC::LogLevel::Warning>(STR("[Briefcase.PregameTimer] load failed: {}\n"),
                                                RC::to_wstring(error.what()));
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

extern "C" __declspec(dllexport) void uninstall_mod(RC::CppUserModBase *mod) { delete mod; }
