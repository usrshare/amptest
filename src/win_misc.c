#include "win_misc.h"
#include <windows.h>
#include <stdio.h>

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
