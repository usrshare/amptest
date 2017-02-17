// vim: cin:sts=4:sw=4
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <locale.h>

#include "wa_plugins.h"
#include "ui.h"
#include "ui_string.h"
#include "menus.h"
#include "win_misc.h"


char filePath[1024];

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

bool doubleMode = false;

int timercnt = 0;
int scrollcnt = 0;

#define TIMER_BLANK INT_MIN

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
    if (time >= 0) {
	snprintf(text,6,"% 03d%02d\n", time / 60, time % 60);
    } else {
	snprintf(text,6,"-%02d%02d\n", abs(time) / 60, abs(time) % 60);
    }
    return skinDrawTimeString(hWnd,text,x,y);
}

int hover(short x, short y, short w, short h) {

    if ((mouse.X >= x) && (mouse.X < (x+w)) &&
	    (mouse.Y >= y) && (mouse.Y < (y+h))) return 1; else return 0;
}

int hold(short x, short y, short w, short h, short bmask) {

    if ( (mouse.clickX >= x) && (mouse.clickX < (x+w)) &&
	    (mouse.clickY >= y) && (mouse.clickY < (y+h)) &&
	    (mouse.X >= x) && (mouse.X < (x+w)) &&
	    (mouse.Y >= y) && (mouse.Y < (y+h)) &&
	    (mouse.buttons & bmask)) {

	return 1;
    }
    return 0;
}

int click(short x, short y, short w, short h, short bmask) {

    if ( (mouse.clickX >= x) && (mouse.clickX < (x+w)) &&
	    (mouse.clickY >= y) && (mouse.clickY < (y+h)) &&
	    (mouse.X >= x) && (mouse.X < (x+w)) &&
	    (mouse.Y >= y) && (mouse.Y < (y+h)) && 
	    (mouse.buttons & bmask) ) {

	mouse.clickX = -1; mouse.clickY = -1;
	return 1;
    }
    return 0;
}


enum element_avail {
    EA_NEVER = 0, //never available
    EA_NORMAL = 1, //only when the window is in its normal state
    EA_WINSHADE = 2, //only when the window is in (yet to be implemented) windowshade
    EA_ALWAYS = 3, //always
};

enum element_type {    
    ET_LABEL = 0, //no interaction
    ET_BUTTON = 1, // works as a click button
    ET_HSLIDER = 2, //horizontal slider (position,volume,balance)
    ET_VSLIDER = 3, //vertical slider (equalizer)
    ET_MDBUTTON = 4, // like a button, but reacts on mousedown instead of click
};

enum element_events {
    EE_CLICK,
    EE_HOLD,
    EE_DBLCLICK,
};

struct element;

typedef void (*elementcb)(struct element* el, enum element_events ev);


struct element {

    unsigned int x,y,w,h; //position

    int obs,bs; //old state and current state. whenever bs != obs, this element
    //is redrawn.

    enum element_avail avail; //when is the element available
    enum element_type type;
    //specifies if the element should be treated specially.
    //that means the window can't be dragged by this element,
    //and that bs can be rewritten if the element is held.

    int value; //additional value. useful for non-label elements, where
    //bs can't always hold the useful value.

    elementcb drag_cb;
    elementcb click_cb;
    elementcb dblclick_cb;

    unsigned int slider_w; //width (horiz) or height (vert) for sliders
};

