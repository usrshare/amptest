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

enum text_align {
    UITA_LEFT,
    UITA_CENTER,
    UITA_RIGHT
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

int normalizeRect (HWND hWnd, short* x, short* y, short* w, short* h);

int skinInitializePaint(HWND hWnd);
int skinBlit(HWND hWnd, HBITMAP src, short xs, short ys, short xd, short yd, short w, short h);
int skinRect(HWND hWnd, short x, short y, short w, short h, unsigned int color);
int windowBlit(HWND hWnd);
int skinDestroyPaint(HWND hWnd);
int invalidateXYWH(HWND hWnd, short x, short y, short w, short h);
int uiDrawText(HWND hWnd, const char* text, short x, short y, short w, short h, unsigned int bgcolor, unsigned int fgcolor, enum text_align align, int* line_height);

int showSystemMenu(HWND hWnd, int submenu, short x, short y);

int initUI(void);

int createWindows(struct windowCallbacks* wincb);
int windowLoop(void);

enum uiMessageBoxType {
    UIMB_INFO = 0,
    UIMB_WARNING,
    UIMB_ERROR,
    UIMB_COUNT
};

int getWindowSize(HWND hWnd, unsigned int* w, unsigned int* h);

int windowVisibility(HWND hWnd, int show);

int changeWindowTitle(HWND hWnd, const char* title);

int uiOKMessageBox(HWND parenthWnd, const char* text, const char* title, int type);
int uiInputBox(HWND hWnd, const char* prompt, char* rstring, size_t rstrSz);

int uiOpenFile(HWND hWnd, unsigned int types_c, const char** types_v, char* out_file, unsigned int out_sz);
int uiOpenFiles(HWND hWnd, unsigned int types_c, const char** types_v, char* out_file, unsigned int out_sz); //multiple files
void initConsole(void);

#endif
