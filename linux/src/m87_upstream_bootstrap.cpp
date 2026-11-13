// M8.7 guarded upstream client/library bootstrap integration.
// Wires available upstream initialization entry points/components into the Linux bring-up flow.
// Real src/freelancer/Client/main.cpp and engine_init.cpp bootstrap entry points are not yet
// compiled/linked; this hook performs best-effort upstream component initialization via DACOM
// (SystemContainer/Engine discovery) and reports whether full upstream bootstrap is available.

#include "linux_port/platform.h"

#include <dacom.h>
#include <system.h>

#include <string>

#if defined(__GNUC__) || defined(__clang__)
#define FL_M87_EXPORT __attribute__((visibility("default")))
#else
#define FL_M87_EXPORT
#endif

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
  // TODO(M8.7): when upstream client bootstrap/init TUs (src/freelancer/Client/main.cpp,
  // engine_init.cpp and required deps) are compiled and linked for Linux, invoke wrapped
  // WinMain/InitializeEngine (or equivalent upstream entry points) here.

  ICOManager* dacom = DACOM_Acquire();
  if (!dacom) {
    fl_port::log_message(fl_port::LogLevel::Warning,
                         "M8.7 bootstrap hook: DACOM unavailable; upstream bootstrap/init TUs not linked");
    return 0;
  }

  bool invoked_any_upstream_init = false;

  // Best-effort upstream SystemContainer creation (registered by linked upstream system TU).
  IDAComponent* container = nullptr;
  FPortableSystemContainerAggDesc agg{};
  agg.Size = 0x1c;
  agg.ComponentName = "SystemContainer";
  agg.IniPath = ini_path_or_null;
  const GENRESULT gr_sys = dacom->CreateInstance(reinterpret_cast<AGGDESC*>(&agg),
                                                 reinterpret_cast<void**>(&container));
  if (gr_sys == GR_OK && container) {
    invoked_any_upstream_init = true;
    fl_port::log_message(fl_port::LogLevel::Info,
                         "M8.7 bootstrap hook: upstream SystemContainer created (best-effort)");

    // Best-effort interface discovery to exercise upstream component wiring.
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
    qi_log("IAnimation2", "IAnimation2");
    qi_log("IChannel", "IChannel");
    qi_log("IDeformable", "IDeformable");

    container->Release();
  } else {
    fl_port::log_message(fl_port::LogLevel::Warning,
                         std::string("M8.7 bootstrap hook: SystemContainer unavailable gr=") +
                         std::to_string((int)gr_sys));
  }

  if (invoked_any_upstream_init) {
    fl_port::log_message(fl_port::LogLevel::Info,
                         "M8.7 bootstrap hook: partial upstream init components exercised; full client bootstrap/init TUs still unavailable");
  } else {
    fl_port::log_message(fl_port::LogLevel::Info,
                         "M8.7 bootstrap hook: upstream init TUs unavailable (scaffold fallback)");
  }

  // Return 0 until real src/freelancer Client bootstrap/init entry points replace bring-up flow.
  return 0;
}
