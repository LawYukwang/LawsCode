// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

extern void module_init(void);
extern void module_exit(void);
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		module_init();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		module_exit();
		break;
	}
	return TRUE;
}

