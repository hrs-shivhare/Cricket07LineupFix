#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "selection.h"
#include "hook.h"
#include "compatibility.h"

#define THISCALL __attribute__((thiscall))
typedef void *( __cdecl *GetModeFn)(void);
typedef BYTE *( __cdecl *LookupFn)(uint32_t);
typedef int ( __cdecl *IntegerFn)(void);
typedef void (THISCALL *StageFn)(void *, int, uint32_t, uint16_t *);
typedef void (THISCALL *OriginalFn)(void *);
typedef uint16_t ( __cdecl *CaptainFn)(const uint16_t *, const void *);
typedef void (THISCALL *FinalizeFn)(void *, int);

static HMODULE module_self;
static BYTE *image_base;
static void *original_entry;
static volatile LONG initialized, status, selecting;
static const BYTE expected_sha[32] = {
    0xd7,0x76,0xdb,0xa0,0xcc,0x50,0x6b,0x96,0x8f,0x7d,0x22,0x8e,0x03,0x6c,0xf4,0x2a,
    0xeb,0x4a,0xbe,0x45,0xa8,0xd1,0xad,0x0d,0x72,0x26,0xcd,0xeb,0xf9,0x15,0x53,0xa2
};

static uint16_t read16(const void *p) { uint16_t v; memcpy(&v,p,2); return v; }
static uint32_t read32(const void *p) { uint32_t v; memcpy(&v,p,4); return v; }
static void write16(void *p,uint16_t v) { memcpy(p,&v,2); }
static void *address(uint32_t va) { return image_base + (va - 0x400000u); }

static void log_line(const char *line) {
    WCHAR path[MAX_PATH];
    DWORD n = GetModuleFileNameW(module_self,path,MAX_PATH), written;
    HANDLE file;
    if (!n || n >= MAX_PATH) return;
    while (n && path[n-1] != L'\\') --n;
    if (n + 24 >= MAX_PATH) return;
    memcpy(path+n,L"Cricket07Lineup.log",sizeof(L"Cricket07Lineup.log"));
    file = CreateFileW(path,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
                       OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    WriteFile(file,line,(DWORD)strlen(line),&written,NULL);
    WriteFile(file,"\r\n",2,&written,NULL);
    CloseHandle(file);
}

static BOOL executable_hash(BYTE digest[32]) {
    WCHAR path[MAX_PATH];
    BYTE buffer[16384];
    DWORD n, size = 32, length = GetModuleFileNameW(NULL,path,MAX_PATH);
    HANDLE file;
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    BOOL ok = FALSE;
    if (!length || length >= MAX_PATH) return FALSE;
    file = CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                       NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (file == INVALID_HANDLE_VALUE) return FALSE;
    if (!CryptAcquireContextW(&provider,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT) ||
        !CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)) goto done;
    for (;;) {
        if (!ReadFile(file,buffer,sizeof(buffer),&n,NULL)) goto done;
        if (!n) break;
        if (!CryptHashData(hash,buffer,n,0)) goto done;
    }
    if (!CryptGetHashParam(hash,HP_HASHVAL,digest,&size,0)) goto done;
    ok = size == 32;
done:
    if (hash) CryptDestroyHash(hash);
    if (provider) CryptReleaseContext(provider,0);
    CloseHandle(file);
    return ok;
}

static void log_executable_hash(void) {
    BYTE digest[32];
    char hex[65],line[256];
    if(!executable_hash(digest)) {
        log_line("WARNING: executable SHA-256 could not be read; required code/layout checks will decide compatibility.");
        return;
    }
    for(size_t i=0;i<32;++i) snprintf(hex+2*i,3,"%02x",digest[i]);
    snprintf(line,sizeof(line),"EXE SHA-256: %s",hex);
    log_line(line);
    if(memcmp(digest,expected_sha,sizeof(digest)))
        log_line("WARNING: executable SHA-256 differs from reference; continuing with required code/layout checks.");
    else log_line("Executable SHA-256 matches reference; checking loaded code/layout as well.");
}

static size_t read_squad(uint32_t team_id,C07Player *players) {
    BYTE *team=((LookupFn)address(0x4e04b0))(team_id);
    int n=((IntegerFn)address(0x4e0550))();
    size_t count=0;
    if (!team || n<11 || n>C07_MAX_PLAYERS) return 0;
    for (int i=0;i<n;++i) {
        uint16_t id=read16(team+i*2);
        BYTE *record;
        C07Player p;
        if (!id) continue;
        record=((LookupFn)address(0x4d8f10))(id);
        if (!record) return 0;
        p.id=id;
        p.role=record[0x15f]&15;
        p.skill=(BYTE)((read32(record+0x6c)>>6)&63);
        p.accuracy=(BYTE)((read32(record+0x74)>>12)&63);
        p.primary_field=(BYTE)((read32(record+0x84)>>10)&15);
        p.secondary_field=(BYTE)((read32(record+0x84)>>14)&15);
        p.legacy_keeper=(BYTE)(read32(record+0x8c)&63);
        players[count++]=p;
    }
    return count;
}

