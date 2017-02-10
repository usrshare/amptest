// vim: cin:sts=4:sw=4
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdbool.h>
#include <locale.h>

#include "menus.h"
#include "wa_plugins.h"
#include "win_misc.h"

char* default_text_layout = 
"Aa\tBb\tCc\tDd\tEe\tFf\tGg\tHh\tIi\tJj\tKk\tLl\tMm\tNn\tOo\tPp\tQq\tRr\tSs\tTt\tUu\tVv\tWw\tXx\tYy\tZz\t\"\t@\n"
"0\t1\t2\t3\t4\t5\t6\t7\t8\t9\t…\t.\t:\t(\t)\t-\t'\t!\t_\t+\t\\\t/\t[\t]\t^\t&\t%\t.\t=\t$\t#\n"
"Åå\tÖö\tÄä\t?\t*\t ";
char* allocd_text_layout = NULL;
char* text_layout = NULL;

char filePath[1024];

struct waInputPlugin* ip = NULL;
struct waOutputPlugin* op = NULL;

struct skinData {
    HBITMAP mainbitmap;
    HBITMAP titlebitmap;
    HBITMAP cbuttons;
    HBITMAP posbar;
    HBITMAP text;
    HBITMAP monoster;
    HBITMAP playpaus;
    HBITMAP nums_ex;
} skin;

struct playbackData {
    int bitrate;
    int samplerate;
    int channels;
    int synched;
} pb;

HWND h_mainwin;

HMENU h_mainmenu;

bool doubleMode = false;

int timercnt = 0;
int scrollcnt = 0;

#define TIMER_BLANK INT_MIN

enum window_types {
    WT_MAIN = 0,
    WT_EQUALIZER,
    WT_PLAYLIST
};

struct windowData {

    //this struct is allocated for each instance of a window and pointed to with
    //set/getWindowData(). any function that receives an hWnd can get the pointer
    //to the structure.

    enum window_types type; //useful for determining which elements this window controls
    HBITMAP hbmpBuf; // bitmap for double buffering
    HDC hdcMemBuf; //hdcmem for dbl buffering

    PAINTSTRUCT ps;
    HDC hdc; //hdc for the window itself
    HDC hdcMem; //hdcMem for the window itself
};

struct windowData* getWindowData(HWND hWnd) {
    return (struct windowData*) GetWindowLongPtr(hWnd,0);
}

int skinInitializePaint(HWND hWnd) {

    HDC wndDC = GetDC(hWnd);
    struct windowData* wdata = getWindowData(hWnd);
    wdata->hdcMem = CreateCompatibleDC(wndDC);
    wdata->hdcMemBuf = CreateCompatibleDC(wndDC);
    wdata->hbmpBuf = CreateCompatibleBitmap(wndDC, 275, 116);
    ReleaseDC(hWnd, wndDC);

    return 0;
}

int skinBlit(HWND hWnd, HBITMAP src, int xs, int ys, int xd, int yd, int w, int h) {

    //this function is used whenever part of a window has to be updated.
    //it can be called at any moment.
    //it updates the hbmpBuf bitmap.

    struct windowData* wdata = getWindowData(hWnd);
    BITMAP srcBuf; //source bitmap.
    GetObject(src, sizeof srcBuf, &srcBuf);

    HGDIOBJ oldSrcBuf = SelectObject(wdata->hdcMemBuf, src);
    HGDIOBJ oldDstBuf = SelectObject(wdata->hdcMem, wdata->hbmpBuf);

    BitBlt(wdata->hdcMem, xd, yd, w ? w : srcBuf.bmWidth, h ? h : srcBuf.bmHeight, wdata->hdcMemBuf, xs, ys, SRCCOPY);

    SelectObject(wdata->hdcMem, oldDstBuf);
    SelectObject(wdata->hdcMemBuf, oldSrcBuf);

    return 0;
}

int windowBlit(HWND hWnd) {

    //this function is only used at the end of the WM_PAINT message, and it
    //redraws the entire window from the bitmap.

    struct windowData* wdata = getWindowData(hWnd);
    BITMAP wndBuf; // window bitmap
    GetObject(wdata->hbmpBuf, sizeof wndBuf, &wndBuf);

    wdata->hdc = BeginPaint(hWnd, &(wdata->ps));

    HGDIOBJ oldBmp = SelectObject(wdata->hdcMem, wdata->hbmpBuf);
    BitBlt(wdata->hdc, 0, 0, wndBuf.bmWidth, wndBuf.bmHeight, wdata->hdcMem, 0, 0, SRCCOPY); //entire bitmap
    SelectObject(wdata->hdcMem,oldBmp);

    EndPaint(hWnd, &(wdata->ps));
    return 0;
}

