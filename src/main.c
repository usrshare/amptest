#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include "wa_plugins.h"
#include "win_misc.h"

const char* default_text_layout = 
	"Aa Bb Cc Dd Ee Ff Gg Hh Ii Jj Kk Ll Mm Nn Oo Pp Qq Rr Ss Tt Uu Vv Ww Xx Yy Zz \" @\n"
	"0 1 2 3 4 5 6 7 8 9 … . : ( ) - ! _ + \\ / [ ] ^ & % . = $ #\n"
	"Åå Öö Ää ? *";

struct waInputPlugin* ip = NULL;
struct waOutputPlugin* op = NULL;

struct skinData {
	HBITMAP mainbitmap;
	HBITMAP titlebitmap;
	HBITMAP cbuttons;
	HBITMAP posbar;
	HBITMAP text;
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

unsigned char can_drag = 0;

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
	int value;
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
	{  .x = 6,   .y = 3, .w = 9,  .h = 9}, //menu
	{  .x = 244, .y = 3, .w = 9,  .h = 9}, //minimize
	{  .x = 254, .y = 3, .w = 9,  .h = 9}, //windowshade
	{  .x = 264, .y = 3, .w = 9,  .h = 9}, //close
	{  .x = 11, .y = 22, .w = 10, .h = 43}, //OAIDV -- larger than the assoc. images
	{  .x = 999, .y = 999, .w = 0, .h = 0}, //volume
	{  .x = 999, .y = 999, .w = 0, .h = 0}, //balance
	{  .x = 999, .y = 999, .w = 0, .h = 0}, //equalizer btn
	{  .x = 999, .y = 999, .w = 0, .h = 0}, //playlist btn
	{  .x = 16, .y = 72, .w = 248, .h = 10}, //scroll bar
	{  .x = 16, .y = 88, .w = 23, .h = 18}, //prev
	{  .x = 39, .y = 88, .w = 23, .h = 18}, //play
	{  .x = 62, .y = 88, .w = 23, .h = 18}, //pause
	{  .x = 85, .y = 88, .w = 23, .h = 18}, //stop
	{  .x = 108, .y = 88, .w = 22, .h = 18}, //next

};

int handleHoldEvents(HWND hWnd) {

	can_drag = 1;

	for (int i=0; i < WB_COUNT; i++) {
		struct element* e = &mw_buttons[i];
		if (hover(e->x, e->y, e->w, e->h)) can_drag = 0;
		e->bs = hold(e->x,e->y,e->w,e->h,1);
		if (e->bs != e->obs) invalidateXYWH(h_mainwin,e->x,e->y,e->w,e->h);
	}
}

int get_click_button() {
	for (int i=0; i < WB_COUNT; i++) {
		struct element* cb = &mw_buttons[i];
		if (click(cb->x,cb->y,cb->w,cb->h,1)) return i;
	}
	return -1;
}

int handleClickEvents(HWND hWnd) {

	switch (get_click_button()) {
		case WB_CLOSE:
			ExitProcess(0);
			break;
		case WB_PLAY:
			if (op->IsPlaying()) ip->Stop();
			ip->Play("demo.mp3");
			break;
		case WB_PAUSE:
			if (ip->IsPaused())
				ip->UnPause();
			else
				ip->Pause();
			break;
		case WB_STOP:
			if (op->IsPlaying()) ip->Stop();
			break;
	}
}

int find_element_to_update(unsigned int element_c, struct element* element_v, struct element** cur) {

	while ((*cur) < (element_v + element_c)) {
		if ((*cur)->bs != (*cur)->obs) { (*cur)->obs = (*cur)->bs; return (*cur)->bs; }
		(*cur)++;
	}
	*cur = NULL;
	return 0;

}

