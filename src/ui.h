#ifndef UI_H
#define UI_H
#include <stdint.h>

#ifndef WHND
//all the code in main.c, wa_plugins.c and other files that wants to tell windows apart,
//but doesn't really do anything with them, WHND would be treated as an intptr, which,
//according to MSDN, it pretty much is.
typedef intptr_t WHND;
#endif

extern HWND h_mainwin;

struct windowCallbacks {
    void (*focuscb)(HWND hWnd, int focused);
    void (*timercb)(HWND hWnd); //timer function
    void (*paintcb)(HWND hWnd); //caused whenever a window wants to paint.
    void (*menucb)(HWND hWnd, int menuid); //if a menu function is called.

    int (*holdcb)(HWND hWnd); // int returns if window can be dragged by this part
    int (*clickcb)(HWND hWnd);
    int (*dblclickcb)(HWND hWnd);
};

struct mouseData {
    HWND hWnd;
    short X; //current position
    short Y;
    short clickX; //last click position
    short clickY;
    short buttons; //buttons held
};

extern struct mouseData mouse;

enum window_types {
    WT_MAIN = 0,
    WT_EQUALIZER,
    WT_PLAYLIST
};

int skinInitializePaint(HWND hWnd);
int skinBlit(HWND hWnd, HBITMAP src, int xs, int ys, int xd, int yd, int w, int h);
int windowBlit(HWND hWnd);
int skinDestroyPaint(HWND hWnd);

int showSystemMenu(HWND hWnd, int submenu, int x, int y);

int initMainMenu (void);
int initMainWindow(void);
int createMainWindow(struct windowCallbacks* wincb);
int windowLoop(void);

enum uiMessageBoxType {
	UIMB_INFO = 0,
	UIMB_WARNING,
	UIMB_ERROR,
	UIMB_COUNT
};

int uiOKMessageBox(HWND parenthWnd, const char* text, const char* title, int type);
int uiInputBox(HWND hWnd, const char* prompt, char* rstring, size_t rstrSz);

#endif
