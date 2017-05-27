//
//  aviplay.h
//  Pal
//
//  Created by Wei Mingzhi on 5/26/17.
//
//

#ifndef AVIPLAY_H
#define AVIPLAY_H

#include "common.h"

PAL_C_LINKAGE_BEGIN

VOID
PAL_AVIInit(
    VOID
);

VOID
PAL_AVIShutdown(
    VOID
);

BOOL
PAL_PlayAVI(
    LPCSTR     lpszPath
);

VOID SDLCALL
AVI_FillAudioBuffer(
    LPVOID          udata,
    LPBYTE          stream,
    INT             len
);

PAL_C_LINKAGE_END

#endif
