/* -*- mode: c; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
//
// Copyright (c) 2009, Pal_Bazzi.
//
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspsdk.h>
#include <psppower.h>
#include <pspthreadman.h>
#include <stdlib.h>
#include <stdio.h>

#define PSP_HEAP_MEMSIZE 12288

PSP_MODULE_INFO("SDLPAL", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_HEAP_SIZE_KB(PSP_HEAP_MEMSIZE);

//
//Exit callback
//
int PSPExitCallback(int arg1, int arg2, void *common)
{
	exit(0);
	return 0;
}

//
//Reopen MKF files when resume from suspend
//
int PSPSuspendCallback(int arg1, int pwrflags, void *common)
{
  if (pwrflags & PSP_POWER_CB_RESUME_COMPLETE)
  {
    UTIL_CloseFile(gpGlobals->f.fpFBP);
    UTIL_CloseFile(gpGlobals->f.fpMGO);
    UTIL_CloseFile(gpGlobals->f.fpBALL);
    UTIL_CloseFile(gpGlobals->f.fpDATA);
    UTIL_CloseFile(gpGlobals->f.fpF);
    UTIL_CloseFile(gpGlobals->f.fpFIRE);
    UTIL_CloseFile(gpGlobals->f.fpRGM);
    UTIL_CloseFile(gpGlobals->f.fpSSS);
    gpGlobals->f.fpFBP = UTIL_OpenRequiredFile("fbp.mkf");
    gpGlobals->f.fpDATA = UTIL_OpenRequiredFile("data.mkf");
    gpGlobals->f.fpFIRE = UTIL_OpenRequiredFile("fire.mkf");
    gpGlobals->f.fpSSS = UTIL_OpenRequiredFile("sss.mkf");
    gpGlobals->lpObjectDesc = PAL_LoadObjectDesc(va("%s%s", PAL_PREFIX, "desc.dat"));
    SOUND_ReloadVOC();
  }
  int cbid;
  cbid = sceKernelCreateCallback("suspend Callback", PSPSuspendCallback, NULL);
	scePowerRegisterCallback(0, cbid);
  return 0;
}

//
//setup callbacks thread
//
int PSPRegisterCallbackThread(SceSize args, void *argp)
{
	int cbid;
	cbid = sceKernelCreateCallback("Exit Callback", PSPExitCallback, NULL);
	sceKernelRegisterExitCallback(cbid);
	cbid = sceKernelCreateCallback("suspend Callback", PSPSuspendCallback, NULL);
	scePowerRegisterCallback(0, cbid);
	sceKernelSleepThreadCB();
	return 0;
}

//
//setup exit callback
//
int PSPSetupCallbacks(void)
{
	int thid = 0;
	thid = sceKernelCreateThread("update_thread", PSPRegisterCallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
		sceKernelStartThread(thid, 0, 0);
	return thid;
}

//
//Init on PSP
//
void sdlpal_psp_init(void)
{
   // Init Debug Screen
   pspDebugScreenInit();

   // PSP set callbacks
   PSPSetupCallbacks();

   // Register sceKernelExitGame() to be called when we exit 
   atexit(sceKernelExitGame);

   // set PSP CPU clock
   scePowerSetClockFrequency(333 , 333 , 166);
}