int skinDestroyPaint(HWND hWnd) {

    struct windowData* wdata = getWindowData(hWnd);
    DeleteObject(wdata->hbmpBuf);
    DeleteDC(wdata->hdcMemBuf);
    wdata->hdcMemBuf = 0;
    DeleteDC(wdata->hdcMem);
    wdata->hdcMem = 0;
    return 0;
}



int iterate_utf8_codepoint(const char* cur, unsigned int* u_val) {

    if ((!cur) || (!u_val)) {
	return -1;
    }
    const unsigned char* c = (const unsigned char*) cur;

    if (c[0] == 0)   { *u_val = 0; return 0; }
    if (c[0] < 0x80) { *u_val = c[0]; return 1; } //0x00 - 0x7F, 1 byte
    if (c[0] < 0xC0) { *u_val = '?'; return 1; } //0x80 - 0xBF, continuation byte
    if (c[0] < 0xE0) { *u_val = ((c[0] & 0x1f) << 6) + (c[1] & 0x3F); return 2; }
    if (c[0] < 0xF0) { *u_val = ((c[0] & 0x0f) << 12) + ((c[1] & 0x3F) << 6) + (c[2] & 0x3F); return 3; }
    /* if (c[0] < 0xF8) { */
    *u_val = ((c[0] & 0x07) << 18) + ((c[1] & 0x3F) << 12) + ((c[2] & 0x3F) << 6) + (c[3] & 0x3F); return 4;
    /* } */
}

int find_utf8char_in_utf8layout(unsigned int c_cp, const char* layout, int* o_x, int* o_y) {

    int x=0, y=0;
    unsigned int l_cp = 0;
    const char* cur_l = layout;
    int r = 1;
    do {
	r = iterate_utf8_codepoint(cur_l, &l_cp);

	if (l_cp == '\t') x++;
	if (l_cp == '\n') {x = 0; y++;}

	if (l_cp == c_cp) {*o_x = x; *o_y = y; return 0;}

	cur_l += r;
    } while (r > 0);


    *o_x = 3; *o_y = 2; //location of the ? character
    return -1;
}

int skinTextLengthC(const char* text) { //length in codepoints

    const char* cur = text;
    int r = 0;
    unsigned int c_cp = 0;

    int dx=0;

    do {
	r = iterate_utf8_codepoint(cur,&c_cp);
	if (r == 0) return dx;
	dx++;
	cur += r;
    } while (true);
}

int skinTextLength(const char* text) {
    return skinTextLengthC(text) * 5;
}
/*
   int skinDrawTextScroll(HWND hWnd, const char* text, int x, int y, int w, int scroll, bool noclear) {

   const char* cur = text;
   int c_x = 0, c_y = 0, r = 0;
   unsigned int c_cp = 0;

   int dx= -scroll;

   if (!noclear) {
   for (int ix=0; ix < (w / 5); ix++) 
   skinBlit(hWnd, skin.text, 150,0,x+(ix*5),y,5,6); //tile with empty space
   }

   do {
   r = iterate_utf8_codepoint(cur,&c_cp);
//printf("Character '%.*s' (%db) has a code %d\n",r,cur,r,c_cp);
if (r == 0) return 0;

find_utf8char_in_utf8layout(c_cp, text_layout, &c_x, &c_y); 

int cwidth = (c_cp == '@' ? 7 : 5);

if (dx >= w) return 0; //if we're already full, end here.
if ((dx + cwidth) > w) cwidth = w - dx; //cut text length to whatever fits

if ((dx + cwidth) > 0) { //if this character is at least partially visible,

int xsh = (dx < 0 ? -dx : 0);

skinBlit(hWnd, skin.text, (c_x*5) + xsh, c_y*6, x + dx + xsh, y, cwidth - xsh, 6);
// if dx < 0, then that means this character should be only blitted partially.
}

dx += (c_cp == '@' ? 7 : 5);
cur += r;

} while (r > 0);
return 0;
}*/

