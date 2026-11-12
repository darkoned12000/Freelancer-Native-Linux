#pragma once
// Windows.h facade for Linux bring-up harness/port TUs.
// Prefer including specific linux_port/win32_*.h headers where possible.
// This aggregator exists to ease guarded compilation of upstream code that expects <windows.h>.

#include "linux_port/win32_compat.h"
#include "linux_port/win32_debug.h"
#include "linux_port/win32_file.h"
#include "linux_port/win32_registry.h"
#include "linux_port/win32_string.h"
#include "linux_port/win32_sync.h"
#include "linux_port/win32_threads.h"
#include "linux_port/win32_time.h"
#include "linux_port/win32_user.h"
#include "linux_port/msvcrt_path.h"