static void log_selection(int side,uint32_t team,int pitch,int is_test,
                          const C07Selection *s) {
    char line[2304];
    int used=snprintf(line,sizeof(line),"SELECTED side=%d team=%lu pitch=%d format=%s keeper=%u XI:",
                      side,(unsigned long)team,pitch,is_test?"Test":"limited",s->keeper);
    for (int i=0;i<11 && used>0 && (size_t)used<sizeof(line)-100;++i) {
        BYTE *p=((LookupFn)address(0x4d8f10))(s->xi[i]);
        unsigned raw=(read32(p+0x6c)>>6)&63;
        unsigned skill=*(image_base+0x3985f9u+2*(raw>62?62:raw));
        used+=snprintf(line+used,sizeof(line)-(size_t)used," %d=%u(%.55s,B%u)",
                       i+1,s->xi[i],(const char *)p+4,skill);
    }
    line[sizeof(line)-1]=0;
    log_line(line);
}

static void THISCALL selection_hook(void *object) {
    BYTE *self=(BYTE *)object, *g;
    int side, pitch, is_test;
    uint32_t team_id;
    C07Player players[C07_MAX_PLAYERS];
    C07Selection result;
    uint16_t pool[C07_MAX_PLAYERS+1]={0};
    size_t count;
    if (InterlockedCompareExchange(&selecting,1,0)) {
        ((OriginalFn)original_entry)(object);
        return;
    }
    side=(int)read32(self+0x34);
    team_id=read32(self+0x38);
    g=((GetModeFn)address(0x40bad0))();
    if (!g || side<0 || side>1 || read32(self+0x30)!=(uint32_t)(uintptr_t)(g+0xfc))
        goto original;
    count=read_squad(team_id,players);
    pitch=((IntegerFn)address(0x487c10))();
    is_test=((IntegerFn)address(0x487c30))()==-1;
    if (!c07_select(players,count,pitch,is_test,&result)) goto original;

    /* Selection is complete before touching live state. The former native
       role-selection passes and their ID filters are no longer invoked. */
    memcpy(g+0xfc+side*22,result.xi,sizeof(result.xi));
    memcpy(g+0x12c+side*12,result.bowling,sizeof(result.bowling));
    write16(g+0x128+side*2,result.keeper);
    memset(g+0x168+side*18,0,18);
    for (size_t i=0;i<result.bench_count && i<9;++i)
        write16(g+0x168+side*18+i*2,result.bench[i]);
    g[0x18c+side]=(BYTE)(result.bench_count?result.bench_count-1:0);
    memcpy(pool,result.bowling,sizeof(result.bowling));
    ((StageFn)address(0x413400))(self,side,team_id,pool);
    {
        BYTE *team=((LookupFn)address(0x4e04b0))(team_id);
        uint16_t captain=((CaptainFn)address(0x4da740))(
            (const uint16_t *)(g+0xfc+side*22),team);
        write16(g+0x198+side*2,captain);
    }
    ((FinalizeFn)address(0x414950))(g+0xe0,side);
    log_selection(side,team_id,pitch,is_test,&result);
    InterlockedExchange(&selecting,0);
    return;
original:
    log_line("FALLBACK: invalid/unexpected selector or roster; using original builder for this request.");
    ((OriginalFn)original_entry)(object);
    InterlockedExchange(&selecting,0);
}

void InitializeASI(void) {
    char reason[256],line[300];
    HMODULE pinned;
    if (InterlockedCompareExchange(&initialized,1,0)) return;
    log_line("Cricket07Lineup 3.2 initializing: v3.1 lineup rules; modified-EXE code/layout compatibility checks.");
    log_executable_hash();
    image_base=(BYTE *)GetModuleHandleW(NULL);
    if ((uintptr_t)image_base!=0x400000u) {
        status=-1;
        log_line("NOT APPLIED: required fixed-address layout is not loaded at base 0x400000.");
        return;
    }
    if(!C07CheckImage(image_base,reason,sizeof(reason))) {
        status=-1;
        snprintf(line,sizeof(line),"NOT APPLIED: compatibility check failed: %s.",reason);
        log_line(line);
        return;
    }
    snprintf(line,sizeof(line),"COMPATIBLE: %s.",reason);
    log_line(line);
    /* A process-lifetime detour must never outlive its DLL. */
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
                            (LPCWSTR)(uintptr_t)&selection_hook,&pinned) ||
        !C07InstallHook(image_base,(const void *)&selection_hook,&original_entry)) {
        status=-2;
        log_line("NOT APPLIED: conflicting entry bytes or hook installation failure.");
        return;
    }
    status=1;
    log_line("APPLIED: automatic lineup builder at VA 0x417330 (RVA 0x17330).");
    log_line("Rules: limited ALL/ASP Accuracy first; applicable Test BAT/ALL Skill flex before bowling; missing OBW join BWL/ALL; keeper competes for opener.");
}

LONG GetLineupStatus(void) { return InterlockedCompareExchange(&status,0,0); }
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved) {
    (void)reserved;
    if (reason==DLL_PROCESS_ATTACH) { module_self=module; DisableThreadLibraryCalls(module); }
    return TRUE;
}
