#ifndef WINTYPES_H
#define WINTYPES_H

#ifndef _WINDOWS_

//these typedefs are used for files that don't use any of the WinAPI functions, but still have
//to pass around types used by it. They should have identical length on both 32- and 64-bit
//versions of windows.

typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HBITMAP;
typedef char* LPSTR;
#endif

#endif
