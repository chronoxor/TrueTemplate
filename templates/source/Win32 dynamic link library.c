/*==========================================================================*/
/*
@@INCLUDE:$\templates\source\header-base@@
*/
/*--------------------------------------------------------------------------*/
/*
@@INCLUDE:$\templates\source\header-gnu@@
*/
/*--------------------------------------------------------------------------*/
/*
@@INCLUDE:$\templates\source\header-cvs@@
*/
/*==========================================================================*/
#ifndef __@@UPPERFILE_NAME@@_@@UPPERFILE_EXT@@__
#define __@@UPPERFILE_NAME@@_@@UPPERFILE_EXT@@__
/*==========================================================================*/
#include <windows.h>
/*==========================================================================*/
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	@@HERE@@// Perform actions based on the reason for calling
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			// Initialize once for each new process.
			// Return FALSE to fail DLL load
			break;
		}
		case DLL_THREAD_ATTACH:
		{
			// Do thread-specific initialization
			break;
		}
		case DLL_THREAD_DETACH:
		{
			// Do thread-specific cleanup
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			// Perform any necessary cleanup
			break;
		}
	}
	// Successful DLL_PROCESS_ATTACH
	return TRUE;
}
/*==========================================================================*/
#endif
