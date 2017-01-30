#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include "wa_plugins.h"
#include "win_misc.h"

struct waInputPlugin* ip = NULL;
struct waOutputPlugin* op = NULL;

struct skinData {
	HBITMAP mainbitmap;
	HBITMAP titlebitmap;
};

struct skinData skin;

HWND h_mainwin;

struct paintData {
	PAINTSTRUCT ps;
	HDC hdc;
	BITMAP bitmap;
	HDC hdcMem;
	HGDIOBJ oldBitmap;
} spaint;

int skinStartPaint(HWND hWnd) {
	spaint.hdc = BeginPaint(hWnd, &(spaint.ps));
	spaint.hdcMem = CreateCompatibleDC(spaint.hdc);
	return 0;
}

int skinEndPaint(HWND hWnd) {
	DeleteDC(spaint.hdcMem);
	spaint.hdcMem = 0;
	EndPaint(hWnd, &(spaint.ps));
	return 0;
}

int skinBlit(HWND hWnd, HBITMAP hbm, int xs, int ys, int xd, int yd, int w, int h) {

	GetObject(hbm, sizeof(spaint.bitmap), &(spaint.bitmap));
	spaint.oldBitmap = SelectObject(spaint.hdcMem, hbm);
	BitBlt(spaint.hdc, xd, yd, w ? w : spaint.bitmap.bmWidth, h ? h : spaint.bitmap.bmHeight, spaint.hdcMem, xs, ys, SRCCOPY);
	SelectObject(spaint.hdcMem, spaint.oldBitmap);
	return 0;
}

CONST RECT mainTitleRect = {.left = 0, .top = 0, .right = 275, .bottom = 14};

short mouseClickX = -1;
short mouseClickY = -1;
short mouseReleaseX = -1;
short mouseReleaseY = -1;
short mouseX = -1;
short mouseY = -1;
short mouseButtons = 0;

int hover(short x, short y, short w, short h) {

	if ((mouseX >= x) && (mouseX < (x+w)) &&
			(mouseY >= y) && (mouseY < (y+h))) return 1; else return 0;
}

int hold(short x, short y, short w, short h, short bmask) {

	if ( (mouseClickX >= x) && (mouseClickX < (x+w)) &&
			(mouseClickY >= y) && (mouseClickY < (y+h)) &&
			(mouseX >= x) && (mouseX < (x+w)) &&
			(mouseY >= y) && (mouseY < (y+h)) &&
			(mouseButtons & bmask)) {

		return 1;
	}
	return 0;
}

int click(short x, short y, short w, short h, short bmask) {

	if ( (mouseClickX >= x) && (mouseClickX < (x+w)) &&
			(mouseClickY >= y) && (mouseClickY < (y+h)) &&
			(mouseReleaseX >= x) && (mouseReleaseX < (x+w)) &&
			(mouseReleaseY >= y) && (mouseReleaseY < (y+h)) && 
			(mouseButtons & bmask) ) {

		mouseClickX = -1; mouseClickY = -1;
		mouseReleaseX = -1; mouseReleaseY = -1;
		return 1;
	}
	return 0;
}

int invalidateXYWH(HWND hWnd, UINT x, UINT y, UINT w, UINT h) {
	CONST RECT r = {.top = y, .left = x, .bottom = y+h, .right = x+w};
	return InvalidateRect(hWnd,&r,0);
}

struct element {

	unsigned int x,y,w,h;
	int obs,bs;	
};

enum windowelements {
	WE_TITLEBAR,
	WE_TIMER,
	WE_TITLE,
	WE_BITRATE, //kbps
	WE_MIXRATE, //kHz
	WE_MONOSTER,
	WE_VISUAL,
	WE_COUNT
};

enum windowbuttons {
	WB_MENU,
	WB_MINIMIZE,
	WB_WINDOWSHADE,
	WB_CLOSE,
	WB_OAIDV,
	WB_VOLUME,
	WB_BALANCE,
	WB_EQ,
	WB_PL,
	WB_SCROLLBAR,
	WB_PREV,
	WB_PLAY,
	WB_PAUSE,
	WB_STOP,
	WB_NEXT,
	WB_OPEN,
	WB_SHUFFLE,
	WB_REPEAT,
	WB_COUNT,
};

struct element mw_elements[WE_COUNT] = {
	{  .x = 0,   .y = 0, .w = 275,  .h = 14}, //titlebar

};

struct element mw_buttons[WB_COUNT] = {
	{  .x = 3,   .y = 3, .w = 9,  .h = 9}, //menu
	{  .x = 244, .y = 3, .w = 9,  .h = 9}, //minimize
	{  .x = 254, .y = 3, .w = 9,  .h = 9}, //windowshade
	{  .x = 264, .y = 3, .w = 9,  .h = 9}, //close
};


int handleHoldEvents(HWND hWnd) {

	for (int i=0; i < WB_COUNT; i++) {
		struct element* e = &mw_buttons[i];
		e->bs = hold(e->x,e->y,e->w,e->h,1);
		if (e->bs != e->obs) invalidateXYWH(h_mainwin,e->x,e->y,e->w,e->h);
	}
}

