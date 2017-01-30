#include <windows.h>
#include <commctrl.h>

#include <stdio.h>

struct waGeneralPlugin {
	int version;
	char* description;
	int (*init)();
	void (*config)();
	void (*quit)();
	HWND hwndParent;
	HINSTANCE hDllInstance;
};

//taken from the Winamp SDK
struct waOutputPlugin {
	int version;				// module version (OUT_VER)
	char *description;			// description of module, with version string
	intptr_t id;						// module id. each input module gets its own. non-nullsoft modules should
								// be >= 65536. 

	HWND hMainWindow;			// winamp's main window (filled in by winamp)
	HINSTANCE hDllInstance;		// DLL instance handle (filled in by winamp)

	void (*Config)(HWND hwndParent); // configuration dialog 
	void (*About)(HWND hwndParent);  // about dialog

	void (*Init)();				// called when loaded
	void (*Quit)();				// called when unloaded

	int (*Open)(int samplerate, int numchannels, int bitspersamp, int bufferlenms, int prebufferms); 
					// returns >=0 on success, <0 on failure

					// NOTENOTENOTE: bufferlenms and prebufferms are ignored in most if not all output plug-ins. 
					//    ... so don't expect the max latency returned to be what you asked for.
					// returns max latency in ms (0 for diskwriters, etc)
					// bufferlenms and prebufferms must be in ms. 0 to use defaults. 
					// prebufferms must be <= bufferlenms
					// pass bufferlenms==-666 to tell the output plugin that it's clock is going to be used to sync video
					//   out_ds turns off silence-eating when -666 is passed

	void (*Close)();	// close the ol' output device.

	int (*Write)(char *buf, int len);	
					// 0 on success. Len == bytes to write (<= 8192 always). buf is straight audio data. 
					// 1 returns not able to write (yet). Non-blocking, always.

	int (*CanWrite)();	// returns number of bytes possible to write at a given time. 
						// Never will decrease unless you call Write (or Close, heh)

	int (*IsPlaying)(); // non0 if output is still going or if data in buffers waiting to be
						// written (i.e. closing while IsPlaying() returns 1 would truncate the song

	int (*Pause)(int pause); // returns previous pause state

	void (*SetVolume)(int volume); // volume is 0-255
	void (*SetPan)(int pan); // pan is -128 to 128

	void (*Flush)(int t);	// flushes buffers and restarts output at time t (in ms) 
							// (used for seeking)

	int (*GetOutputTime)(); // returns played time in MS
	int (*GetWrittenTime)(); // returns time written in MS (used for synching up vis stuff)

};

//taken from the Winamp SDK
struct waInputPlugin {
	int version;				// module type (IN_VER)
	char *description;			// description of module, with version string

	HWND hMainWindow;			// winamp's main window (filled in by winamp)
	HINSTANCE hDllInstance;		// DLL instance handle (Also filled in by winamp)

	char *FileExtensions;		// "mp3\0Layer 3 MPEG\0mp2\0Layer 2 MPEG\0mpg\0Layer 1 MPEG\0"
								// May be altered from Config, so the user can select what they want
	
	int is_seekable;			// is this stream seekable? 
	int UsesOutputPlug;			// does this plug-in use the output plug-ins? (musn't ever change, ever :)
													// note that this has turned into a "flags" field
													// see IN_MODULE_FLAG_*

	void (*Config)(HWND hwndParent); // configuration dialog
	void (*About)(HWND hwndParent);  // about dialog

	void (*Init)();				// called at program init
	void (*Quit)();				// called at program quit

#define GETFILEINFO_TITLE_LENGTH 2048 
	void (*GetFileInfo)(const char *file, char *title, int *length_in_ms); // if file == NULL, current playing is used

#define INFOBOX_EDITED 0
#define INFOBOX_UNCHANGED 1
	int (*InfoBox)(const char *file, HWND hwndParent);
	
	int (*IsOurFile)(const char *fn);	// called before extension checks, to allow detection of mms://, etc
	// playback stuff
	int (*Play)(const char *fn);		// return zero on success, -1 on file-not-found, some other value on other (stopping winamp) error
	void (*Pause)();			// pause stream
	void (*UnPause)();			// unpause stream
	int (*IsPaused)();			// ispaused? return 1 if paused, 0 if not
	void (*Stop)();				// stop (unload) stream

	// time stuff
	int (*GetLength)();			// get length in ms
	int (*GetOutputTime)();		// returns current output time in ms. (usually returns outMod->GetOutputTime()
	void (*SetOutputTime)(int time_in_ms);	// seeks to point in stream (in ms). Usually you signal your thread to seek, which seeks and calls outMod->Flush()..

	// volume stuff
	void (*SetVolume)(int volume);	// from 0 to 255.. usually just call outMod->SetVolume
	void (*SetPan)(int pan);	// from -127 to 127.. usually just call outMod->SetPan
	
	// in-window builtin vis stuff

	void (*SAVSAInit)(int maxlatency_in_ms, int srate);		// call once in Play(). maxlatency_in_ms should be the value returned from outMod->Open()
	// call after opening audio device with max latency in ms and samplerate
	void (*SAVSADeInit)();	// call in Stop()


	// simple vis supplying mode
	void (*SAAddPCMData)(void *PCMData, int nch, int bps, int timestamp); 
											// sets the spec data directly from PCM data
											// quick and easy way to get vis working :)
											// needs at least 576 samples :)

