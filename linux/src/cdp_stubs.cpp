// Linux single-player stubs for CDPClient/CDPServer static methods.
#include "dalib_net.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <time.h>
static inline unsigned long fl_cdp_stub_time_ms() {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
  return static_cast<unsigned long>(static_cast<unsigned long>(ts.tv_sec) * 1000ul +
                                    static_cast<unsigned long>(ts.tv_nsec) / 1000000ul);
}
#define timeGetTime fl_cdp_stub_time_ms
#endif

// Return current time in offline/single-player bring-up to avoid timeout false positives.
unsigned long CDPClient::GetLastMsgTimestamp() { return timeGetTime(); }
unsigned long CDPClient::GetPingDelay() { return 0; }
unsigned long CDPClient::GetSendQueueSize() { return 0; }
bool CDPClient::IsPingOutstanding() { return false; }
void CDPClient::SetSourcePort(unsigned long) {}
unsigned long CDPClient::GetLinkQuality() { return 0xFF; }

unsigned long CDPServer::GetLastMsgTimestamp() { return timeGetTime(); }
void CDPServer::CrashCleanup() {}
