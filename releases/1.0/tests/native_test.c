#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
/* Compile the production adapter into this harness so its native calls can
   target executable x86 fixture stubs without running or altering the game. */
#include "../src/plugin.c"

static BYTE mode_fixture[0x300], selector_fixture[0x40];
static BYTE team_fixture[0xaf8], records[20][0x168];
static int roster_count=20, fixture_pitch=8, fixture_test, stage_index;
static int completed, finalized, original_called, native_checks;

static void put32(void *p,uint32_t value) { memcpy(p,&value,4); }
static BYTE *__cdecl mock_mode(void) { return mode_fixture; }
static BYTE *__cdecl mock_player(uint32_t id) {
    assert(id>=1001 && id<=1020);
    return records[id-1001];
}
static BYTE *__cdecl mock_team(uint32_t id) { assert(id==42); return team_fixture; }
static int __cdecl mock_count(void) { return roster_count; }
static int __cdecl mock_pitch(void) { return fixture_pitch; }
static int __cdecl mock_overs(void) { return fixture_test?-1:50; }
static int side_of(void *self) { return (int)read32((BYTE *)self+0x34); }
static void check_stage(void *self,int side,uint32_t team,uint16_t *pool,int index) {
    assert(self==selector_fixture && side==side_of(self) && team==42 && pool);
    assert(stage_index++==index);
    assert(selector_fixture[0x3c]==0 && selector_fixture[0x3d]==0);
    ++native_checks;
}
static void append(uint16_t *pool,uint16_t id) {
    while(*pool) ++pool;
    *pool=id;
}
static void THISCALL mock_stage1(void *self,int side,uint32_t team,uint16_t *pool) {
    check_stage(self,side,team,pool,0);
    append(pool,1011); append(pool,1012);
    write16(mode_fixture+0x12c+side*12,1011);
    write16(mode_fixture+0x12e + side*12,1012);
}
static void THISCALL mock_stage2(void *self,int side,uint32_t team,uint16_t *pool) {
    check_stage(self,side,team,pool,1);
    if (fixture_pitch==12) {
        append(pool,1010); write16(mode_fixture+0x130+side*12,1010);
    }
}
static void THISCALL mock_stage3(void *self,int side,uint32_t team,uint16_t *pool) {
    check_stage(self,side,team,pool,2);
    append(pool,1013);
    write16(mode_fixture+(fixture_pitch==12?0x132:0x130)+side*12,1013);
}
static void THISCALL mock_stage4(void *self,int side,uint32_t team,uint16_t *pool) {
    check_stage(self,side,team,pool,3);
}
static void THISCALL mock_complete(void *self,int side,uint32_t team,uint16_t *pool) {
    uint16_t *xi=(uint16_t *)(mode_fixture+0xfc+side*22);
    assert(self==selector_fixture && team==42 && pool && stage_index==4);
    assert(selector_fixture[0x3c]==1 && selector_fixture[0x3d]==1);
    for(int i=0;i<11;++i) {
        assert(xi[i]>=1001 && xi[i]<=1020);
        for(int j=0;j<i;++j) assert(xi[i]!=xi[j]);
    }
    ++completed;
}
static uint16_t __cdecl mock_captain(const uint16_t *xi,const void *team) {
    assert(xi==(uint16_t *)(mode_fixture+0xfc+side_of(selector_fixture)*22));
    assert(team==team_fixture);
    return 1003;
}
static void THISCALL mock_finalize(void *self,int side) {
    assert(self==mode_fixture+0xe0 && side==side_of(selector_fixture));
    ++finalized;
}
static void THISCALL mock_original(void *self) { assert(self==selector_fixture); ++original_called; }

