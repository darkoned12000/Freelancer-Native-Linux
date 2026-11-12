#pragma once

// Minimal Linux bring-up shim for DirectX 8 types needed by render/IRenderPipeline.h.
// This is a compile-time type provider only; no DirectX runtime is used (M5 uses Vulkan).
// Layouts match the DirectX 8.1 SDK structures (verified by static_asserts in d3d8_min.h).

#include "windows.h"

#include <cstddef>

struct D3DVECTOR { float x, y, z; };
struct D3DCOLORVALUE { float r, g, b, a; };

typedef enum _D3DLIGHTTYPE {
  D3DLIGHT_POINT = 1,
  D3DLIGHT_SPOT = 2,
  D3DLIGHT_DIRECTIONAL = 3,
} D3DLIGHTTYPE;

struct _D3DLIGHT8 {
  D3DLIGHTTYPE Type;
  D3DCOLORVALUE Diffuse;
  D3DCOLORVALUE Specular;
  D3DCOLORVALUE Ambient;
  D3DVECTOR Position;
  D3DVECTOR Direction;
  float Range;
  float Falloff;
  float Attenuation0;
  float Attenuation1;
  float Attenuation2;
  float Theta;
  float Phi;
};
typedef struct _D3DLIGHT8 D3DLIGHT8;

struct _D3DMATERIAL8 {
  D3DCOLORVALUE Diffuse;
  D3DCOLORVALUE Ambient;
  D3DCOLORVALUE Specular;
  D3DCOLORVALUE Emissive;
  float Power;
};
typedef struct _D3DMATERIAL8 D3DMATERIAL8;

// D3DRENDERSTATETYPE is a dword enum in D3D8.
typedef int D3DRENDERSTATETYPE;

// D3DCOLOR: packed 32-bit color (ARGB).
typedef unsigned int D3DCOLOR;

// D3DVIEWPORT8: viewport rectangle + depth range.
struct D3DVIEWPORT8 {
  unsigned int X;
  unsigned int Y;
  unsigned int Width;
  unsigned int Height;
  float MinZ;
  float MaxZ;
};
typedef struct D3DVIEWPORT8 D3DVIEWPORT8;

static_assert(sizeof(D3DVIEWPORT8) == 0x18, "D3DVIEWPORT8 size");

// Minimal types needed by render/d3d8_min.h static_asserts (compile-time only).
typedef struct { char _[0x34]; } D3DPRESENT_PARAMETERS;

struct D3DCAPS8 {
  char _p0[0x1C];
  unsigned int DevCaps;
  char _p1[0x24 - 0x20];
  unsigned int RasterCaps;
  char _p2[0x3C - 0x28];
  unsigned int TextureCaps;
  char _p3[0x58 - 0x40];
  unsigned int MaxTextureWidth;
  unsigned int MaxTextureHeight;
  char _p4[0xD4 - 0x60];
};
typedef struct IDirect3D8 IDirect3D8;

static_assert(sizeof(D3DLIGHT8) == 0x68, "D3DLIGHT8 size");
static_assert(sizeof(D3DMATERIAL8) == 0x44, "D3DMATERIAL8 size");
static_assert(sizeof(D3DPRESENT_PARAMETERS) == 0x34, "D3DPRESENT_PARAMETERS");
static_assert(sizeof(D3DCAPS8) == 0xD4, "D3DCAPS8 DX8");