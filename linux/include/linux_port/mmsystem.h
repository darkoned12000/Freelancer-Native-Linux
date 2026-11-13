#pragma once
// Minimal mmsystem.h compatibility shim for Linux port probes/builds.
#include "linux_port/win32_compat.h"

#ifndef MMRESULT
typedef UINT MMRESULT;
#endif
#ifndef MMSYSERR_NOERROR
#define MMSYSERR_NOERROR 0
#endif
#ifndef TIME_ONESHOT
#define TIME_ONESHOT 0x0000
#endif
#ifndef TIME_PERIODIC
#define TIME_PERIODIC 0x0001
#endif
#ifndef TIMERR_NOERROR
#define TIMERR_NOERROR 0
#endif

typedef struct timecaps_tag {
  UINT wPeriodMin;
  UINT wPeriodMax;
} TIMECAPS, *PTIMECAPS, *LPTIMECAPS;

inline MMRESULT timeGetDevCaps(LPTIMECAPS ptc, UINT cbtc) {
  if (ptc && cbtc >= sizeof(TIMECAPS)) { ptc->wPeriodMin = 1; ptc->wPeriodMax = 1000000; }
  return TIMERR_NOERROR;
}
inline MMRESULT timeBeginPeriod(UINT) { return TIMERR_NOERROR; }
inline MMRESULT timeEndPeriod(UINT) { return TIMERR_NOERROR; }

#ifndef PlaySound
#define PlaySound PlaySoundA
#endif
inline BOOL PlaySoundA(LPCSTR, HMODULE, DWORD) { return FALSE; }
inline BOOL PlaySoundW(const wchar_t*, HMODULE, DWORD) { return FALSE; }

#ifndef SND_ASYNC
#define SND_ASYNC 0x0001
#define SND_NODEFAULT 0x0002
#define SND_MEMORY 0x0004
#define SND_LOOP 0x0008
#define SND_NOSTOP 0x0010
#define SND_PURGE 0x0040
#define SND_FILENAME 0x00020000L
#define SND_RESOURCE 0x00040004L
#endif
