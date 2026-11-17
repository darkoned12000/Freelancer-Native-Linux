#pragma once
#include <cstdarg>
#include <cstdlib>
#define _P_WAIT 0
#define _P_NOWAIT 1
inline int _spawnl(int mode, const char* path, const char* arg0, ...) { (void)mode;(void)path;(void)arg0; return -1; }
inline int _spawnvp(int mode, const char* path, const char* const* argv) { (void)mode;(void)path;(void)argv; return -1; }