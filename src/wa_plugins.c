#include "wa_plugins.h"
#include "win_misc.h"

typedef void* (*voidp_func_void)(void);

void emptyfunc(void) {
	return;
}

int return0(void) {
	return 0;
}

struct waInputPlugin* loadInputPlugin(const char* filename, struct waOutputPlugin* outputPlugin) {

	HMODULE in = LoadLibrary(filename);
	if (!in) { msgerror("load input"); return NULL; }
	
	voidp_func_void in_get = (voidp_func_void) GetProcAddress(in, "winampGetInModule2");
	if (!in_get) { msgerror("getproc input"); return NULL; }

	struct waInputPlugin* ip = in_get();
	
	ip->hDllInstance = in;

	// these functions are not currently implemented by this program.
	// to prevent plugins from crashing, the pointers all point to empty
	// functions that either return nothing or zero.

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
	ip->outMod = outputPlugin;
	return ip;
}

struct waOutputPlugin* loadOutputPlugin(const char* filename) {
	
	HMODULE out = LoadLibrary(filename);
	if (!out) { msgerror("load output"); return NULL; }
	
	voidp_func_void out_get = (voidp_func_void) GetProcAddress(out, "winampGetOutModule");
	if (!out_get) { msgerror("getproc output"); return NULL; }
	
	struct waOutputPlugin* op = out_get();
	
	op->hDllInstance = out;
	return op;
}
