#ifndef INPOUT32_DYN_H
#define INPOUT32_DYN_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 导出函数声明（与 DLL 原始声明一致） */
void    __stdcall Out32(short PortAddress, short data);
short   __stdcall Inp32(short PortAddress);
BOOL    __stdcall IsInpOutDriverOpen(void);
BOOL    __stdcall IsXP64Bit(void);

#ifdef __cplusplus
}
#endif

#endif /* INPOUT32_DYN_H */