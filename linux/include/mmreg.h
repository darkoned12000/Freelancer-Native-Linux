#pragma once
#include "linux_port/win32_compat.h"
// Minimal mmreg.h shim for Linux bring-up.
typedef struct tWAVEFORMATEX {
  WORD  wFormatTag;
  WORD  nChannels;
  DWORD nSamplesPerSec;
  DWORD nAvgBytesPerSec;
  WORD  nBlockAlign;
  WORD  wBitsPerSample;
  WORD  cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX, *LPWAVEFORMATEX;
typedef const WAVEFORMATEX* LPCWAVEFORMATEX;
#define WAVE_FORMAT_PCM 1
#define WAVE_FORMAT_IEEE_FLOAT 3
#define WAVE_FORMAT_EXTENSIBLE 0xFFFE