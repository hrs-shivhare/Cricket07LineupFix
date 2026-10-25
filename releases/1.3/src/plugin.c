#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "selection.h"
#include "hook.h"

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

static BOOL supported_executable(void) {
    WCHAR path[MAX_PATH];
    BYTE buffer[16384], digest[32];
    DWORD n, size = sizeof(digest), length = GetModuleFileNameW(NULL,path,MAX_PATH);
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
    ok = size == sizeof(digest) && !memcmp(digest,expected_sha,sizeof(digest));
done:
    if (hash) CryptDestroyHash(hash);
    if (provider) CryptReleaseContext(provider,0);
    CloseHandle(file);
    return ok;
}

typedef struct SavedSide {
    uint16_t xi[11], bowling[6], keeper, bench[9], captain;
    BYTE reserve_last;
} SavedSide;

static void save_side(BYTE *g,int side,SavedSide *saved) {
    memcpy(saved->xi,g+0xfc+side*22,sizeof(saved->xi));
    memcpy(saved->bowling,g+0x12c+side*12,sizeof(saved->bowling));
    memcpy(saved->bench,g+0x168+side*18,sizeof(saved->bench));
    saved->keeper=read16(g+0x128+side*2);
    saved->captain=read16(g+0x198+side*2);
    saved->reserve_last=g[0x18c+side];
}

static void restore_side(BYTE *g,int side,const SavedSide *saved) {
    memcpy(g+0xfc+side*22,saved->xi,sizeof(saved->xi));
    memcpy(g+0x12c+side*12,saved->bowling,sizeof(saved->bowling));
    memcpy(g+0x168+side*18,saved->bench,sizeof(saved->bench));
    write16(g+0x128+side*2,saved->keeper);
    write16(g+0x198+side*2,saved->captain);
    g[0x18c+side]=saved->reserve_last;
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
    size_t count,pool_count=0;
    SavedSide saved;
    BYTE flag_limited,flag_test;
    const uint32_t stages[]={0x4161f0,0x416370,0x416610,0x416890};
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
    if (!c07_begin(players,count,&result)) goto original;
    pitch=((IntegerFn)address(0x487c10))();
    is_test=((IntegerFn)address(0x487c30))()==-1;
    save_side(g,side,&saved);
    memcpy(g+0xfc+side*22,result.xi,sizeof(result.xi));
    memset(g+0x12c+side*12,0,sizeof(result.bowling));
    write16(g+0x128+side*2,result.keeper);

    /* The original bowler passes are retained, including their pitch quotas,
       accuracy comparisons and ordering. Disable only this selector object's
       two hardcoded-ID exclusion switches while those passes execute. */
    flag_limited=self[0x3c]; flag_test=self[0x3d];
    self[0x3c]=0; self[0x3d]=0;
    for (size_t i=0;i<sizeof(stages)/sizeof(stages[0]);++i)
        ((StageFn)address(stages[i]))(self,side,team_id,pool);
    self[0x3c]=flag_limited; self[0x3d]=flag_test;
    while (pool_count<C07_MAX_PLAYERS && pool[pool_count]) ++pool_count;
    memcpy(result.bowling,g+0x12c+side*12,sizeof(result.bowling));
    if (!c07_finish(players,count,pitch,is_test,pool,pool_count,&result)) {
        restore_side(g,side,&saved);
        goto original;
    }
    memcpy(g+0xfc+side*22,result.xi,sizeof(result.xi));
    memcpy(g+0x12c+side*12,result.bowling,sizeof(result.bowling));
    memset(g+0x168+side*18,0,18);
    for (size_t i=0;i<result.bench_count && i<9;++i)
        write16(g+0x168+side*18+i*2,result.bench[i]);
    /* This game field is the last occupied index, not a reserve count. */
    g[0x18c+side]=(BYTE)(result.bench_count?result.bench_count-1:0);
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
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS32 *pe;
    HMODULE pinned;
    if (InterlockedCompareExchange(&initialized,1,0)) return;
    log_line("Cricket07Lineup 1.3 initializing: credited first extra any/Skill; second extra ALL/ASP/Skill.");
    if (!supported_executable()) {
        status=-1;
        log_line("NOT APPLIED: unsupported executable SHA-256.");
        return;
    }
    image_base=(BYTE *)GetModuleHandleW(NULL);
    dos=(IMAGE_DOS_HEADER *)image_base;
    pe=(IMAGE_NT_HEADERS32 *)(image_base+dos->e_lfanew);
    if (dos->e_magic!=IMAGE_DOS_SIGNATURE || pe->Signature!=IMAGE_NT_SIGNATURE ||
        pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 ||
        pe->OptionalHeader.SizeOfImage!=6606446u) {
        status=-1;
        log_line("NOT APPLIED: unsupported PE image.");
        return;
    }
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
    log_line("Rules: Accuracy-winning ALL/ASP in batting trio frees first extra pick only; second stays ALL/ASP; extra opener priority retained.");
}

LONG GetLineupStatus(void) { return InterlockedCompareExchange(&status,0,0); }
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved) {
    (void)reserved;
    if (reason==DLL_PROCESS_ATTACH) { module_self=module; DisableThreadLibraryCalls(module); }
    return TRUE;
}
