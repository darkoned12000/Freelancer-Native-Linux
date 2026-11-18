// M8.7 guarded upstream client/library bootstrap integration.
// Progresses toward upstream client init/bootstrap by invoking linked upstream
// InitializeEngine (with temporary single-player-safe stubs/fallbacks) when available.

#include "linux_port/platform.h"

#include <dacom.h>
#include <system.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "freelancer/Client/globals.h"
#include "freelancer/Client/intro_connect.h"
#include "freelancer/Client/client_state.h"
#include "freelancer/Client/space_state.h"
#include "freelancer/Client/gameflow.h"
#include "freelancer/Client/mission_briefing.h"
#include "freelancer/Client/client_frame_tick.h"

#if defined(__GNUC__) || defined(__clang__)
#define FL_M87_EXPORT __attribute__((visibility("default")))
#else
#define FL_M87_EXPORT
#endif

// Upstream client init entry point (src/freelancer/Client/engine_init.cpp), linked in M8.7 bring-up.
extern bool InitializeEngine(const FClientCommandLine* CommandLine);

namespace {
struct FPortableSystemContainerAggDesc {
  unsigned long Size;
  const char* ComponentName;
  const void* IniPath;
  unsigned long Reserved0C;
  unsigned long Reserved10;
  unsigned long Reserved14;
  unsigned long Reserved18;
};
}

