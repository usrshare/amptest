// vim: cin:sts=4:sw=4
#ifndef UI_H
#define UI_H
#include <stdint.h>
#include "wintypes.h"

enum ui_windows {
    W_MAIN = 0,
    W_PLAYLIST,
    W_EQUALIZER,
    W_COUNT
};

extern HWND h_window[];

struct windowCallbacks {
    void (*focuscb)(HWND hWnd, int focused);
    void (*timercb)(HWND hWnd); //timer function
    void (*paintcb)(HWND hWnd); //caused whenever a window wants to paint.
    void (*menucb)(HWND hWnd, int menuid); //if a menu function is called.

    int (*holdcb)(HWND hWnd); // int returns if window can be dragged by this part
    int (*clickcb)(HWND hWnd);
    int (*rightclickcb)(HWND hWnd);
    int (*wheelcb)(HWND hWnd);
    int (*dblclickcb)(HWND hWnd);
};

enum mouse_buttons {
    UIMB_LEFT = 1,
    UIMB_MIDDLE = 2,
    UIMB_RIGHT = 4,
}; //I wanted to call them MB_*, but MB_RIGHT was taken by MessageBox

enum mouse_wheel {
    MW_NONE = 0,
    MW_UP = 1,
    MW_DOWN = 2,
};

struct mouseData {
    HWND hWnd;
    short X; //current position
    short Y;
    short clickX; //last click position
    short clickY;
    short buttons; //buttons held
    short wheel; //wheel status
};

extern struct mouseData mouse;

HBITMAP loadOptSkinBitmap (const char* filename);
HBITMAP loadSkinBitmap (const char* filename);

int skinInitializePaint(HWND hWnd);
int skinBlit(HWND hWnd, HBITMAP src, int xs, int ys, int xd, int yd, int w, int h);
int windowBlit(HWND hWnd);
int skinDestroyPaint(HWND hWnd);
int invalidateXYWH(HWND hWnd, unsigned int x, unsigned int y, unsigned int w, unsigned int h);

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

int uiOpenFile(HWND hWnd, unsigned int types_c, const char** types_v, char* out_file, unsigned int out_sz);
void initConsole(void);

#endif