int skinDrawText(HWND hWnd, const char* text, int x, int y, int w, int skip) {

    const char* cur = text;
    int c_x = 0, c_y = 0, r = 0;
    unsigned int c_cp = 0;

    if (w < 0) return 0;
    int dx = 0;

    for (int ix=0; ix < (w / 5); ix++) 
	skinBlit(hWnd, skin.text, 150,0,x+(ix*5),y,5,6); //tile with empty space

    int chars = 0;
    do {
	r = iterate_utf8_codepoint(cur,&c_cp);
	//printf("Character '%.*s' (%db) has a code %d\n",r,cur,r,c_cp);
	if (r == 0) return 0;

	if (chars >= skip) {
	    find_utf8char_in_utf8layout(c_cp, text_layout, &c_x, &c_y); 

	    if (dx >= w) return 0; //if we're already full, end here.

	    skinBlit(hWnd, skin.text, (c_x*5), c_y*6, x + dx, y, 5, 6);

	    dx += 5;
	}
	cur += r;
	chars++;

    } while (r > 0);
    return 0;
}

const char* nums_ex_layout = "0123456789 -"; //

int skinDrawTimeString(HWND hWnd, const char* text, int x, int y) {

    const char* c = text;
    int dx = x;
    while (*c != 0) {
	char* ex = strchr(nums_ex_layout, *c);
	if (ex) {
	    int sx = (ex - nums_ex_layout) * 9;
	    skinBlit(hWnd,skin.nums_ex,sx,0,dx,y,9,13);
	}
	dx += 13;
	c++;
	if (c == text+3) dx += 2;
    }
    return 0;
}

int skinDrawTime(HWND hWnd, int time, int x, int y) {

    char text[6];
    snprintf(text,6,"% 03d%02d\n", time / 60, abs(time) % 60);
    return skinDrawTimeString(hWnd,text,x,y);
}

CONST RECT mainTitleRect = {.left = 0, .top = 0, .right = 275, .bottom = 14};

struct mouseData {
    short clickX;
    short clickY;
    short releaseX;
    short releaseY;
    short x;
    short y;
    short buttons;
};

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

enum element_avail {
    EA_NEVER = 0, //never available
    EA_NORMAL = 1, //only when the window is in its normal state
    EA_WINSHADE = 2, //only when the window is in (yet to be implemented) windowshade
    EA_ALWAYS = 3, //always
};

struct element {

    unsigned int x,y,w,h; //position

    int obs,bs; //old state and current state. whenever bs != obs, this element
    //is redrawn.

    enum element_avail avail; //when is the element available
    int button; //specifies if the element should be treated as a button
    //that means the window can't be dragged by this element,
    //and that bs can be rewritten if the btton is held.

    int value; //additional value. useful for elements where button==1 and
    //therefore bs can't always hold the useful value.
};

enum windowelements {
    WE_BACKGROUND,
    WE_TITLEBAR,
    WE_PLAYPAUS,
    WE_TIMER,
    WE_TITLE,
    WE_BITRATE, //kbps
    WE_MIXRATE, //kHz
    WE_MONOSTER,

    WE_B_MENU,
    WE_B_MINIMIZE,
    WE_B_WINDOWSHADE,
    WE_B_CLOSE,
    WE_B_OAIDV,
    WE_B_VOLUME,
    WE_B_BALANCE,
    WE_B_EQ,
    WE_B_PL,
    WE_B_SCROLLBAR,
    WE_B_PREV,
    WE_B_PLAY,
    WE_B_PAUSE,
    WE_B_STOP,
    WE_B_NEXT,
    WE_B_OPEN,
    WE_B_SHUFFLE,
    WE_B_REPEAT,
    WE_COUNT
};

enum PLAYPAUS_STATUS {
    PS_STOP,
    PS_PAUSE,
    PS_PLAY_NOSYNCH,
    PS_PLAY_SYNCH,
};

struct element mw_elements[WE_COUNT] = {
    {  .x = 0,   .y = 0, .w = 275, .h = 116}, //window background
    {  .x = 0,   .y = 0, .w = 275,  .h = 14}, //titlebar
    {  .x = 24,  .y = 28,.w =  11,  .h = 9},  //play/pause indicator
    {  .x = 35,  .y = 26,.w =  63,  .h = 13}, //timer
    {  .x = 110, .y = 27,.w = 155,  .h = 6},  //title scroller 
    {  .x = 111, .y = 43,.w = 15,   .h = 6},  //bitrate
    {  .x = 156, .y = 43,.w = 10,   .h = 6},  //sample rate
    {  .x = 212, .y = 41,.w = 56,   .h = 12}, //mono/stereo

