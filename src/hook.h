#ifndef C07_LINEUP_HOOK_H
#define C07_LINEUP_HOOK_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define C07_LINEUP_RVA 0x00017330u
#define C07_COOKIE_RVA 0x00415e2cu
#define C07_HOOK_LENGTH 8u
BOOL C07InstallHook(BYTE *image, const void *replacement, void **original);
#endif