extern "C" FL_M87_EXPORT int fl_m87_try_invoke_upstream_bootstrap(void* /*native_window_or_null*/,
                                                                  const char* ini_path_or_null) {
  ICOManager* dacom = DACOM_Acquire();
  if (!dacom) {
    fl_port::log_message(fl_port::LogLevel::Warning,
                         "M8.7 bootstrap hook: DACOM unavailable");
    return 0;
  }

  bool exercised_any = false;

  // Best-effort upstream SystemContainer creation (registered by linked upstream system TU).
  IDAComponent* container = nullptr;
  FPortableSystemContainerAggDesc agg{};
  agg.Size = 0x1c;
  agg.ComponentName = "SystemContainer";
  agg.IniPath = ini_path_or_null;
  const GENRESULT gr_sys = dacom->CreateInstance(reinterpret_cast<AGGDESC*>(&agg),
                                                 reinterpret_cast<void**>(&container));
  if (gr_sys == GR_OK && container) {
    exercised_any = true;
    fl_port::log_message(fl_port::LogLevel::Info,
                         "M8.7 bootstrap hook: upstream SystemContainer created (best-effort)");
    auto qi_log = [&](const char* iid, const char* label) {
      void* out = nullptr;
      if (container->QueryInterface(iid, &out) == GR_OK && out) {
        reinterpret_cast<IDAComponent*>(out)->Release();
        fl_port::log_message(fl_port::LogLevel::Info,
                             std::string("M8.7 bootstrap hook: acquired ") + label);
      }
    };
    qi_log("Engine", "Engine");
    qi_log("IEngine", "IEngine");
    qi_log("IEngine2", "IEngine2");
    qi_log("IHardpoint", "IHardpoint");
    qi_log("IAnimation", "IAnimation");
    container->Release();
  } else {
    fl_port::log_message(fl_port::LogLevel::Warning,
                         std::string("M8.7 bootstrap hook: SystemContainer unavailable gr=") +
                         std::to_string((int)gr_sys));
  }

  // Prepare upstream globals expected by InitializeEngine (config/data paths, options).
  FClientCommandLine cmd;
  cmd.Count = 0;
  std::memset(cmd.Argv, 0, sizeof(cmd.Argv));
  std::memset(cmd.Buffer, 0, sizeof(cmd.Buffer));
  try {
    LoadStartupConfig(&g_display, &cmd);
    if (ini_path_or_null && *ini_path_or_null) {
      // Prefer resolved freelancer.ini path from bring-up flow when provided.
      g_display.ini_path.Length = CopyBoundedString(g_display.ini_path.Text, 0x104, ini_path_or_null);
    }
    fl_port::log_message(fl_port::LogLevel::Info,
                         std::string("M8.7 bootstrap hook: startup config ini='") + g_display.ini_path.Text +
                         "' data='" + g_display.data_path.Text + "'");
  } catch (...) {
    fl_port::log_message(fl_port::LogLevel::Warning,
                         "M8.7 bootstrap hook: LoadStartupConfig threw (continuing)");
  }

  // Invoke linked upstream InitializeEngine in a guarded single-player-safe mode.
  // Many dependencies are currently stubbed; treat failure as non-fatal for bring-up fallback.
  fl_port::log_message(fl_port::LogLevel::Info,
                       "M8.7 bootstrap hook: invoking upstream InitializeEngine (guarded)");
  bool init_ok = false;
  try {
    init_ok = InitializeEngine(&cmd);
  } catch (...) {
    fl_port::log_message(fl_port::LogLevel::Error,
                         "M8.7 bootstrap hook: upstream InitializeEngine threw (caught)");
    return exercised_any ? 1 : 0;
  }

  if (init_ok) {
    fl_port::log_message(fl_port::LogLevel::Info,
                         "M8.7 bootstrap hook: upstream InitializeEngine reported success");

    // Task 1 (M8.7): exercise real upstream runtime/game-flow consumers beyond init-subset probes.
    // Guarded to keep bring-up stable while upstream state/UI/render integration remains incomplete.
    try {
      const int initial_env = ComputeInitialEnvironment();
      char msg[128];
      std::snprintf(msg, sizeof(msg),
                    "M8.7 runtime consumer probe: ComputeInitialEnvironment() => %d", initial_env);
      fl_port::log_message(fl_port::LogLevel::Info, msg);
    } catch (...) {
      fl_port::log_message(fl_port::LogLevel::Warning,
                           "M8.7 runtime consumer probe: ComputeInitialEnvironment threw (ignored)");
    }

    try {
      // Exercise upstream timing consumers (frame timer/timing state).
      TickClientFrameTimer();
      UpdateClientTiming();
      const double frame_cap = GetClientFrameTimeCapped();
      char msg[160];
      std::snprintf(msg, sizeof(msg),
                    "M8.7 runtime consumer probe: timing tick ok (frame_time_capped=%f)", (double)frame_cap);
      fl_port::log_message(fl_port::LogLevel::Info, msg);
    } catch (...) {
      fl_port::log_message(fl_port::LogLevel::Warning,
                           "M8.7 runtime consumer probe: timing consumers threw (ignored)");
    }

    try {
      // Probe access to upstream client state singletons (runtime consumer/state wiring).
      IEnvironment* intro = nullptr;
      IEnvironment* space = nullptr;
      IEnvironment* gf = nullptr;
      IEnvironment* brief = nullptr;
      try { intro = GetIntroFocusState(); } catch (...) {}
      try { space = GetSpaceState(); } catch (...) {}
      try { gf = GetGameflowState(); } catch (...) {}
      try { brief = GetMissionBriefingState(); } catch (...) {}
      char msg[192];
      std::snprintf(msg, sizeof(msg),
                    "M8.7 runtime consumer probe: state singletons intro=%p space=%p gf=%p briefing=%p",
                    (void*)intro, (void*)space, (void*)gf, (void*)brief);
      fl_port::log_message(fl_port::LogLevel::Info, msg);
    } catch (...) {
      fl_port::log_message(fl_port::LogLevel::Warning,
                           "M8.7 runtime consumer probe: state singleton access threw (ignored)");
    }

    // Task 1/2 (M8.7): exercise upstream initial-state selection/transition path.
    // Uses real ComputeInitialEnvironment (Linux-guarded) and EnterClientState (bring-up stub
    // until real client state TUs are linked), storing result for downstream integration probes.
    try {
      const int initial_env = ComputeInitialEnvironment();
      IEnvironment* entered = nullptr;
      try {
        entered = EnterClientState(initial_env, g_client.current_state);
      } catch (...) {
        fl_port::log_message(fl_port::LogLevel::Warning,
                             "M8.7 bootstrap: EnterClientState threw during initial-state probe");
      }
      g_client.current_state = entered;
      char msg[160];
      std::snprintf(msg, sizeof(msg),
                    "M8.7 bootstrap: initial env=%d entered_state=%p (stored in g_client.current_state)",
                    initial_env, (void*)entered);
      fl_port::log_message(fl_port::LogLevel::Info, msg);
    } catch (...) {
      fl_port::log_message(fl_port::LogLevel::Warning,
                           "M8.7 bootstrap: initial-state transition probe threw (ignored)");
    }

    // Task 1/2 (M8.7): guarded probe of upstream per-frame/game-flow consumer (TickClientFrame).
    // Exercises real frame tick logic against current-state object; failures are non-fatal.
    try {
      TickClientFrame(0.0);
      TickClientFrame(1.0 / 60.0);
      fl_port::log_message(fl_port::LogLevel::Info,
                           "M8.7 runtime consumer probe: TickClientFrame (real/guarded state) completed");
    } catch (...) {
      fl_port::log_message(fl_port::LogLevel::Warning,
                           "M8.7 runtime consumer probe: TickClientFrame threw (ignored)");
    }

    // Task 1/2 (M8.7/M8.8): exercise real EnterClientState cases (2/3/4/5/7/8) under guarded
    // runtime behavior with real GF/briefing/space state singletons linked. Restore intro/menu
    // state afterward to preserve controlled SDL/Vulkan bring-up flow.
    try {
      auto log_case = [](int case_id, const char* label, IEnvironment* prev, IEnvironment* next) {
        char msg[256];
        std::snprintf(msg, sizeof(msg),
                      "M8.7/M8.8 transition probe: case=%d (%s) prev=%p next=%p%s",
                      case_id, label ? label : "?", (void*)prev, (void*)next,
                      (next == prev ? " [unchanged]" : ""));
        fl_port::log_message(fl_port::LogLevel::Info, msg);
      };

      IEnvironment* cur = g_client.current_state;

      // Case 4: GF slot 10, returns previous unchanged.
      try {
        IEnvironment* next = EnterClientState(4, cur);
        log_case(4, "GF Slot10", cur, next);
        cur = next ? next : cur;
      } catch (...) {
        fl_port::log_message(fl_port::LogLevel::Warning,
                             "M8.7/M8.8 transition probe: case 4 threw (ignored)");
      }

      // Case 5: Space slot 10, returns previous unchanged.
      try {
        IEnvironment* next = EnterClientState(5, cur);
        log_case(5, "Space Slot10", cur, next);
        cur = next ? next : cur;
      } catch (...) {
        fl_port::log_message(fl_port::LogLevel::Warning,
                             "M8.7/M8.8 transition probe: case 5 threw (ignored)");
      }

      // Case 3: enter base/GF state (from intro/menu context preferred; bFromIntro derived).
      try {
        IEnvironment* prev = cur;
        IEnvironment* next = EnterClientState(3, prev);
        log_case(3, "EnterBaseState/GF", prev, next);
        if (next) cur = next;
      } catch (...) {
        fl_port::log_message(fl_port::LogLevel::Warning,
                             "M8.7/M8.8 transition probe: case 3 threw (ignored)");
      }

      // Case 8: enter mission briefing from current state (typically GF in upstream flow).
      try {
        IEnvironment* prev = cur;
        IEnvironment* next = EnterClientState(8, prev);
        log_case(8, "EnterMissionBriefingState", prev, next);
        if (next) cur = next;
      } catch (...) {
        fl_port::log_message(fl_port::LogLevel::Warning,
                             "M8.7/M8.8 transition probe: case 8 threw (ignored)");
      }

      // Case 2: enter in-game/space state.
      try {
        IEnvironment* prev = cur;
        IEnvironment* next = EnterClientState(2, prev);
        log_case(2, "EnterInGameState/Space", prev, next);
        if (next) cur = next;
      } catch (...) {
        fl_port::log_message(fl_port::LogLevel::Warning,
                             "M8.7/M8.8 transition probe: case 2 threw (ignored)");
      }

      // Case 7: load-game restart path (guarded in Linux TU; expected safe failure/no transition).
      try {
        IEnvironment* prev = cur;
        IEnvironment* next = EnterClientState(7, prev);
        log_case(7, "EnterLoadGameState(guarded)", prev, next);
        if (next) cur = next;
      } catch (...) {
        fl_port::log_message(fl_port::LogLevel::Warning,
                             "M8.7/M8.8 transition probe: case 7 threw (ignored)");
      }

      // Restore menu/intro state (case 6) for controlled bring-up loop stability.
      try {
        IEnvironment* prev = cur;
        IEnvironment* next = EnterClientState(6, prev);
        log_case(6, "EnterMenuState/restore", prev, next);
        if (next) cur = next;
      } catch (...) {
        fl_port::log_message(fl_port::LogLevel::Warning,
                             "M8.7/M8.8 transition probe: case 6 restore threw (ignored)");
      }

      g_client.current_state = cur;
      fl_port::log_message(fl_port::LogLevel::Info,
                           "M8.7/M8.8 transition probe: completed (intro/menu state restored where possible)");
    } catch (...) {
      fl_port::log_message(fl_port::LogLevel::Warning,
                           "M8.7/M8.8 transition probe: threw (ignored)");
    }
  } else {
    fl_port::log_message(fl_port::LogLevel::Warning,
                         "M8.7 bootstrap hook: upstream InitializeEngine reported failure (continuing bring-up fallback)");
  }

  // Partial progress: upstream init TU invoked; full WinMain/state loop integration still pending.
  return (init_ok || exercised_any) ? 1 : 0;
}


