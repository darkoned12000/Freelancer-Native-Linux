// Linux single-player stubs for CDPClient/CDPServer static methods.
#include "dalib_net.h"

unsigned long CDPClient::GetLastMsgTimestamp() { return 0; }
unsigned long CDPClient::GetPingDelay() { return 0; }
unsigned long CDPClient::GetSendQueueSize() { return 0; }
bool CDPClient::IsPingOutstanding() { return false; }
void CDPClient::SetSourcePort(unsigned long) {}
unsigned long CDPClient::GetLinkQuality() { return 0xFF; }

unsigned long CDPServer::GetLastMsgTimestamp() { return 0; }
void CDPServer::CrashCleanup() {}