static void stub(uint32_t va,const void *function) {
    BYTE *at=address(va);
    at[0]=0xb8;
    put32(at+1,(uint32_t)(uintptr_t)function);
    at[5]=0xff; at[6]=0xe0; /* mov eax,function; jmp eax, preserving ECX/stack */
}
static void player(int index,int role,int skill,int accuracy) {
    BYTE *p=records[index];
    snprintf((char *)p+4,60,"Fixture%d",index+1);
    p[0x15f]=(BYTE)role;
    put32(p+0x6c,(uint32_t)skill<<6);
    put32(p+0x74,(uint32_t)accuracy<<12);
}
static void setup(int side,int owk,int test) {
    memset(mode_fixture,0xa5,sizeof(mode_fixture));
    memset(selector_fixture,0,sizeof(selector_fixture));
    memset(team_fixture,0,sizeof(team_fixture));
    memset(records,0,sizeof(records));
    for(int i=0;i<20;++i) { write16(team_fixture+i*2,(uint16_t)(1001+i)); player(i,0,1,1); }
    player(0,owk?8:6,62,1);
    player(1,8,45,10); player(2,2,55,10);
    player(3,1,50,10); player(4,1,48,10); player(5,1,46,10);
    player(6,9,20,60); player(7,5,59,50); player(8,9,58,10);
    player(9,3,15,61); player(10,10,14,55); player(11,10,13,52);
    player(12,4,12,40);
    put32(selector_fixture+0x30,(uint32_t)(uintptr_t)(mode_fixture+0xfc));
    put32(selector_fixture+0x34,(uint32_t)side);
    put32(selector_fixture+0x38,42);
    selector_fixture[0x3c]=1; selector_fixture[0x3d]=1;
    roster_count=20; fixture_test=test; fixture_pitch=test?12:8;
    stage_index=completed=finalized=original_called=0;
    InterlockedExchange(&selecting,0);
}
static void test_adapter(void) {
    for(int side=0;side<2;++side) for(int owk=0;owk<2;++owk) for(int test=0;test<2;++test) {
        BYTE other_before[22], others_bowl[12];
        int other=1-side;
        setup(side,owk,test);
        memcpy(other_before,mode_fixture+0xfc+other*22,22);
        memcpy(others_bowl,mode_fixture+0x12c+other*12,12);
        selection_hook(selector_fixture);
        assert(completed==1 && finalized==1 && original_called==0);
        assert(read16(mode_fixture+0x128+side*2)==1001);
        assert(read16(mode_fixture+0x198+side*2)==1003);
        assert(read16(mode_fixture+0xfe + side*22)==1003);
        assert(read16(mode_fixture+0xfc+side*22)==(owk?1001:1002));
        if (!owk) assert(read16(mode_fixture+0x100+side*22)==1001);
        if (!test) {
            assert(read16(mode_fixture+0x132+side*12)==1007); /* first ALL, Accuracy */
            assert(read16(mode_fixture+0x134+side*12)==1008); /* second ASP, Skill */
        }
        assert(mode_fixture[0x18c+side]==8);
        assert(!memcmp(other_before,mode_fixture+0xfc+other*22,22));
        assert(!memcmp(others_bowl,mode_fixture+0x12c+other*12,12));
    }
    setup(0,0,0); roster_count=10;
    selection_hook(selector_fixture);
    assert(original_called==1 && !completed && !stage_index);
    setup(0,0,0); put32(selector_fixture+0x30,0);
    selection_hook(selector_fixture);
    assert(original_called==1 && !completed && !stage_index);
    setup(0,0,0); InterlockedExchange(&selecting,1);
    selection_hook(selector_fixture);
    assert(original_called==1 && selecting==1);
    InterlockedExchange(&selecting,0);
}

static uint32_t THISCALL detour_destination(void *self) { return read32(self); }
static void test_detour(void) {
    BYTE prefix[8]={0x83,0xec,0x1c,0xa1,0,0,0,0};
    BYTE *entry=image_base+C07_LINEUP_RVA;
    BYTE suffix[]={0x83,0xc4,0x1c,0xc3};
    void *trampoline=NULL,*second=NULL;
    uint32_t expected=0x12345678, result;
    DWORD old,ignored;
    put32(prefix+4,(uint32_t)(uintptr_t)(image_base+C07_COOKIE_RVA));
    memcpy(entry,prefix,8); memcpy(entry+8,suffix,sizeof(suffix));
    put32(image_base+C07_COOKIE_RVA,0x89abcdef);
    VirtualProtect(entry,32,PAGE_EXECUTE_READ,&old);
    assert(C07InstallHook(image_base,(const void *)&detour_destination,&trampoline));
    result=((uint32_t(THISCALL *)(void *))entry)(&expected);
    assert(result==expected);
    result=((uint32_t(THISCALL *)(void *))trampoline)(&expected);
    assert(result==0x89abcdef); /* replayed A1, balanced displaced sub esp */
    assert(!C07InstallHook(image_base,(const void *)&detour_destination,&second));
    VirtualProtect(entry,32,PAGE_EXECUTE_READWRITE,&ignored);
    memcpy(entry,prefix,8); entry[0]=0xcc;
    assert(!C07InstallHook(image_base,(const void *)&detour_destination,&second));
    VirtualFree(trampoline,0,MEM_RELEASE);
}

static void test_dll(const char *path) {
    HMODULE dll=LoadLibraryA(path);
    void (__cdecl *initialize)(void);
    LONG (__cdecl *get_status)(void);
    assert(dll);
    initialize=(void(__cdecl *)(void))GetProcAddress(dll,"InitializeASI");
    get_status=(LONG(__cdecl *)(void))GetProcAddress(dll,"GetLineupStatus");
    assert(initialize && get_status && get_status()==0);
    initialize();
    assert(get_status()==-1); /* fixture executable must fail game SHA gate */
    initialize(); assert(get_status()==-1);
    FreeLibrary(dll);
}

int main(int argc,char **argv) {
    assert(argc==2);
    image_base=VirtualAlloc(NULL,0x650000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
    assert(image_base);
    original_entry=(void *)&mock_original;
    stub(0x40bad0,(const void *)&mock_mode); stub(0x4d8f10,(const void *)&mock_player);
    stub(0x4e04b0,(const void *)&mock_team); stub(0x4e0550,(const void *)&mock_count);
    stub(0x487c10,(const void *)&mock_pitch); stub(0x487c30,(const void *)&mock_overs);
    stub(0x4161f0,(const void *)&mock_stage1); stub(0x416370,(const void *)&mock_stage2);
    stub(0x416610,(const void *)&mock_stage3); stub(0x416890,(const void *)&mock_stage4);
    stub(0x413400,(const void *)&mock_complete); stub(0x4da740,(const void *)&mock_captain);
    stub(0x414950,(const void *)&mock_finalize);
    for(int i=0;i<63;++i) image_base[0x3985f9+2*i]=(BYTE)i;
    test_adapter(); test_detour(); test_dll(argv[1]);
    VirtualFree(image_base,0,MEM_RELEASE);
    printf("Native adapter/thiscall/detour/trampoline/DLL tests passed (%d native stage checks).\n",native_checks);
    return 0;
}
