#pragma once
#ifndef CALLBACK
#define CALLBACK
#endif
#ifndef LPBYTE
#define LPBYTE unsigned char*
#endif
#ifndef ACM_FORMATSUGGESTF_WFORMATTAG
#define ACM_FORMATSUGGESTF_WFORMATTAG 0x00010000
#define ACM_STREAMOPENF_NONREALTIME 0x00000200
#define ACM_STREAMSIZEF_SOURCE 0x00000000
#define ACM_STREAMCONVERTF_END 0x00000020
#endif
#include "linux_port/win32_compat.h"
#include "mmreg.h"
// Minimal ACM (audio compression manager) shim for Linux bring-up.
typedef void* HACMDRIVER;
typedef void* HACMDRIVERID;
typedef void* HACMSTREAM;
typedef struct {
  DWORD cbStruct;
  DWORD fdwStatus;
  DWORD_PTR dwUser;
  LPBYTE pbSrc;
  DWORD cbSrcLength;
  DWORD cbSrcLengthUsed;
  DWORD dwSrcUser;
  LPBYTE pbDst;
  DWORD cbDstLength;
  DWORD cbDstLengthUsed;
  DWORD dwDstUser;
  DWORD dwReservedDriver;
} ACMSTREAMHEADER, *PACMSTREAMHEADER, *LPACMSTREAMHEADER;
typedef struct {
  DWORD cbStruct;
  FOURCC fccType;
  FOURCC fccComp;
  WORD  wMid;
  WORD  wPid;
  DWORD vdwACM;
  DWORD vdwDriver;
  DWORD fdwSupport;
  DWORD cFormatTags;
  DWORD cFilterTags;
  HICON hicon;
  char  szShortName[32];
  char  szLongName[128];
  char  szCopyright[80];
  char  szLicensing[120];
  char  szFeatures[512];
} ACMDRIVERDETAILSA, *PACMDRIVERDETAILSA, *LPACMDRIVERDETAILSA;
typedef ACMDRIVERDETAILSA ACMDRIVERDETAILS;
typedef struct { DWORD cbStruct; } ACMFORMATCHOOSE;
typedef struct { DWORD cbStruct; } ACMDRVDETAILS;
#define ACM_FORMATSUGGESTF_WFORMATTAG 0x00010000
inline int acmFormatSuggest(HACMDRIVER had, WAVEFORMATEX* pwfxSrc, WAVEFORMATEX* pwfxDst, DWORD cbwfxDst, DWORD fdwSuggest) { (void)had;(void)pwfxSrc;(void)pwfxDst;(void)cbwfxDst;(void)fdwSuggest; return -1; }
inline int acmStreamOpen(HACMSTREAM* phas, HACMDRIVER had, WAVEFORMATEX* pwfxSrc, WAVEFORMATEX* pwfxDst, void* pwfltr, DWORD cbwfltr, DWORD dwCallback, DWORD fdwOpen) { (void)phas;(void)had;(void)pwfxSrc;(void)pwfxDst;(void)pwfltr;(void)cbwfltr;(void)dwCallback;(void)fdwOpen; if (phas) *phas=nullptr; return -1; }
inline int acmStreamClose(HACMSTREAM has, DWORD fdwClose) { (void)has;(void)fdwClose; return 0; }
inline int acmStreamConvert(HACMSTREAM has, void* phdr, DWORD fdwConvert) { (void)has;(void)phdr;(void)fdwConvert; return -1; }
inline int acmStreamSize(HACMSTREAM has, DWORD cbSrc, DWORD* pcbDst, DWORD fdwSize) { (void)has;(void)cbSrc;(void)pcbDst;(void)fdwSize; if (pcbDst) *pcbDst=cbSrc; return 0; }
inline int acmStreamPrepareHeader(HACMSTREAM has, ACMSTREAMHEADER* phdr, DWORD fdwPrepare) { (void)has;(void)phdr;(void)fdwPrepare; return -1; }
inline int acmStreamUnprepareHeader(HACMSTREAM has, ACMSTREAMHEADER* phdr, DWORD fdwUnprepare) { (void)has;(void)phdr;(void)fdwUnprepare; return 0; }

typedef int (CALLBACK* ACMDRIVERENUMCB)(HACMDRIVERID, DWORD, DWORD);
inline int acmDriverEnum(ACMDRIVERENUMCB fnCallback, DWORD dwInstance, DWORD fdwEnum) { (void)fnCallback;(void)dwInstance;(void)fdwEnum; return 0; }
inline int acmDriverOpen(HACMDRIVER* phad, HACMDRIVERID hadid, DWORD fdwOpen) { (void)phad;(void)hadid;(void)fdwOpen; if (phad) *phad=nullptr; return -1; }
inline int acmDriverDetailsA(HACMDRIVERID hadid, LPACMDRIVERDETAILSA padd, DWORD fdwDetails) { (void)hadid;(void)padd;(void)fdwDetails; return -1; }