// M8.7 guarded upstream client/library bootstrap integration.
// Progresses toward upstream client init/bootstrap by invoking linked upstream
// InitializeEngine (with temporary single-player-safe stubs/fallbacks) when available.

#include "linux_port/platform.h"

#include <dacom.h>
#include <system.h>

#include <cstring>
#include <string>

struct FClientCommandLine;

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

struct FClientCommandLineShim {
  unsigned Count = 0;
  char Buffer[1024] = {};
  char* Argv[64] = {};
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

  // Invoke linked upstream InitializeEngine in a guarded single-player-safe mode.
  // Many dependencies are currently stubbed; treat failure as non-fatal for bring-up fallback.
  fl_port::log_message(fl_port::LogLevel::Info,
                       "M8.7 bootstrap hook: invoking upstream InitializeEngine (guarded)");
  bool init_ok = false;
  try {
    FClientCommandLineShim cmd{};
    init_ok = InitializeEngine(reinterpret_cast<const FClientCommandLine*>(&cmd));
  } catch (...) {
    fl_port::log_message(fl_port::LogLevel::Error,
                         "M8.7 bootstrap hook: upstream InitializeEngine threw (caught)");
    return exercised_any ? 1 : 0;
  }

  if (init_ok) {
    fl_port::log_message(fl_port::LogLevel::Info,
                         "M8.7 bootstrap hook: upstream InitializeEngine reported success");
  } else {
    fl_port::log_message(fl_port::LogLevel::Warning,
                         "M8.7 bootstrap hook: upstream InitializeEngine reported failure (continuing bring-up fallback)");
  }

  // Partial progress: upstream init TU invoked; full WinMain/state loop integration still pending.
  return (init_ok || exercised_any) ? 1 : 0;
}