    {  .x = 6,   .y = 3, .w = 9,  .h = 9,	.button=1}, //menu
    {  .x = 244, .y = 3, .w = 9,  .h = 9,	.button=1}, //minimize
    {  .x = 254, .y = 3, .w = 9,  .h = 9,	.button=1}, //windowshade
    {  .x = 264, .y = 3, .w = 9,  .h = 9,	.button=1}, //close
    {  .x = 11, .y = 22, .w = 10, .h = 43,	.button=1}, //OAIDV -- larger than the assoc. images
    {  .x = 999, .y = 999, .w = 0, .h = 0,	.button=1}, //volume
    {  .x = 999, .y = 999, .w = 0, .h = 0,	.button=1}, //balance
    {  .x = 999, .y = 999, .w = 0, .h = 0,	.button=1}, //equalizer btn
    {  .x = 999, .y = 999, .w = 0, .h = 0,	.button=1}, //playlist btn
    {  .x = 16, .y = 72, .w = 248, .h = 10,	.button=1}, //scroll bar
    {  .x = 16, .y = 88, .w = 23, .h = 18,	.button=1}, //prev
    {  .x = 39, .y = 88, .w = 23, .h = 18,	.button=1}, //play
    {  .x = 62, .y = 88, .w = 23, .h = 18,	.button=1}, //pause
    {  .x = 85, .y = 88, .w = 23, .h = 18,	.button=1}, //stop
    {  .x = 108, .y = 88, .w = 22, .h = 18,	.button=1}, //next
    {  .x = 136, .y = 89, .w = 22, .h = 16,	.button=1}, //open
};

void UI_SetInfo(int br, int sr, int st, int synch) {
    if (br != -1) {
	pb.bitrate = br;
	mw_elements[WE_BITRATE].bs = br;
    }
    if (sr != -1) {
	pb.samplerate = sr;
	mw_elements[WE_MIXRATE].bs = sr;
    }
    if (st != -1) {
	pb.channels = st;
	mw_elements[WE_MONOSTER].bs = st;
    }
    if (synch != -1) {
	pb.synched = synch;
	mw_elements[WE_PLAYPAUS].bs = st ? PS_PLAY_SYNCH : PS_PLAY_NOSYNCH;
    }
}

int handleHoldEvents(HWND hWnd) {

    can_drag = 1;

    for (int i=0; i < WE_COUNT; i++) {
	struct element* e = &mw_elements[i];
	if (!e->button) continue; //only work on buttons
	if (hover(e->x, e->y, e->w, e->h)) can_drag = 0;
	e->bs = hold(e->x,e->y,e->w,e->h,1);
	if (e->bs != e->obs) invalidateXYWH(h_mainwin,e->x,e->y,e->w,e->h);
    }
    return 0;
}

int get_click_button() {
    for (int i=0; i < WE_COUNT; i++) {
	struct element* cb = &mw_elements[i];
	if (!cb->button) continue; //only work on buttons
	if (click(cb->x,cb->y,cb->w,cb->h,1)) return i;
    }
    return -1;
}

int handleDoubleClickEvents(HWND hWnd) {

    switch (get_click_button()) {
    }
    return 0;
}

int filePlay(void) {
    if (op->IsPlaying()) ip->Stop();
    ip->Play(filePath);
    return 0;
}

int openFileAndPlay(void) {

    OPENFILENAME ofn = {
	.lStructSize = sizeof ofn,
	.hwndOwner = h_mainwin,
	.hInstance = NULL,
	.lpstrFilter = "MP3 Files\0*.mp3\0\0",
	.lpstrCustomFilter = NULL,
	.nMaxCustFilter = 0,
	.nFilterIndex = 1,
	.lpstrFile = filePath,
	.nMaxFile = 1024,
    };
    BOOL r = GetOpenFileName(&ofn);
    if (!r) return 1;
    return filePlay();
}

