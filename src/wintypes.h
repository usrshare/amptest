#ifndef WINTYPES_H
#define WINTYPES_H

#ifndef _WINDOWS_

//all the code in main.c, wa_plugins.c and other files that wants to tell windows apart,
//but doesn't really do anything with them, WHND would be treated as an intptr, which,
//according to MSDN, it pretty much is.

typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HBITMAP;
typedef char* LPSTR;
#endif

#endif