int getHSliderValue(struct element* cur) {

    int mCX = mouse.clickX - cur->x; // x position at the moment of click
    int mRX = mouse.X - cur->x; // x position at the current moment

    int newval;

    if ( (mCX >= cur->value) && (mCX < (cur->value + cur->slider_w + 1)) ) {
	newval = cur->value + (mRX - mCX);
    } else {
	newval = mRX - (cur->slider_w / 2);
    }

    if (newval < 0) return 0;
    if (newval > (cur->w - cur->slider_w + 1)) return (cur->w - cur->slider_w + 1);
    return newval;
}

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
    {  .x = 35,  .y = 26,.w =  63,  .h = 13, .type = ET_MDBUTTON}, //timer
    {  .x = 110, .y = 27,.w = 155,  .h = 6,  .type = ET_MDBUTTON},  //title scroller 
    {  .x = 111, .y = 43,.w = 15,   .h = 6},  //bitrate
    {  .x = 156, .y = 43,.w = 10,   .h = 6},  //sample rate
    {  .x = 212, .y = 41,.w = 56,   .h = 12}, //mono/stereo

    {  .x = 6,   .y = 3, .w = 9,  .h = 9,	.type=ET_BUTTON}, //menu
    {  .x = 244, .y = 3, .w = 9,  .h = 9,	.type=ET_BUTTON}, //minimize
    {  .x = 254, .y = 3, .w = 9,  .h = 9,	.type=ET_BUTTON}, //windowshade
    {  .x = 264, .y = 3, .w = 9,  .h = 9,	.type=ET_BUTTON}, //close
    {  .x = 11, .y = 22, .w = 10, .h = 43,	.type=ET_BUTTON}, //OAIDV -- larger than the assoc. images
    {  .x = 999, .y = 999, .w = 0, .h = 0,	.type=ET_HSLIDER, .slider_w = 14}, //volume
    {  .x = 999, .y = 999, .w = 0, .h = 0,	.type=ET_HSLIDER, .slider_w = 14}, //balance
    {  .x = 999, .y = 999, .w = 0, .h = 0,	.type=ET_BUTTON}, //equalizer btn
    {  .x = 999, .y = 999, .w = 0, .h = 0,	.type=ET_BUTTON}, //playlist btn
    {  .x = 16, .y = 72, .w = 248, .h = 10,	.type=ET_HSLIDER, .slider_w = 28}, //scroll bar
    {  .x = 16, .y = 88, .w = 23, .h = 18,	.type=ET_BUTTON}, //prev
    {  .x = 39, .y = 88, .w = 23, .h = 18,	.type=ET_BUTTON}, //play
    {  .x = 62, .y = 88, .w = 23, .h = 18,	.type=ET_BUTTON}, //pause
    {  .x = 85, .y = 88, .w = 23, .h = 18,	.type=ET_BUTTON}, //stop
    {  .x = 108, .y = 88, .w = 22, .h = 18,	.type=ET_BUTTON}, //next
    {  .x = 136, .y = 89, .w = 22, .h = 16,	.type=ET_BUTTON}, //open
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

    int can_drag = 1;

    for (int i=0; i < WE_COUNT; i++) {
	struct element* e = &mw_elements[i];
	if (e->type == ET_LABEL) continue; //skip label elements
	if (hover(e->x, e->y, e->w, e->h)) can_drag = 0;

	int newbs;

	if (hold(e->x,e->y,e->w,e->h,1)) {
	    switch(e->type) {
		case ET_LABEL: newbs = 0; break;
		case ET_BUTTON: newbs = 1; break;
		case ET_HSLIDER: newbs = 1 + getHSliderValue(e); break; 
		case ET_VSLIDER: newbs = 1 + (mouse.Y - e->y); break;
		case ET_MDBUTTON: break;
	    }
	} else newbs = 0;
	e->bs = newbs;
	if (e->bs != e->obs) invalidateXYWH(hWnd,e->x,e->y,e->w,e->h);
    }
    return can_drag;
}

int get_hover_button() {
    for (int i=0; i < WE_COUNT; i++) {
	struct element* e = &mw_elements[i];
	if (e->type == ET_LABEL) continue; //skip label elements
	if (hover(e->x,e->y,e->w,e->h)) return i;
    }
    return -1;
}

int get_click_button() {
    for (int i=0; i < WE_COUNT; i++) {
	struct element* e = &mw_elements[i];
	if (e->type == ET_LABEL) continue; //skip label elements
	if (click(e->x,e->y,e->w,e->h,1)) return i;
    }
    return -1;
}