int handleClickEvents(HWND hWnd) {

    switch (get_click_button()) {
	case WE_B_MENU: {
			  POINT mp = {.x = 6, .y = 12};
			  MapWindowPoints(hWnd,NULL,&mp,1);
			  HMENU h_sysmenu = GetSubMenu(h_mainmenu, 0);
			  TrackPopupMenuEx(h_sysmenu,TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON,mp.x,mp.y,h_mainwin,NULL);
			  break; }
	case WE_B_CLOSE:
		      ExitProcess(0);
		      break;
	case WE_B_PAUSE:
		      if (ip->IsPaused())
			  ip->UnPause();
		      else
			  ip->Pause();
		      break;
	case WE_B_STOP:
		      if (op->IsPlaying()) ip->Stop();
		      break;
	case WE_B_OPEN: {
			  OPENFILENAME ofn = {
			      .lStructSize = sizeof ofn,
			      .hwndOwner = h_mainwin,
			      .hInstance = NULL,
			      .lpstrFilter = "MP3 Files\0*.mp3\0\0",
			      .lpstrCustomFilter = NULL,
			      .nMaxCustFilter = 0,
			      .nFilterIndex = 1,
			      .lpstrFile = filePath,
			      .nMaxFile = 1024,
			  };
			  BOOL r = GetOpenFileName(&ofn);
			  if (!r) break;
		      }
	case WE_B_PLAY:
		      if (op->IsPlaying()) ip->Stop();
		      ip->Play(filePath);


		      break;
    }
    return 0;
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
  
INT_PTR CALLBACK URLDialogProc( HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
    switch (uMsg) 
    { 
	case WM_INITDIALOG:
	    return true;
	case WM_COMMAND: 
	    switch (LOWORD(wParam)) 
	    { 
		case IDOK: 
		    if (!GetDlgItemText(hWndDlg, IDD_INPUT, filePath, 1024)) 
			filePath[0]=0; //if failed to receive filePath, set it to empty string
		    // Fall through. 
		case IDCANCEL: 
		    EndDialog(hWndDlg, wParam); 
		    return TRUE; 
	    } 
    } 
    return FALSE;    
}

LRESULT WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch(uMsg) {
	case WM_SETFOCUS:
	case WM_KILLFOCUS:
	    mw_elements[WE_TITLEBAR].bs = (uMsg == WM_SETFOCUS);
	    if (mw_elements[WE_TITLEBAR].bs != mw_elements[WE_TITLEBAR].obs) InvalidateRect(hWnd,&mainTitleRect,0);
	    return DefWindowProc(hWnd,uMsg,wParam,lParam);
	    break;
	case WM_CREATE: {
			    //create the struct and store it in the pointer provided.
			    struct windowData* wd = malloc(sizeof(struct windowData));
			    memset(wd, 0, sizeof *wd);
			    SetWindowLongPtr(hWnd,0,(LONG_PTR)wd);
			    skinInitializePaint(hWnd);
			    return 0;
			    break; }
	case WM_COMMAND: {
			     int menuid = LOWORD(wParam);
			     if (HIWORD(wParam) == 0) {
				 switch (menuid) {
				     case IDM_I_OPENFILE:
					 return openFileAndPlay();
					 break;
				     case IDM_I_OPENLOC:
					 break;
				     case IDM_I_OPENURL:
					 DialogBox(NULL,MAKEINTRESOURCE(IDD_OPENLOC),hWnd,URLDialogProc);
					 break;
				     case IDM_I_FILEINFO:
					 ip->InfoBox(filePath, hWnd);
					 break;
				     case IDM_I_ABOUT:
					 MessageBoxA(hWnd, "amptest - https://github.com/usrshare/amptest/", "About amptest", MB_ICONINFORMATION);
					 break;
				     case IDM_I_EXIT:
					 ExitProcess(0);
					 break;
				 }
			     }

			     //if (LOWORD(wParam) == 1) ip->Play("demo.mp3");
			     return 0;
			     break; }
	case WM_DESTROY:
			 skinDestroyPaint(hWnd);
			 free (getWindowData(hWnd)); //free the data now that the window is gone.
			 PostQuitMessage(0);
			 return 0;
			 break;
	case WM_TIMER:
			 timercnt++;	

			 if ((timercnt % 10) == 0) { //scroll bar
			     if (op->IsPlaying()) {
				 int len = ip->GetLength();
				 int cur = ip->GetOutputTime();

				 int xpos = (cur * 219) / len;
				 if (xpos < 0) xpos = 0;
				 if (xpos >= 219) xpos = 219;
				 mw_elements[WE_B_SCROLLBAR].value = xpos;
				 mw_elements[WE_B_SCROLLBAR].bs = xpos;
			     } else mw_elements[WE_B_SCROLLBAR].value = -1;
			 }

			 if (op->IsPlaying()) {
			     if ((timercnt % 5) == 0) scrollcnt++; 
			     mw_elements[WE_TITLE].bs = 1 + scrollcnt;
			     if (ip->IsPaused()) { mw_elements[WE_PLAYPAUS].bs = PS_PAUSE; }
			     mw_elements[WE_TIMER].bs = (ip->IsPaused() && (timercnt % 20 >= 10)) ? TIMER_BLANK : (ip->GetOutputTime() / 1000);
			 } else {
			     scrollcnt = 0;
			     mw_elements[WE_TITLE].bs = 0;
			     mw_elements[WE_MONOSTER].bs = 0; //remove both mono and stereo
			     mw_elements[WE_PLAYPAUS].bs = PS_STOP;
			     mw_elements[WE_TIMER].bs = TIMER_BLANK;
			 }

			 for (int i=0; i < WE_COUNT; i++) {
			     struct element* e = &mw_elements[i];
			     if (e->bs != e->obs) invalidateXYWH(h_mainwin,e->x,e->y,e->w,e->h);
			 }

			 break;
	case WM_NCHITTEST: {
			       POINT mp = {GET_X_PARAM(lParam),GET_Y_PARAM(lParam) };
			       MapWindowPoints(NULL,hWnd,&mp,1);
			       mouseX = mp.x; mouseY = mp.y;
			       handleHoldEvents(hWnd);
			       LRESULT r = DefWindowProc(hWnd,uMsg,wParam,lParam);
			       if ((can_drag) && (r == HTCLIENT)) r = HTCAPTION;
			       return r; } 
	case WM_LBUTTONDOWN:
			   //record press location for "pushed down" buttons.
			   mouseClickX = LOWORD(lParam);
			   mouseClickY = HIWORD(lParam);
			   mouseButtons |= 1;
			   handleHoldEvents(hWnd);
			   return DefWindowProc(hWnd,uMsg,wParam,lParam);
	case WM_LBUTTONUP:
			   mouseReleaseX = LOWORD(lParam);
			   mouseReleaseY = HIWORD(lParam);
			   //handle all the "click" events here.
			   handleClickEvents(hWnd);
			   mouseButtons &= (~1);
			   handleHoldEvents(hWnd); //release held buttons
			   return DefWindowProc(hWnd,uMsg,wParam,lParam);
	case WM_LBUTTONDBLCLK:
			   mouseReleaseX = LOWORD(lParam);
			   mouseReleaseY = HIWORD(lParam);
			   handleDoubleClickEvents(hWnd);
			   mouseButtons &= (~1);
			   return DefWindowProc(hWnd,uMsg,wParam,lParam);
	case WM_MOUSEMOVE:
			   mouseX = LOWORD(lParam);
			   mouseY = HIWORD(lParam);
			   handleHoldEvents(hWnd);
			   return DefWindowProc(hWnd,uMsg,wParam,lParam);
	case WM_PAINT: {


			   int i=0;
			   struct element* cur = &mw_elements[0];
			   do {
			       i = find_element_to_update(WE_COUNT,mw_elements,&cur);

			       switch (cur - mw_elements) {
				   case WE_BACKGROUND:
				       skinBlit(hWnd, skin.mainbitmap, 0, 0, 0, 0, 0, 0);
				       break;
				   case WE_TITLEBAR:
				       if (GetForegroundWindow() == hWnd) {
					   skinBlit(hWnd, skin.titlebitmap, 27, 0, 0, 0, 275, 14);
				       } else {
					   skinBlit(hWnd, skin.titlebitmap, 27, 15, 0, 0, 275, 14);
				       }
				       break;
				   case WE_PLAYPAUS: {
							 switch(cur->bs) {
							     case PS_STOP:
								 skinBlit(hWnd, skin.mainbitmap, cur->x, cur->y, cur->x, cur->y, 2, 9);
								 skinBlit(hWnd, skin.playpaus, 18, 0, cur->x + 2, cur->y,9, 9);
								 break;
							     case PS_PAUSE:
								 skinBlit(hWnd, skin.mainbitmap, cur->x, cur->y, cur->x, cur->y, 2, 9);
								 skinBlit(hWnd, skin.playpaus, 9, 0, cur->x + 2, cur->y, 9, 9);
								 break;
							     case PS_PLAY_NOSYNCH:
								 skinBlit(hWnd, skin.playpaus, 1, 0, cur->x + 3, cur->y, 8, 9);
								 skinBlit(hWnd, skin.playpaus, 39, 0, cur->x, cur->y, 3, 9);
								 break;
							     case PS_PLAY_SYNCH:
								 skinBlit(hWnd, skin.playpaus, 1, 0, cur->x + 3, cur->y, 8, 9);
								 skinBlit(hWnd, skin.playpaus, 36, 0, cur->x, cur->y, 3, 9);
								 break;
							 }

						     }
						     break;
				   case WE_TIMER: {
						      if (cur->bs == TIMER_BLANK) {
							  skinDrawTimeString(hWnd, "     ", cur->x, cur->y);
						      } else {
							  skinDrawTime(hWnd, ip->GetOutputTime()/1000, cur->x, cur->y);
						      }
						      break; }
				   case WE_TITLE: {

						      char title[128];

						      sprintf(title,"hello world");
						      int titlesz = skinTextLength(title);

						      if (op->IsPlaying()) {

							  int lms = 0;
							  char f_title[GETFILEINFO_TITLE_LENGTH];
							  ip->GetFileInfo(NULL,f_title,&lms);

							  snprintf(title,128,"%.112s (%d:%02d)",f_title, lms / (60 * 1000), abs((lms / 1000) % 60));
							  titlesz = skinTextLength(title);
							  if (titlesz > cur->w) { strcat(title, " *** "); titlesz = skinTextLength(title); }
						      }

						      while (scrollcnt >= skinTextLengthC(title)) scrollcnt = 0;

						      if (titlesz <= (cur->w)) {
							  skinDrawText(h_mainwin,title,cur->x,cur->y,cur->w,0);
						      } else {
							  skinDrawText(h_mainwin,title,cur->x,cur->y,cur->w,scrollcnt % titlesz);
							  skinDrawText(h_mainwin,title,cur->x + titlesz - ((5*scrollcnt) % titlesz),cur->y,cur->w - (titlesz - ((5*scrollcnt) % titlesz)),0);
						      }
						  }
						  break;
				   case WE_BITRATE: {
							invalidateXYWH(hWnd,cur->x,cur->y,cur->w,cur->h);
							char bitrate[4];
							snprintf(bitrate,4,"%3d",pb.bitrate);
							skinDrawText(h_mainwin,bitrate,cur->x,cur->y,cur->w,0);
							break;
						    }
				   case WE_MIXRATE: {
							invalidateXYWH(hWnd,cur->x,cur->y,cur->w,cur->h);
							char samplerate[3];
							snprintf(samplerate,3,"%2d",pb.samplerate);
							skinDrawText(h_mainwin,samplerate,cur->x,cur->y,cur->w,0);
							break;
						    }
				   case WE_MONOSTER: {
							 invalidateXYWH(hWnd,cur->x,cur->y,cur->w,cur->h);

							 skinBlit(hWnd, skin.monoster, 29, (pb.channels == 1) ? 0 : 12, cur->x, cur->y, 27, 12);
							 skinBlit(hWnd, skin.monoster, 0, (pb.channels >= 2) ? 0 : 12, cur->x + 27, cur->y, 29, 12);
							 break;
						     }
				   case WE_B_MENU:
				       skinBlit(hWnd, skin.titlebitmap,0,i ? 9 : 0,6,3,9,9);
				       break;
				   case WE_B_MINIMIZE:
				       skinBlit(hWnd, skin.titlebitmap,9,i ? 9 : 0,244,3,9,9);
				       break;
				   case WE_B_WINDOWSHADE:
				       skinBlit(hWnd, skin.titlebitmap,i ? 9 : 0,18,254,3,9,9);
				       break;
				   case WE_B_CLOSE:
				       skinBlit(hWnd, skin.titlebitmap,18,i ? 9 : 0,264,3,9,9);
				       break;
				   case WE_B_PREV:
				       skinBlit(hWnd, skin.cbuttons, 0, i ? 18 : 0, 16,88,23,18);
				       break;	
				   case WE_B_PLAY:
				       skinBlit(hWnd, skin.cbuttons, 23, i ? 18 : 0, 16+23,88,23,18);
				       break;	
				   case WE_B_PAUSE:
				       skinBlit(hWnd, skin.cbuttons, 46, i ? 18 : 0, 16+46,88,23,18);
				       break;	
				   case WE_B_STOP:
				       skinBlit(hWnd, skin.cbuttons, 69, i ? 18 : 0, 16+69,88,23,18);
				       break;	
				   case WE_B_NEXT:
				       skinBlit(hWnd, skin.cbuttons, 92, i ? 18 : 0, 16+92,88,22,18);
				       break;
				   case WE_B_OPEN:
				       skinBlit(hWnd, skin.cbuttons, 114, i ? 16 : 0, 136,89,22,16);
				       break; 
				   case WE_B_SCROLLBAR:
				       invalidateXYWH(hWnd,cur->x,cur->y,cur->w,cur->h);
				       if (mw_elements[WE_B_SCROLLBAR].value >= 0) {
					   skinBlit(hWnd, skin.posbar, 0, 0, 16, 72, 248, 10); 
					   skinBlit(hWnd, skin.posbar, 248, 0, 16 + mw_elements[WE_B_SCROLLBAR].value, 72, 29, 10); 
				       } else {
					   skinBlit(hWnd, skin.mainbitmap, 16, 72, 16, 72, 248, 10); 
				       }
			       }
			   } while (cur);

			   windowBlit(hWnd);
			   break;
		       }
	default:
		       return DefWindowProc(hWnd,uMsg,wParam,lParam);
    }
    return 0;
}

