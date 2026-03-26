#ifndef INPOUT32_DYN_H
#define INPOUT32_DYN_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

void    __stdcall Out32(short PortAddress, short data);
short   __stdcall Inp32(short PortAddress);
BOOL    __stdcall IsInpOutDriverOpen(void);
BOOL    __stdcall IsXP64Bit(void);

#ifdef INPOUT32_DYN_IMPLEMENTATION
#ifdef _WIN64
#define INPOUT_DLL_NAME "inpoutx64.dll"
#else
#define INPOUT_DLL_NAME "inpout32.dll"
#endif

typedef void(__stdcall* Out32Func)(short, short);
typedef short(__stdcall* Inp32Func)(short);
typedef BOOL(__stdcall* IsOpenFunc)(void);
typedef BOOL(__stdcall* Is64Func)(void);

static HMODULE  hInpOut = NULL;
static Out32Func fpOut32 = NULL;
static Inp32Func fpInp32 = NULL;
static IsOpenFunc fpIsOpen = NULL;
static Is64Func   fpIs64 = NULL;

static BOOL EnsureLoaded(void)
{
	if (hInpOut != NULL) {
		return (hInpOut != (HMODULE)-1);
	}

	hInpOut = LoadLibraryA(INPOUT_DLL_NAME);
	if (hInpOut == NULL) {
		hInpOut = (HMODULE)-1;
		return FALSE;
	}

	fpOut32 = (Out32Func)GetProcAddress(hInpOut, "Out32");
	fpInp32 = (Inp32Func)GetProcAddress(hInpOut, "Inp32");
	fpIsOpen = (IsOpenFunc)GetProcAddress(hInpOut, "IsInpOutDriverOpen");
	fpIs64 = (Is64Func)GetProcAddress(hInpOut, "IsXP64Bit");

	return TRUE;
}

extern "C" void __stdcall Out32(short PortAddress, short data)
{
	if (!EnsureLoaded() || fpOut32 == NULL)
		return;
	fpOut32(PortAddress, data);
}

extern "C" short __stdcall Inp32(short PortAddress)
{
	if (!EnsureLoaded() || fpInp32 == NULL)
		return 0;
	return fpInp32(PortAddress);
}

extern "C" BOOL __stdcall IsInpOutDriverOpen(void)
{
	if (!EnsureLoaded() || fpIsOpen == NULL)
		return FALSE;
	return fpIsOpen();
}

extern "C" BOOL __stdcall IsXP64Bit(void)
{
	if (!EnsureLoaded() || fpIs64 == NULL)
		return FALSE;
	return fpIs64();
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* INPOUT32_DYN_H */