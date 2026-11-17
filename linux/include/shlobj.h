#ifndef _SHLOBJ_H
#define _SHLOBJ_H
#include "linux_port/win32_compat.h"
#define CSIDL_FLAG_CREATE 0x8000
#define CSIDL_LOCAL_APPDATA 0x001c
#define CSIDL_PERSONAL 0x0005
#define CSIDL_MYPICTURES 0x0027
inline int SHGetFolderPathA(void*, int, void*, int, char* dest) {
  if (dest) dest[0] = '\0';
  return 0;
}
#define _strdup strdup
#define _wcsdup wcsdup
struct _FILETIME : FILETIME {};
struct _SYSTEMTIME {
  unsigned short wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
};
typedef _SYSTEMTIME SYSTEMTIME;
#define MsgWaitForMultipleObjects(a,b,c,d,e) (0)
inline int FileTimeToSystemTime(const FILETIME*, SYSTEMTIME* t){ if(t) memset(t,0,sizeof(*t)); return 1; }
inline int GetTimeFormatA(unsigned, unsigned, const SYSTEMTIME*, const void*, char* buf, int){ if(buf) buf[0]=0; return 1; }
inline int GetDateFormatA(unsigned, unsigned, const SYSTEMTIME*, const void*, char* buf, int){ if(buf) buf[0]=0; return 1; }
#endif