WNDCLASS mainwin = {0,(WNDPROC)WindowProc,0,sizeof (void*),NULL,NULL,NULL,(HBRUSH) COLOR_BTNFACE+1,NULL,"helloMain"};

int ampInit() {

    op = loadOutputPlugin("plugins/out_wave.dll");
    if (!op) { return 1; }

    ip = loadInputPlugin("plugins/in_mp3.dll",op);
    ip->SetInfo = UI_SetInfo;
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
	msgerror(filename);
    }
    return r;
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

    SetConsoleOutputCP(65001); //this program still outputs everything as UTF-8, even when running in Windows.

    mainwin.hInstance = GetModuleHandle(0);
    mainwin.hCursor = LoadCursor(0,IDC_ARROW);

    RegisterClass(&mainwin);
    InitCommonControls();

    h_mainmenu = LoadMenu(mainwin.hInstance, MAKEINTRESOURCE(IDM_MAINMENU) );

    // force the initial draw of all elements.
    for (int i=0; i < WE_COUNT; i++ ) {
	mw_elements[i].bs = 0; mw_elements[i].obs = -1;}

    strcpy(filePath,"demo.mp3");

    ampInit();

    h_mainwin = CreateWindowEx(0, "helloMain", "hello world", WS_VISIBLE | WS_POPUP, 128, 128, 275, 116, NULL, NULL, mainwin.hInstance, NULL);


    SetTimer(h_mainwin,0,50,NULL);

    skin.mainbitmap = loadSkinBitmap("skin\\main.bmp");
    skin.titlebitmap = loadSkinBitmap("skin\\titlebar.bmp");
    skin.cbuttons = loadSkinBitmap("skin\\cbuttons.bmp");
    skin.posbar = loadSkinBitmap("skin\\posbar.bmp");
    skin.text = loadSkinBitmap("skin\\text.bmp");
    skin.monoster = loadSkinBitmap("skin\\monoster.bmp");
    skin.playpaus = loadSkinBitmap("skin\\playpaus.bmp");
    skin.nums_ex = loadSkinBitmap("skin\\nums_ex.bmp");

    FILE* textlayout = NULL;
    if ( (textlayout = fopen("skin/text.txt","rb")) ) {

	fseek(textlayout,0,SEEK_END);
	long fsz = ftell(textlayout);
	fseek(textlayout,0,SEEK_SET);

	allocd_text_layout = realloc(allocd_text_layout,fsz);
	fread(allocd_text_layout,fsz,1,textlayout);
	fclose(textlayout);
	text_layout = allocd_text_layout;

    } else text_layout = default_text_layout;

    //this is where WindowProc will start being called.

    MSG lastmsg;
    while (GetMessage(&lastmsg,NULL,0,0)) {
	TranslateMessage(&lastmsg);
	DispatchMessage(&lastmsg);
    }


    MessageBoxA(0, "hello world", "test", 0);
    return 0;
}
