// Freelancer Decomp: a reimplementation of the Freelancer (2003) game client and engine,
// faithful to the behaviour of the original game.
// This file was reconstructed by reverse engineering the binaries of Freelancer 1.1,
// developed by Digital Anvil.
// Author: Giperion.

// Isolated TU: import DACOM's stricmp without the CRT <string.h> declaration of the POSIX name.
// Retail Common.dll IAT: DACOM.DLL!stricmp (PTR 0639902c), called by the StringCache intern tree
// (find 06333ea0, insert 06333f30, node insert 063341a0) and many other Common lookups.

#if defined(_MSC_VER) || defined(_WIN32)
extern "C" __declspec(dllimport) int __cdecl stricmp(const char* Left, const char* Right);
#else
#include <strings.h>
static inline int fl_port_stricmp(const char* Left, const char* Right)
{
    return strcasecmp(Left ? Left : "", Right ? Right : "");
}
#define stricmp fl_port_stricmp
#endif

#if !defined(_MSC_VER) && !defined(_WIN32)
__attribute__((weak))
#endif
int CallDacomStricmp(const char* Left, const char* Right)
{
    return stricmp(Left, Right);
}
