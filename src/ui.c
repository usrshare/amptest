// vim: cin:sts=4:sw=4
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "ui.h"
#include "win_misc.h"
#include "menus.h"

HWND h_mainwin;

HMENU h_mainmenu;

struct uiWindow {
    HWND v;
};

struct mouseData mouse = {.X = -1, .Y = -1, .clickX = -1, .clickY = -1, .buttons = 0};

struct windowData {

    //this struct is allocated for each instance of a window and pointed to with
    //set/getWindowData(). any function that receives an hWnd can get the pointer
    //to the structure.

    HBITMAP hbmpBuf; // bitmap for double buffering
    HDC hdcMemBuf; //hdcmem for dbl buffering

    PAINTSTRUCT ps;
    HDC hdc; //hdc for the window itself
    HDC hdcMem; //hdcMem for the window itself
    struct windowCallbacks cb;
};

struct windowData* getWindowData(HWND hWnd) {
    return (struct windowData*) GetWindowLongPtr(hWnd,0);
}

HBITMAP loadOptSkinBitmap (const char* filename) {
    HBITMAP r = LoadImage(NULL, filename, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
    return r;
}

HBITMAP loadSkinBitmap (const char* filename) {
    HBITMAP r = LoadImage(NULL, filename, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
    if (r == NULL) {
	msgerror(filename);
    }
    return r;
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

int invalidateXYWH(HWND hWnd, unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
    CONST RECT r = {.top = y, .left = x, .bottom = y+h, .right = x+w};
    return InvalidateRect(hWnd,&r,0);
}

int showSystemMenu(HWND hWnd, int submenu, int x, int y) {
    POINT mp = {.x = x, .y = y};
    MapWindowPoints(hWnd, NULL, &mp, 1);
    HMENU h_sysmenu = GetSubMenu(h_mainmenu, submenu);
    TrackPopupMenuEx(h_sysmenu,TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON,mp.x,mp.y,hWnd,NULL);
    return 0;
}

#ifndef GET_X_PARAM
int GET_X_PARAM(LPARAM lParam) { return (int)(lParam & 0xFFFF); }
int GET_Y_PARAM(LPARAM lParam) { return (int)(lParam >> 16); }
#endif

LRESULT WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    struct windowData* wdata = getWindowData(hWnd);
    switch(uMsg) {
	case WM_SETFOCUS:
	case WM_KILLFOCUS: {

			       if (wdata->cb.focuscb) wdata->cb.focuscb(hWnd, uMsg == WM_SETFOCUS);

			       return DefWindowProc(hWnd,uMsg,wParam,lParam); 
			       break;} 
	case WM_CREATE: {
			    //create the struct and store it in the pointer provided.
			    SetWindowLongPtr(hWnd,0,(LONG_PTR)(((LPCREATESTRUCT)lParam)->lpCreateParams) );
			    skinInitializePaint(hWnd);
			    return 0;
			    break; }
	case WM_COMMAND: {
			     int menuid = LOWORD(wParam);
			     if (HIWORD(wParam) == 0) {
				 if (wdata->cb.menucb) wdata->cb.menucb(hWnd, menuid);
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
			 if (wdata->cb.timercb) wdata->cb.timercb(hWnd);

			 break;
	case WM_NCHITTEST: {
			       POINT mp = {GET_X_PARAM(lParam),GET_Y_PARAM(lParam) };
			       MapWindowPoints(NULL,hWnd,&mp,1);
			       mouse.hWnd = hWnd;
			       mouse.X = mp.x; mouse.Y = mp.y;
			       int can_drag = 0;
			       if (wdata->cb.holdcb) can_drag = wdata->cb.holdcb(hWnd);
			       LRESULT r = DefWindowProc(hWnd,uMsg,wParam,lParam);
			       if ((can_drag) && (r == HTCLIENT)) r = HTCAPTION;
			       return r; } 
	case WM_LBUTTONDOWN:
			   //record press location for "pushed down" buttons.
			   mouse.hWnd = hWnd;
			   mouse.clickX = LOWORD(lParam);
			   mouse.clickY = HIWORD(lParam);
			   mouse.buttons |= 1;
			   if (wdata->cb.holdcb) wdata->cb.holdcb(hWnd);
			   return DefWindowProc(hWnd,uMsg,wParam,lParam);
	case WM_LBUTTONUP:
			   mouse.hWnd = hWnd;
			   mouse.X = LOWORD(lParam);
			   mouse.Y = HIWORD(lParam);
			   //handle all the "click" events here.
			   if (wdata->cb.clickcb) wdata->cb.clickcb(hWnd);
			   mouse.buttons &= (~1);
			   if (wdata->cb.holdcb) wdata->cb.holdcb(hWnd);
			   return DefWindowProc(hWnd,uMsg,wParam,lParam);
	case WM_LBUTTONDBLCLK:
			   mouse.hWnd = hWnd;
			   mouse.X = LOWORD(lParam);
			   mouse.Y = HIWORD(lParam);
			   if (wdata->cb.dblclickcb) wdata->cb.dblclickcb(hWnd);
			   mouse.buttons &= (~1);
			   return DefWindowProc(hWnd,uMsg,wParam,lParam);
	case WM_MOUSEMOVE:
			   mouse.hWnd = hWnd;
			   mouse.X = LOWORD(lParam);
			   mouse.Y = HIWORD(lParam);
			   if (wdata->cb.holdcb) wdata->cb.holdcb(hWnd);
			   return DefWindowProc(hWnd,uMsg,wParam,lParam);
	case WM_MOUSELEAVE:
			   mouse.X = -1;
			   mouse.Y = -1;
			   mouse.clickX = -1;
			   mouse.clickY = -1;
			   if (wdata->cb.holdcb) wdata->cb.holdcb(hWnd);
	case WM_PAINT: {
			   if (wdata->cb.paintcb) wdata->cb.paintcb(hWnd);
			   break;
		       }
	default:
		       return DefWindowProc(hWnd,uMsg,wParam,lParam);
    }
    return 0;
}

WNDCLASS mainwin = {0,(WNDPROC)WindowProc,0,sizeof (void*),NULL,NULL,NULL,(HBRUSH) COLOR_BTNFACE+1,NULL,"helloMain"};

int initMainMenu (void) {
    h_mainmenu = LoadMenu(mainwin.hInstance, MAKEINTRESOURCE(IDM_MAINMENU) );
    return 0;
}

int initMainWindow (void) {

    mainwin.hInstance = GetModuleHandle(0);
    mainwin.hCursor = LoadCursor(0,IDC_ARROW);

    RegisterClass(&mainwin);
    InitCommonControls();

    return 0;
}

int createMainWindow(struct windowCallbacks* wincb) {

    struct windowData* wd = malloc(sizeof(struct windowData));
    memset(wd, 0, sizeof *wd);
    memcpy(&(wd->cb), wincb, sizeof (struct windowCallbacks));

    h_mainwin = CreateWindowEx(0, "helloMain", "hello world", WS_VISIBLE | WS_POPUP, 128, 128, 275, 116, NULL, NULL, mainwin.hInstance, wd);

    SetTimer(h_mainwin,0,50,NULL);
    return 0; 
}

int windowLoop(void) {

    //this is where WindowProc will start being called.

    MSG lastmsg;
    while (GetMessage(&lastmsg,NULL,0,0)) {
	TranslateMessage(&lastmsg);
	DispatchMessage(&lastmsg);
    }
    return 0;
}


UINT winMsgTypes[UIMB_COUNT] = { MB_ICONINFORMATION, MB_ICONWARNING, MB_ICONERROR };

int uiOKMessageBox(HWND parenthWnd, const char* text, const char* title, int type) {

    return MessageBoxA(parenthWnd, text, title, winMsgTypes[type]);
}

struct uiInputBoxParams {
    const char* prompt;
    char* rstring;
    size_t rstrSz;
};

INT_PTR CALLBACK InputBoxProc( HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
    switch (uMsg) 
    { 
	case WM_INITDIALOG:
	    SetWindowLongPtr(hWndDlg,GWLP_USERDATA,lParam);
	    struct uiInputBoxParams* p = (void*) lParam;
	    SetDlgItemText(hWndDlg, IDD_PROMPT, p->prompt);
	    return TRUE;
	case WM_COMMAND: 
	    switch (LOWORD(wParam)) 
	    { 
		case IDOK: {

		    struct uiInputBoxParams* p = (struct uiInputBoxParams*) GetWindowLongPtr(hWndDlg,GWLP_USERDATA);
		    if (!GetDlgItemText(hWndDlg, IDD_INPUT, p->rstring, p->rstrSz)) 
			p->rstring[0]=0; //if failed to receive filePath, set it to empty string
		    // Fall through. 
			   }
		case IDCANCEL: 
		    EndDialog(hWndDlg, wParam); 
		    return TRUE; 
	    } 
    } 
    return FALSE;    
}

int uiInputBox(HWND hWnd, const char* prompt, char* rstring, size_t rstrSz) {

    struct uiInputBoxParams p = {.prompt = prompt, .rstring = rstring, .rstrSz = rstrSz};

    DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_INPUTBOX), hWnd, InputBoxProc, (LPARAM)&p);
    return 0;
}

void stardotify(const char* extensions, char* o_extensions) {
    //for size reasons, make sure the o_extensions buffer is at least twice as large as the input one, incl. the zero terminator.
    //this is enough to handle the worst case scenario, i guarantee.

    int sl = strlen(extensions) + 1;
    char c_ext [sl]; strcpy(c_ext,extensions);

    char* saveptr = NULL;
    char* cur = strtok_r(c_ext,";",&saveptr);
    while (cur) {
	strcat(o_extensions,"*.");
	strcat(o_extensions,cur);

	cur = strtok_r(NULL,";",&saveptr);
	if (cur) strcat(o_extensions,";");
    }
    free(c_ext);
    return;
}

/*
void print_until_dbl_zero(const char* str) {

    const char* cur = str;
    while (*cur != 0) {
	fputs(cur,stdout);
	cur += strlen(cur);

	if (cur[1] != 0) { fputs("\e[31m.\e[0m",stdout); cur++;}
    }
    puts("");
}
*/

char* concat_winamp_file_formats(const char* str, char* o_allfiles, unsigned int o_sz) {
    
    char* all_cur = o_allfiles;
    unsigned int r_szleft = o_sz-2;

    const char* extensions = str; const char* description = strchr(str, 0) + 1;

    int i=0;
    while (extensions[0] != 0) {

	int el = (strlen(extensions)+1)*2;
	char o_ext[el]; o_ext[0] = 0;

	stardotify(extensions, o_ext);

	int r = snprintf(all_cur,r_szleft,"%s%s",o_ext, *(strchr(description,0)+1) ? ";" : "" );
	if (r > r_szleft) return NULL;
	all_cur += r;
	r_szleft -= r;

	extensions = strchr(description, 0) + 1;
	description = strchr(extensions, 0) + 1;
	i++;
    }
    return all_cur;
}

char* parse_winamp_file_formats(const char* str, char* o_filters, unsigned int o_sz) {

//this function must be used only on strings where the programmer knows there's a double zero at the end.

    char* res_cur = o_filters;
    unsigned int r_szleft = o_sz-2;

    const char* extensions = str; const char* description = strchr(str, 0) + 1;

    int i=0;
    while (extensions[0] != 0) {

	int el = (strlen(extensions)+1)*2;
	char o_ext[el]; o_ext[0] = 0;

	stardotify(extensions, o_ext);

	int r = snprintf(res_cur,r_szleft,"%s%c%s%c",description, 0, o_ext, 0);
	if (r > r_szleft) return NULL;
	res_cur += r;
	r_szleft -= r;

	extensions = strchr(description, 0) + 1;
	description = strchr(extensions, 0) + 1;
	i++;
    }
    return res_cur;
}

int uiOpenFile(HWND hWnd, unsigned int types_c, const char** types_v, char* out_file, unsigned int out_sz) {

    // -- this huge piece of code below concatenates all the filter strings into one.

    #define RES_SIZE 65536

    char res_total[RES_SIZE]; //if that's not enough, ease up on the plugins!
    char* res_last = res_total;
   
    int r = sprintf(res_total,"All Supported Files%c",0);
    res_last += r;

    for (int i=0; i< types_c; i++) {
	if (res_last) res_last = concat_winamp_file_formats(types_v[i],res_last,RES_SIZE - (res_last - res_total));
	res_last[0] = (i < (types_c-1)) ? ';' : 0; res_last++;
    }
    for (int i=0; i< types_c; i++) {
	if (res_last) res_last = parse_winamp_file_formats(types_v[i],res_last,RES_SIZE - (res_last - res_total));
    }
    res_last[0] = 0; res_last++;
    //print_until_dbl_zero(res_total);

    // -- finally!
    
    OPENFILENAME ofn = {
	.lStructSize = sizeof ofn,
	.hwndOwner = h_mainwin,
	.hInstance = NULL,
	.lpstrFilter = res_total,
	.lpstrCustomFilter = NULL,
	.nMaxCustFilter = 0,
	.nFilterIndex = 1,
	.lpstrFile = out_file,
	.nMaxFile = out_sz,
    };
    r = GetOpenFileName(&ofn);
    return r;
}

void initConsole(void) {
    SetConsoleOutputCP(65001); //this program still outputs everything as UTF-8, even when running in Windows.
}

