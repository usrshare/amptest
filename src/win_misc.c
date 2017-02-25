#include "win_misc.h"
#include <windows.h>
#include <stdio.h>

int msgerror(const char* prefix) {

	DWORD errcode = GetLastError();
	char errortxt[256];

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS ,NULL,errcode,0,errortxt,256,NULL);
	char msgtxt[1024];
	if (prefix) {
		snprintf(msgtxt,1024,"%s: %s (%u)",prefix,errortxt,errcode);
	} else {
		snprintf(msgtxt,1024,"Error %lu: %s",errcode,errortxt);
	}
	MessageBoxA(0, msgtxt, "Error", MB_OK);
	return 0;
}