int handleDoubleClickEvents(HWND hWnd) {
    printf("dblclk %d,%d\n",mouse.X,mouse.Y);

    switch (get_hover_button()) {
	case WE_TITLE:
			exit(0);
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


    return 0;
}

int filePlay(void) {
    if ((ip) && (op->IsPlaying())) ip->Stop();
    if (preparePlugin(filePath)) return 1;
    ip->Play(filePath);
    return 0;
}

int openFile(void) {

    int filter_c = countInputPlugins();
    const char* filter_v[filter_c];
    getInputPluginExtensions(filter_c,filter_v);
    return uiOpenFile(h_mainwin, filter_c, filter_v, filePath, 1024);
}

int openFileAndPlay(void) {

    if (openFile() == 0) return 1;
    return filePlay();
}

int updateScrollbarValue(void) {

    if ((op) && (op->IsPlaying())) {
	int len = ip ? ip->GetLength() : 1;
	int cur = ip ? ip->GetOutputTime() : 0;

	int xpos = (cur * 219) / len;
	if (xpos < 0) xpos = 0;
	if (xpos >= 219) xpos = 219;
	mw_elements[WE_B_SCROLLBAR].value = xpos;
	if (mw_elements[WE_B_SCROLLBAR].bs <= 0) mw_elements[WE_B_SCROLLBAR].bs = -1-xpos;
    } else mw_elements[WE_B_SCROLLBAR].value = -1;
    return 0;
}


int handleClickEvents(HWND hWnd) {

    switch (get_click_button()) {
	case WE_B_MENU: showSystemMenu(hWnd, 0, 6, 12);
			break;
	case WE_B_CLOSE:
			exit(0);
			break;
	case WE_B_PAUSE:
			if (ip) {
			if (ip->IsPaused())
			    ip->UnPause();
			else
			    ip->Pause();
			}
			break;
	case WE_B_STOP:
			if ( (ip) && (op->IsPlaying()) ) ip->Stop();
			break;
	case WE_B_OPEN: 
			if (!openFile()) break;
	case WE_B_PLAY:
			filePlay();
			break;
	case WE_B_SCROLLBAR: {
				 int len = ip ? ip->GetLength() : 0;
				 int px = (mw_elements[WE_B_SCROLLBAR].bs - 1);
				 int wx = mw_elements[WE_B_SCROLLBAR].w - mw_elements[WE_B_SCROLLBAR].slider_w;
				 if (px < 0) px=0; if (px > wx) px = wx;
				 ip->SetOutputTime (len * (px / (double)wx));
				 updateScrollbarValue();
				 break; }
    }
    return 0;
}

void mainWinTimerFunc(HWND hWnd) {
    timercnt++;	

    if ((timercnt % 10) == 0) {
	updateScrollbarValue();
    }

    if (op->IsPlaying()) {
	if ((timercnt % 5) == 0) scrollcnt++; 
	mw_elements[WE_TITLE].bs = 1 + scrollcnt;
	if ( (ip) && (ip->IsPaused()) ) { mw_elements[WE_PLAYPAUS].bs = PS_PAUSE; }
	mw_elements[WE_TIMER].bs = ip ? ((ip->IsPaused() && (timercnt % 20 >= 10)) ? TIMER_BLANK : (ip->GetOutputTime() / 1000)) : TIMER_BLANK;
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
}

void mainWinFocusFunc(HWND hWnd, int focused) {
    mw_elements[WE_TITLEBAR].bs = focused;
    //if (mw_elements[WE_TITLEBAR].bs != mw_elements[WE_TITLEBAR].obs) InvalidateRect(hWnd,&mainTitleRect,0);
}

void mainWinMenuFunc(HWND hWnd, int menuid) {

    switch (menuid) {
	case IDM_I_OPENFILE:
	    openFileAndPlay();
	    break;
	case IDM_I_OPENLOC:
	    break;
	case IDM_I_OPENURL: {
				int r = uiInputBox(hWnd, "Input the URL of the resource to be played:", filePath, 1024);
				if (r) filePlay();
				break; }
	case IDM_I_FILEINFO:
			    if (ip) ip->InfoBox(filePath, hWnd);
			    break;
	case IDM_I_ABOUT:
			    uiOKMessageBox(hWnd, "amptest - https://github.com/usrshare/amptest/", "About amptest", UIMB_INFO);
			    break;
	case IDM_O_INPUTPREF:
			    if (ip) ip->Config(hWnd);
			    break;
	case IDM_O_OUTPUTPREF:
			    if (op) op->Config(hWnd);
			    break;
	case IDM_I_EXIT:
			    exit(0);
			    break;
    }

}

void mainWinPaintFunc(HWND hWnd) {
    int i=0;
    struct element* cur = &mw_elements[0];
    do {
	i = find_element_to_update(WE_COUNT,mw_elements,&cur);

	switch (cur - mw_elements) {
	    case WE_BACKGROUND:
		skinBlit(hWnd, skin.mainbitmap, 0, 0, 0, 0, 0, 0);
		break;
	    case WE_TITLEBAR:
		invalidateXYWH(hWnd,cur->x,cur->y,cur->w,cur->h);
		skinBlit(hWnd, skin.titlebitmap, 27, cur->bs ? 0 : 15, 0, 0, 275, 14);
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
			       if ((!ip) || (cur->bs == TIMER_BLANK)) {
				   skinDrawTimeString(hWnd, "     ", cur->x, cur->y);
			       } else {
				   skinDrawTime(hWnd, ip->GetOutputTime()/1000, cur->x, cur->y);
			       }
			       break; }
	    case WE_TITLE: {

			       char title[128];

			       sprintf(title,"hello world");
			       int titlesz = skinTextLength(title);

			       if ( (ip) && (op->IsPlaying()) ) {

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
				  skinBlit(hWnd, skin.posbar, 0, 0, cur->x, cur->y, 248, 10);

				  if (cur->bs > 0) {
				      int px = getHSliderValue(cur); 
				      skinBlit(hWnd, skin.posbar, 278, 0, cur->x + px, cur->y, 29, 10); 

				  } else {
				      skinBlit(hWnd, skin.posbar, 248, 0, cur->x + mw_elements[WE_B_SCROLLBAR].value, cur->y, 29, 10); 
				  }

			      } else {
				  skinBlit(hWnd, skin.mainbitmap, cur->x, cur->y, cur->x, cur->y, 248, 10); 
			      }
	}
    } while (cur);
    windowBlit(hWnd);
}

int ampInit() {

    op = loadOutputPlugin("plugins/out_wave.dll");
    if (!op) { return 1; }
    op->Init();
    
    gf.SetInfo = UI_SetInfo;
    gf.hMainWindow = h_mainwin;
    
    int inputs_c = scanPlugins("plugins");
    if (!inputs_c) { return 1; }
    
    return 0;
}

int main (int argc, char** argv) {

    initConsole();

    initMainMenu();
    initMainWindow();

    // force the initial draw of all elements.
    for (int i=0; i < WE_COUNT; i++ ) {
	mw_elements[i].bs = 0; mw_elements[i].obs = -1;}
    updateScrollbarValue();

    strcpy(filePath,"demo.mp3");

    ampInit();

    struct windowCallbacks wincb = {
	.focuscb = mainWinFocusFunc,
	.timercb = mainWinTimerFunc,
	.paintcb = mainWinPaintFunc,
	.menucb = mainWinMenuFunc,

	.holdcb = handleHoldEvents,
	.clickcb = handleClickEvents,
	.dblclickcb = handleDoubleClickEvents,
    };

    createMainWindow(&wincb);

    skin.mainbitmap = loadSkinBitmap("skin\\main.bmp");
    skin.titlebitmap = loadSkinBitmap("skin\\titlebar.bmp");
    skin.cbuttons = loadSkinBitmap("skin\\cbuttons.bmp");
    skin.posbar = loadSkinBitmap("skin\\posbar.bmp");
    skin.text = loadSkinBitmap("skin\\text.bmp");
    skin.monoster = loadSkinBitmap("skin\\monoster.bmp");
    skin.playpaus = loadSkinBitmap("skin\\playpaus.bmp");
    skin.nums_ex = loadSkinBitmap("skin\\nums_ex.bmp");

    load_text_layout("skin\\text.txt");

    windowLoop();

    return 0;
}
