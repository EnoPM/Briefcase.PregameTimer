#include "MockHost.hpp"
static Json timerConfig() { return {{"durationSeconds",30},{"diagnostics",true}}; }
static void timer() {
    {
        Backend b(timerConfig());
        Module mod(L"Briefcase.PregameTimer.dll");
        check(mod.InvalidLoad() == BC_VERSION_MISMATCH, "Timer rejects missing host");
        check(mod.Load(b) == BC_OK, "Timer loads");
        check(b.hooks.empty(), "Timer initialization is deferred");
        b.pump();
        check(b.hooks.size() == 1 && b.deleted.size() == 1,
              "Timer registers one event and deletion callback");
        b.objects[1].phase = 0;
        b.event(":HandleNewSpyLoadoutCompleted", BC_HOOK_POST);
        check(b.timerWrites == 0, "Timer ignores non-pregame phase");
        b.objects[1].phase = 1;
        b.objects[1].flags = 16;
        b.event(":HandleNewSpyLoadoutCompleted", BC_HOOK_POST);
        check(b.timerWrites == 0, "Timer ignores default objects");
        b.objects[1].flags = 0;
        b.event(":HandleNewSpyLoadoutCompleted", BC_HOOK_POST);
        check(b.objects[1].timer == 30 && b.timerWrites == 1 && b.refs[1] == 2,
              "Timer applies once and retains match");
        b.objects[1].timer = 20;
        b.event(":HandleNewSpyLoadoutCompleted", BC_HOOK_POST);
        check(b.objects[1].timer == 20 && b.timerWrites == 1, "Later deployment does not restart countdown");
        b.remove(1);
        b.event(":HandleNewSpyLoadoutCompleted", BC_HOOK_POST, 2);
        check(b.objects[2].timer == 30 && b.timerWrites == 2, "New match instance receives countdown");
        mod.Stop();
        check(b.clean(), "Timer releases hooks and references");
        check(mod.Load(b) == BC_OK, "Timer can create a new session after stop");
        b.pump();
        b.objects[2].timer = 90;
        b.event(":HandleNewSpyLoadoutCompleted", BC_HOOK_POST, 2);
        check(b.objects[2].timer == 30 && b.timerWrites == 3, "Timer state does not leak into next session");
        mod.Stop();
        check(b.clean(), "Second timer stop is clean");
    }
    for (unsigned failure = 1; failure <= 4; ++failure) {
        Backend b(timerConfig());
        Module mod(L"Briefcase.PregameTimer.dll");
        if (failure <= 3)
            b.failResolve = failure;
        else
            b.failHook = 1;
        check(mod.Load(b) == BC_OK, "Timer accepts deferred setup");
        b.pump();
        check(b.clean(), "Timer partial initialization cleaned immediately");
        mod.Stop();
        check(b.clean(), "Timer failed setup stop idempotent");
    }
    {
        Backend b(timerConfig());
        Module mod(L"Briefcase.PregameTimer.dll");
        check(mod.Load(b) == BC_OK, "Timer loads before cancellation");
        mod.Stop();
        b.pump();
        check(b.resolves == 0 && b.clean(), "Queued callback after stop does not initialize");
    }
    {
        Backend b(timerConfig());
        Module mod(L"Briefcase.PregameTimer.dll");
        b.rejectPost = true;
        check(mod.Load(b) == BC_DENIED && !b.task, "Timer reports queue rejection");
        b.rejectPost = false;
        check(mod.Load(b) == BC_OK, "Timer retries after queue rejection");
        b.pump();
        b.rejectReadback = true;
        b.event(":HandleNewSpyLoadoutCompleted", BC_HOOK_POST);
        check(b.clean(), "Timer readback failure disables and releases registrations");
        mod.Stop();
        check(b.clean(), "Timer readback failure teardown");
    }
}
static void prefixCompatibility() {
    {
        Backend b(timerConfig());
        Module mod(L"Briefcase.PregameTimer.dll");
        b.unreal.size = offsetof(BcUnrealApi, write_property);
        check(mod.Load(b) == BC_OK, "Timer accepts its original read/hook ABI prefix");
        b.pump();
        b.event(":HandleNewSpyLoadoutCompleted", BC_HOOK_POST);
        check(b.objects[1].timer == 30, "Timer works without write/client extensions");
        mod.Stop();
        check(b.clean(), "Original-prefix timer cleanup");
    }
}
int main() { try { timer(); prefixCompatibility(); std::cout << "PASS " << checks << " lifecycle checks\n"; return 0; } catch(const std::exception& e) { std::cerr << e.what() << "\n"; return 1; } }
