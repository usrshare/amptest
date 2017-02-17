#include <windows.h>
#include <stdio.h>
#include "wa_plugins.h"
#include "win_misc.h"

typedef void* (*voidp_func_void)(void);

struct waInputPlugin* ip = NULL;
struct waOutputPlugin* op = NULL;

struct globalFunctions gf;

struct waInputPlugin** inputs_v = NULL;
int inputs_c = 0;

void emptyfunc(void) {
	return;
}

int return0(void) {
	return 0;
}

struct waInputPlugin* loadInputPlugin(const char* filename) {

	HMODULE in = LoadLibrary(filename);
	if (!in) { msgerror("load input"); return NULL; }
	
	voidp_func_void in_get = (voidp_func_void) GetProcAddress(in, "winampGetInModule2");
	if (!in_get) { return NULL; }

	struct waInputPlugin* newIn = in_get();
	
	newIn->hDllInstance = in;

	// these functions are not currently implemented by this program.
	// to prevent plugins from crashing, the pointers all point to empty
	// functions that either return nothing or zero.

	newIn->SAGetMode = return0;
	newIn->SAAdd = emptyfunc;
	newIn->SAAddPCMData = emptyfunc;
	newIn->VSAAddPCMData = emptyfunc;
	newIn->SAVSAInit = emptyfunc;
	newIn->SAVSADeInit = emptyfunc;
	newIn->VSAGetMode = emptyfunc;
	newIn->VSAAdd = emptyfunc;
	newIn->VSASetInfo = emptyfunc;
	newIn->dsp_isactive = return0;
	newIn->dsp_dosamples = emptyfunc;
	newIn->SetInfo = emptyfunc;
	newIn->outMod = op;

	newIn->SetInfo = gf.SetInfo;
	newIn->hMainWindow = gf.hMainWindow;
	newIn->Init();
	
	printf("Loaded %s (%s).\n",filename, newIn->description);

	return newIn;
}

int loadNewInputPlugin(const char* filename) {

	inputs_v = realloc(inputs_v, (sizeof inputs_v[0]) * (inputs_c + 1) );
	
	inputs_v[inputs_c] = loadInputPlugin(filename);
	if (inputs_v[inputs_c] != NULL) { inputs_c++; return 0; }

	return 1;
}

int scanPlugins(const char* directory) {

	WIN32_FIND_DATA fd;

	int cl = strlen(directory) + strlen("\\*.dll") + 1;
	char filemask[cl];
	strcpy(filemask,directory);
	strcat(filemask,"\\*.dll");

	HANDLE hFind = FindFirstFile(filemask, &fd);

	if (hFind == INVALID_HANDLE_VALUE) { msgerror("FindFirstFile"); return 1; }

	do {
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

		printf("Found a library at %s\n",fd.cFileName);

		int c2l = strlen(directory) + 1 + strlen(fd.cFileName) + 1;
		char fullpath[c2l];
		sprintf(fullpath,"%s\\%s",directory,fd.cFileName);

		loadNewInputPlugin(fullpath);

	} while (FindNextFile(hFind, &fd) != 0);
	return inputs_c;	
}

const char* nextString(const char* string) {
	if (!string) return NULL;
	const char* r = strchr(string,0) + 1;
	if (*r) return r; else return NULL;
}

int checkExtensions(const char* media_fn, const char* extensions) {
	const char* curext = extensions;
	
	char* media_ext = strrchr(media_fn,'.');
	if (!media_ext) return 0; else media_ext++;

	while (curext) {

	int ext_sz = strlen(curext) + 1;
	char ext_tmp[ext_sz]; strcpy(ext_tmp, curext);

	char* saveptr = NULL;
	char* ext_tok = strtok_r(ext_tmp,";",&saveptr);

	while (ext_tok) {
		if (strcasecmp(media_ext,ext_tok) == 0) { return 1;}
		ext_tok = strtok_r(NULL,";",&saveptr);
	}

	curext = nextString(nextString(curext)); //skip the descriptions
	}
	return 0;
}

int preparePlugin(const char* media_fn) {
	for (int i=0; i < inputs_c; i++) {
		int r = 0;
		r = checkExtensions(media_fn, inputs_v[i]->FileExtensions);
		if (!r) r = inputs_v[i]->IsOurFile(media_fn);
		if (r) {
			if (ip != inputs_v[i]) {
			}
			ip = inputs_v[i];
			//printf("Plugin: %s\n", ip->description);
			return 0;
		}
	}	
	return 1;
}

struct waOutputPlugin* loadOutputPlugin(const char* filename) {
	
	HMODULE out = LoadLibrary(filename);
	if (!out) { msgerror("load output"); return NULL; }
	
	voidp_func_void out_get = (voidp_func_void) GetProcAddress(out, "winampGetOutModule");
	if (!out_get) { msgerror("getproc output"); return NULL; }
	
	struct waOutputPlugin* newOut = out_get();
	
	newOut->hDllInstance = out;

	for (int i = 0; i < inputs_c; i++)
		inputs_v[i]->outMod = newOut;

	op = newOut;
	return newOut;
}

int countInputPlugins(void) { return inputs_c; }

int getInputPluginExtensions(int types_c, const char** types_v) {
	for (int i=0; i < types_c; i++)
		types_v[i] = inputs_v[i]->FileExtensions;

	return 0;
}