#ifndef GET_X_PARAM
int GET_X_PARAM(LPARAM lParam) { return (int)(lParam & 0xFFFF); }
int GET_Y_PARAM(LPARAM lParam) { return (int)(lParam >> 16); }
#endif

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
			//if (LOWORD(wParam) == 1) ip->Play("demo.mp3");
			return 0;
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
			break;
		case WM_TIMER: 
			if (op->IsPlaying()) {
				int len = ip->GetLength();
				int cur = ip->GetOutputTime();
				
				int xpos = (cur * 219) / len;
				if (xpos < 0) xpos = 0;
				if (xpos >= 219) xpos = 219;
				mw_buttons[WB_SCROLLBAR].value = xpos;
				mw_buttons[WB_SCROLLBAR].bs = mw_buttons[WB_SCROLLBAR].obs + 1;
			} else mw_buttons[WB_SCROLLBAR].value = -1;
			invalidateXYWH(h_mainwin,16,72,248,10);
			break;
		case WM_NCHITTEST: {
					   POINT mp;
					   mp.x = GET_X_PARAM(lParam);
					   mp.y = GET_Y_PARAM(lParam);
					   MapWindowPoints(NULL,hWnd,&mp,1);
					   mouseX = mp.x; mouseY = mp.y;
					   handleHoldEvents(hWnd);
					   LRESULT r = DefWindowProc(hWnd,uMsg,wParam,lParam);
					   if ((can_drag) && (r == HTCLIENT)) r = HTCAPTION;
					   return r;
					   break; }
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
				   struct element* cur = &mw_elements[0];
				   do {
					   i = find_element_to_update(WE_COUNT,mw_elements,&cur);

					   switch (cur - mw_elements) {
						   case WE_TITLEBAR:
							   if (GetForegroundWindow() == hWnd) {
								   skinBlit(hWnd, skin.titlebitmap, 27, 0, 0, 0, 275, 14);
							   } else {
								   skinBlit(hWnd, skin.titlebitmap, 27, 15, 0, 0, 275, 14);
							   }
							   break;
					   }
				   } while (cur);
				   cur=&mw_buttons[0];
				   do {
					   i = find_element_to_update(WB_COUNT,mw_buttons,&cur);
					   switch(cur - mw_buttons) {
						   case WB_MENU:
							   skinBlit(hWnd, skin.titlebitmap,0,i ? 9 : 0,6,3,9,9);
							   break;
						   case WB_MINIMIZE:
							   skinBlit(hWnd, skin.titlebitmap,9,i ? 9 : 0,244,3,9,9);
							   break;
						   case WB_WINDOWSHADE:
							   skinBlit(hWnd, skin.titlebitmap,i ? 9 : 0,18,254,3,9,9);
							   break;
						   case WB_CLOSE:
							   skinBlit(hWnd, skin.titlebitmap,18,i ? 9 : 0,264,3,9,9);
							   break;
						   case WB_PREV:
							   skinBlit(hWnd, skin.cbuttons, 0, i ? 18 : 0, 16,88,23,18);
							   break;	
						   case WB_PLAY:
							   skinBlit(hWnd, skin.cbuttons, 23, i ? 18 : 0, 16+23,88,23,18);
							   break;	
						   case WB_PAUSE:
							   skinBlit(hWnd, skin.cbuttons, 46, i ? 18 : 0, 16+46,88,23,18);
							   break;	
						   case WB_STOP:
							   skinBlit(hWnd, skin.cbuttons, 69, i ? 18 : 0, 16+69,88,23,18);
							   break;	
						   case WB_NEXT:
							   skinBlit(hWnd, skin.cbuttons, 92, i ? 18 : 0, 16+92,88,22,18);
							   break;	
					           case WB_SCROLLBAR:
							   if (mw_buttons[WB_SCROLLBAR].value >= 0) {
								   skinBlit(hWnd, skin.posbar, 0, 0, 16, 72, 248, 10); 
								   skinBlit(hWnd, skin.posbar, 248, 0, 16 + mw_buttons[WB_SCROLLBAR].value, 72, 29, 10); 
							   } else {
								   skinBlit(hWnd, skin.mainbitmap, 16, 72, 16, 72, 248, 10); 
							   }
					   }
				   } while (cur);
				   skinEndPaint(hWnd);
				   break;
		default:
				   return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
}

WNDCLASS mainwin = {0,(WNDPROC)WindowProc,0,0,NULL,NULL,NULL,(HBRUSH) COLOR_BTNFACE+1,NULL,"helloMain"};

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

HBITMAP loadSkinBitmap (const char* filename) {
	HBITMAP r = LoadImage(NULL, filename, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
	if (r == NULL) {
		char errortxt[256];
		snprintf(errortxt,256,"Unable to load %s", filename);
		msgerror(errortxt);
	}
	return r;
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

	mainwin.hInstance = GetModuleHandle(0);
	mainwin.hCursor = LoadCursor(0,IDC_ARROW);

	RegisterClass(&mainwin);
	InitCommonControls();

	// force the initial draw of all elements.
	for (int i=0; i < WE_COUNT; i++ ) {
		mw_elements[i].bs = 0; mw_elements[i].obs = -1;}
	for (int i=0; i < WB_COUNT; i++ ) {
		mw_buttons[i].bs = 0; mw_buttons[i].obs = -1;}

	ampInit();

	h_mainwin = CreateWindowEx(0, "helloMain", "hello world", WS_VISIBLE | WS_POPUP, 128, 128, 275, 116, NULL, NULL, mainwin.hInstance, NULL);

	SetTimer(h_mainwin,0,50,NULL);

	skin.mainbitmap = loadSkinBitmap("skin/main.bmp");
	skin.titlebitmap = loadSkinBitmap("skin/titlebar.bmp");
	skin.cbuttons = loadSkinBitmap("skin/cbuttons.bmp");
	skin.posbar = loadSkinBitmap("skin/posbar.bmp");
	skin.text = loadSkinBitmap("skin/text.bmp");

	//this is where WindowProc will start being called.

	MSG lastmsg;
	while (GetMessage(&lastmsg,NULL,0,0)) {
		TranslateMessage(&lastmsg);
		DispatchMessage(&lastmsg);
	}


	MessageBoxA(0, "hello world", "test", 0);
	return 0;
}
