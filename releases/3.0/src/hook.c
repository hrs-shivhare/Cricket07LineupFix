#include "hook.h"
#include <stdint.h>
#include <string.h>

static void relative_jump(BYTE *at, const void *to) {
    uint32_t delta = (uint32_t)(uintptr_t)to - ((uint32_t)(uintptr_t)at + 5u);
    at[0] = 0xe9;
    memcpy(at + 1, &delta, sizeof(delta));
}

BOOL C07InstallHook(BYTE *image, const void *replacement, void **original) {
    BYTE expected[8] = {0x83,0xec,0x1c,0xa1,0,0,0,0};
    BYTE patch[8] = {0xe9,0,0,0,0,0x90,0x90,0x90};
    BYTE *target, *trampoline;
    DWORD old_protection, ignored;
    uint32_t cookie;
    if (!image || !replacement || !original) return FALSE;
    *original = NULL;
    target = image + C07_LINEUP_RVA;
    cookie = (uint32_t)(uintptr_t)(image + C07_COOKIE_RVA);
    memcpy(expected + 4, &cookie, sizeof(cookie));
    if (memcmp(target, expected, sizeof(expected))) return FALSE;
    trampoline = VirtualAlloc(NULL, 13, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!trampoline) return FALSE;
    /* Both displaced instructions are complete. The absolute A1 operand has
       already been relocated by the PE loader; it needs no trampoline fixup. */
    memcpy(trampoline, target, C07_HOOK_LENGTH);
    relative_jump(trampoline + C07_HOOK_LENGTH, target + C07_HOOK_LENGTH);
    if (!VirtualProtect(trampoline, 13, PAGE_EXECUTE_READ, &ignored)) {
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return FALSE;
    }
    FlushInstructionCache(GetCurrentProcess(), trampoline, 13);
    if (!VirtualProtect(target, C07_HOOK_LENGTH, PAGE_EXECUTE_READWRITE, &old_protection)) {
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return FALSE;
    }
    /* Publish the fallback before the entry point becomes callable. */
    *original = trampoline;
    relative_jump(patch, replacement);
    /* relative_jump used patch's address; the installed instruction lives at target. */
    {
        uint32_t delta = (uint32_t)(uintptr_t)replacement - ((uint32_t)(uintptr_t)target + 5u);
        memcpy(patch + 1, &delta, sizeof(delta));
    }
    memcpy(target, patch, sizeof(patch));
    FlushInstructionCache(GetCurrentProcess(), target, sizeof(patch));
    /* Once installed the trampoline must remain alive even if restoring the
       page's old protection fails. Returning success preserves that invariant. */
    VirtualProtect(target, C07_HOOK_LENGTH, old_protection, &ignored);
    return TRUE;
}