	// advanced vis supplying mode, only use if you're cool. Use SAAddPCMData for most stuff.
	int (*SAGetMode)();		// gets csa (the current type (4=ws,2=osc,1=spec))
							// use when calling SAAdd()
	int (*SAAdd)(void *data, int timestamp, int csa); // sets the spec data, filled in by winamp


	// vis stuff (plug-in)
	// simple vis supplying mode
	void (*VSAAddPCMData)(void *PCMData, int nch, int bps, int timestamp); // sets the vis data directly from PCM data
											// quick and easy way to get vis working :)
											// needs at least 576 samples :)

	// advanced vis supplying mode, only use if you're cool. Use VSAAddPCMData for most stuff.
	int (*VSAGetMode)(int *specNch, int *waveNch); // use to figure out what to give to VSAAdd
	int (*VSAAdd)(void *data, int timestamp); // filled in by winamp, called by plug-in


	// call this in Play() to tell the vis plug-ins the current output params. 
	void (*VSASetInfo)(int srate, int nch); // <-- Correct (benski, dec 2005).. old declaration had the params backwards


	// dsp plug-in processing: 
	// (filled in by winamp, calld by input plug)

	// returns 1 if active (which means that the number of samples returned by dsp_dosamples
	// could be greater than went in.. Use it to estimate if you'll have enough room in the
	// output buffer
	int (*dsp_isactive)(); 

	// returns number of samples to output. This can be as much as twice numsamples. 
	// be sure to allocate enough buffer for samples, then.
	int (*dsp_dosamples)(short int *samples, int numsamples, int bps, int nch, int srate);


	// eq stuff
	void (*EQSet)(int on, char data[10], int preamp); // 0-64 each, 31 is +0, 0 is +12, 63 is -12. Do nothing to ignore.

	// info setting (filled in by winamp)
	void (*SetInfo)(int bitrate, int srate, int stereo, int synched); // if -1, changes ignored? :)

	struct waOutputPlugin *outMod; // filled in by winamp, optionally used :)
};

int msgerror(char* prefix) {

	DWORD errcode = GetLastError();
	char errortxt[256];

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,errcode,0,errortxt,256,NULL);
	char msgtxt[1024];
	if (prefix) {
		snprintf(msgtxt,1024,"%s: %s (%d)",prefix,errortxt,errcode);
	} else {
		snprintf(msgtxt,1024,"Error %d: %s",errcode,errortxt);
	}
	MessageBoxA(0, msgtxt, "Error", MB_OK);
	return 0;
}

typedef void* (*voidp_func_void)(void);

void emptyfunc(void) {
	return;
}

int return0(void) {
	return 0;
}

struct waInputPlugin* ip = NULL;
struct waOutputPlugin* op = NULL;

LRESULT WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch(uMsg) {
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
		default:
			return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
}

WNDCLASS mainwin = {0,WindowProc,0,0,NULL,NULL,NULL,COLOR_BTNFACE+1,NULL,"helloMain"};

HWND mainwhnd;


int ampInit() {

	HMODULE in = LoadLibrary("plugins/in_mp3.dll");
	if (!in) { msgerror("load input"); return 1; }
	HMODULE out = LoadLibrary("plugins/out_wave.dll");
	if (!out) { msgerror("load input"); return 1; }

	voidp_func_void in_get = (voidp_func_void) GetProcAddress(in, "winampGetInModule2");
	if (!in_get) { msgerror("getproc input"); return 1; }

	ip = in_get();

	voidp_func_void out_get = (voidp_func_void) GetProcAddress(out, "winampGetOutModule");
	if (!out_get) { msgerror("getproc output"); return 1; }
	
	op = out_get();

	op->hMainWindow = mainwhnd;
	op->hDllInstance = out;
	op->Init();

	ip->hMainWindow = mainwhnd;
	ip->hDllInstance = in;
	ip->SAGetMode = emptyfunc;
	ip->SAAdd = emptyfunc;
	ip->SAAddPCMData = emptyfunc;
	ip->VSAAddPCMData = emptyfunc;
	ip->SAVSAInit = emptyfunc;
	ip->SAVSADeInit = emptyfunc;
	ip->VSAGetMode = emptyfunc;
	ip->VSAAdd = emptyfunc;
	ip->VSASetInfo = emptyfunc;
	ip->dsp_isactive = return0;
	ip->dsp_dosamples = emptyfunc;
	ip->SetInfo = emptyfunc;
	ip->outMod = op;
	ip->Init();
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

	mainwin.hInstance = GetModuleHandle(0);
	mainwin.hCursor = LoadCursor(0,IDC_ARROW);
	
	RegisterClass(&mainwin);
	InitCommonControls();

	mainwhnd = CreateWindowEx(0, "helloMain", "hello world", WS_VISIBLE | WS_POPUP, 128, 128, 275, 116, NULL, NULL, mainwin.hInstance, NULL);
	//HANDLE mainimg = LoadImage(NULL, "skin/main.bmp", IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
	//if (mainimg) SendMessage( mainwhnd, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)mainimg); else msgerror("load bitmap");

	CreateWindow("BUTTON","Play",WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 16,96,24,16, mainwhnd, (HMENU)1, mainwin.hInstance, NULL);


	ampInit();

	MSG lastmsg;
	while (GetMessage(&lastmsg,NULL,0,0)) {
		TranslateMessage(&lastmsg);
		DispatchMessage(&lastmsg);
	}


	MessageBoxA(0, "hello world", "test", 0);
	return 0;
}