int handleClickEvents(HWND hWnd) {

	if (click(264,3,9,9,1)) {
		ExitProcess(0);
	}
}

int find_element_to_update(unsigned int element_c, struct element* element_v, unsigned int element_i) {

	for (int i = element_i; i < element_c; i++) {
		struct element* e = &element_v[i];
		if (e->bs != e->obs) { e->obs = e->bs; return i; }
	}
	return -1;

}

LRESULT WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch(uMsg) {
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			mw_elements[WE_TITLEBAR].bs = (uMsg == WM_SETFOCUS);
			if (mw_elements[WE_TITLEBAR].bs != mw_elements[WE_TITLEBAR].obs) InvalidateRect(hWnd,&mainTitleRect,0);
			return DefWindowProc(hWnd,uMsg,wParam,lParam);
			break;
		case WM_CREATE:
			return 0;
			break;
		case WM_COMMAND:
			if (LOWORD(wParam) == 1) ip->Play("demo.mp3");
			return 0;
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
			break;
		case WM_LBUTTONDOWN:
			//record press location for "pushed down" buttons.
			mouseClickX = LOWORD(lParam);
			mouseClickY = HIWORD(lParam);
			mouseButtons |= 1;
			handleHoldEvents(hWnd);
			break;
		case WM_LBUTTONUP:
			mouseReleaseX = LOWORD(lParam);
			mouseReleaseY = HIWORD(lParam);
			//handle all the "click" events here.
			handleClickEvents(hWnd);
			mouseButtons &= (~1);
			handleHoldEvents(hWnd);
			break;
		case WM_MOUSEMOVE:
			mouseX = LOWORD(lParam);
			mouseY = HIWORD(lParam);
			handleHoldEvents(hWnd);
			break;
		case WM_PAINT: 
			skinStartPaint(hWnd);
			skinBlit(hWnd, skin.mainbitmap, 0, 0, 0, 0, 0, 0);

			int i=0;
			while (( i = find_element_to_update(WE_COUNT,mw_elements,i)) != -1) {
				switch (i) {
					case WE_TITLEBAR:
						if (GetForegroundWindow() == hWnd) {
							skinBlit(hWnd, skin.titlebitmap, 27, 0, 0, 0, 275, 14);
						} else {
							skinBlit(hWnd, skin.titlebitmap, 27, 15, 0, 0, 275, 14);
						}
						break;
				}
			}
			i=0;
			while (( i = find_element_to_update(WB_COUNT,mw_buttons,i)) != -1) {
				switch (i) {
					case WB_MINIMIZE:
						if (mw_buttons[WB_MINIMIZE].bs) {
							skinBlit(hWnd, skin.titlebitmap,9,9,244,3,9,9);
						} else {
							skinBlit(hWnd, skin.titlebitmap,9,0,244,3,9,9);
						}
						break;
					case WB_WINDOWSHADE:
						if (mw_buttons[WB_WINDOWSHADE].bs) {
							skinBlit(hWnd, skin.titlebitmap,9,18,254,3,9,9);
						} else {
							skinBlit(hWnd, skin.titlebitmap,0,18,254,3,9,9);
						}
						break;
					case WB_CLOSE:
						if (mw_buttons[WB_CLOSE].bs) {
							skinBlit(hWnd, skin.titlebitmap,18,9,264,3,9,9);
						} else {
							skinBlit(hWnd, skin.titlebitmap,18,0,264,3,9,9);
						}
						break;
				}
			}
			skinEndPaint(hWnd);
			break;
		default:
			return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
}

WNDCLASS mainwin = {0,(WNDPROC)WindowProc,0,0,NULL,NULL,NULL,COLOR_BTNFACE+1,NULL,"helloMain"};

int ampInit() {

	op = loadOutputPlugin("plugins/out_wave.dll");
	if (!op) { return 1; }

	ip = loadInputPlugin("plugins/in_mp3.dll",op);
	if (!ip) { return 1; }

	ip->hMainWindow = h_mainwin;
	op->hMainWindow = h_mainwin;
	ip->Init();
	op->Init();

	return 0;
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

	mainwin.hInstance = GetModuleHandle(0);
	mainwin.hCursor = LoadCursor(0,IDC_ARROW);

	RegisterClass(&mainwin);
	InitCommonControls();

	h_mainwin = CreateWindowEx(0, "helloMain", "hello world", WS_VISIBLE | WS_POPUP, 128, 128, 275, 116, NULL, NULL, mainwin.hInstance, NULL);

	skin.mainbitmap = LoadImage(NULL, "skin/main.bmp", IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
	if (!skin.mainbitmap) msgerror("load bitmap");

	skin.titlebitmap = LoadImage(NULL, "skin/titlebar.bmp", IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
	if (!skin.titlebitmap) msgerror("load titlebar bitmap");

	CreateWindow("BUTTON","Play",WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 16,96,24,16, h_mainwin, (HMENU)1, mainwin.hInstance, NULL);


	ampInit();

	MSG lastmsg;
	while (GetMessage(&lastmsg,NULL,0,0)) {
		TranslateMessage(&lastmsg);
		DispatchMessage(&lastmsg);
	}


	MessageBoxA(0, "hello world", "test", 0);
	return 0;
}
