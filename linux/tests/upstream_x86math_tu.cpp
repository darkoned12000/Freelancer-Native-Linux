// Linux M3 harness TU: builds portable x86math (3DMathEngine) implementation for
// DACOM registration and numeric validation (isolated from MSVC/x87 asm upstream TU).

#if !defined(_WIN32)
#include "linux_port/windows.h"
#include "linux_port/platform.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"

#endif

#include "../src/x86math_portable.cpp"

#if !defined(_WIN32)
#pragma GCC diagnostic pop
#endif
