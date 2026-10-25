#ifndef C07_COMPATIBILITY_H
#define C07_COMPATIBILITY_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stddef.h>
/* Does not call game code or write to the image. The reference has no base
 * relocations; production initialization also requires load base 0x400000. */
BOOL C07CheckImage(const BYTE *image, char *reason, size_t reason_size);
#endif
