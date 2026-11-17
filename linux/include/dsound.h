#pragma once
#include "linux_port/win32_compat.h"
// Minimal DirectSound shim for Linux bring-up (compile support).
typedef struct IDirectSound IDirectSound;
typedef struct IDirectSound IDirectSound;
typedef struct IDirectSound8 IDirectSound8;
typedef struct IDirectSoundBuffer IDirectSoundBuffer;
typedef struct IDirectSoundBuffer8 IDirectSoundBuffer8;
typedef struct IDirectSound3DBuffer IDirectSound3DBuffer;
typedef struct IDirectSound3DBuffer8 IDirectSound3DBuffer8;
typedef struct IDirectSound3DListener IDirectSound3DListener;
typedef IDirectSound8* LPDIRECTSOUND8;
typedef IDirectSoundBuffer8* LPDIRECTSOUNDBUFFER8;
typedef IDirectSound3DBuffer8* LPDIRECTSOUND3DBUFFER8;
#define DS_OK 0
struct DSCAPS { DWORD dwSize; };
struct DSBCAPS { DWORD dwSize; };
struct DSBUFFERDESC { DWORD dwSize; };
struct _DS3DBUFFER { DWORD dwSize; };
typedef struct _DS3DBUFFER DS3DBUFFER;