#include "../src/compatibility.h"
#include "../src/hook.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include "../src/compatibility_data.h"

static unsigned checks;
static void check(BYTE *image,int wanted,const char *substring) {
    char reason[256];
    assert(!!C07CheckImage(image,reason,sizeof(reason))==wanted);
    if(substring)assert(strstr(reason,substring));
    ++checks;
}

static void test_fixtures(void) {
    const size_t allocation=0x700000;
    BYTE *image=VirtualAlloc(NULL,allocation,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER *)image;
    IMAGE_NT_HEADERS32 *pe;
    DWORD old,ignored;
    assert(image);
    dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x100;
    pe=(IMAGE_NT_HEADERS32 *)(image+dos->e_lfanew);
    pe->Signature=IMAGE_NT_SIGNATURE;
    pe->FileHeader.Machine=IMAGE_FILE_MACHINE_I386;
    pe->FileHeader.SizeOfOptionalHeader=sizeof(IMAGE_OPTIONAL_HEADER32);
    pe->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    pe->OptionalHeader.ImageBase=0x400000;
    pe->OptionalHeader.SizeOfImage=(DWORD)allocation;
    for(size_t i=0;i<sizeof(c07_code_ranges)/sizeof(c07_code_ranges[0]);++i) {
        const C07CodeRange *r=&c07_code_ranges[i];
        memcpy(image+r->rva,r->bytes,r->length);
    }
    check(image,1,NULL); /* compatible code; no whole-file identity required */
    image[0x2000]^=0x5a;check(image,1,NULL); /* unrelated code modification */
    image[allocation-1]^=0x5a;check(image,1,NULL); /* added section/overlay data */
    image[0x3985f9]=99;check(image,1,NULL); /* display table values may differ */
    for(size_t i=0;i<sizeof(c07_code_ranges)/sizeof(c07_code_ranges[0]);++i) {
        const C07CodeRange *r=&c07_code_ranges[i];
        size_t positions[]={0,r->length-1};
        for(size_t j=0;j<2;++j) {
            image[r->rva+positions[j]]^=1;
            check(image,0,"required function");
            image[r->rva+positions[j]]^=1;
        }
    }
    assert(VirtualProtect(image+C07_LINEUP_RVA,4096,PAGE_READWRITE,&old));
    check(image,0,"non-executable");
    assert(VirtualProtect(image+C07_LINEUP_RVA,4096,old,&ignored));
    assert(VirtualProtect(image+0x400000,4096,PAGE_NOACCESS,&old));
    check(image,0,"global/data");
    assert(VirtualProtect(image+0x400000,4096,old,&ignored));
    pe->OptionalHeader.SizeOfImage=0x600000;check(image,0,"address layout");
    pe->OptionalHeader.SizeOfImage=(DWORD)allocation;
    pe->OptionalHeader.ImageBase=0x500000;check(image,0,"address layout");
    pe->OptionalHeader.ImageBase=0x400000;
    pe->FileHeader.Machine=IMAGE_FILE_MACHINE_AMD64;check(image,0,"address layout");
    pe->FileHeader.Machine=IMAGE_FILE_MACHINE_I386;
    pe->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR64_MAGIC;check(image,0,"address layout");
    pe->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    dos->e_lfanew=0x7fffffff;check(image,0,"invalid/unreadable");dos->e_lfanew=0x100;
    dos->e_magic=0;check(image,0,"invalid/unreadable");dos->e_magic=IMAGE_DOS_SIGNATURE;
    check(NULL,0,"invalid/unreadable");check(image,1,NULL);
    VirtualFree(image,0,MEM_RELEASE);
    printf("Compatibility fixtures passed (%u checks: unrelated changes accepted; required code/layout/protection failures refused).\n",checks);
}

/* Map bytes into a disposable private allocation; never execute them or alter
 * the supplied EXE. Runtime validation additionally checks real page protections. */
static int check_file(const char *path) {
    FILE *file=fopen(path,"rb");
    BYTE *bytes=NULL,*image=NULL;
    long length;
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS32 pe;
    char reason[256];
    size_t section_table,allocation_size;
    int result=1;
    if(!file){fprintf(stderr,"Cannot open %s\n",path);return 1;}
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<(long)sizeof(dos)||length>64*1024*1024||fseek(file,0,SEEK_SET))goto done;
    bytes=malloc((size_t)length);if(!bytes||fread(bytes,1,(size_t)length,file)!=(size_t)length)goto done;
    memcpy(&dos,bytes,sizeof(dos));
    if(dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<0||(size_t)dos.e_lfanew+sizeof(pe)>(size_t)length)goto done;
    memcpy(&pe,bytes+dos.e_lfanew,sizeof(pe));
    if(pe.Signature!=IMAGE_NT_SIGNATURE||pe.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC||
       pe.OptionalHeader.SizeOfImage<0x640000||pe.OptionalHeader.SizeOfImage>128*1024*1024||
       pe.OptionalHeader.SizeOfHeaders>(size_t)length||pe.OptionalHeader.SizeOfHeaders>pe.OptionalHeader.SizeOfImage||
       pe.FileHeader.SizeOfOptionalHeader<sizeof(IMAGE_OPTIONAL_HEADER32)||pe.FileHeader.NumberOfSections>96)goto done;
    section_table=(size_t)dos.e_lfanew+24+pe.FileHeader.SizeOfOptionalHeader;
    if(section_table+pe.FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER)>(size_t)length)goto done;
    /* Reference EXE's SizeOfImage is not page-aligned. Its final section has
       file padding through the end of the loader's last committed page. */
    allocation_size=((size_t)pe.OptionalHeader.SizeOfImage+4095u)&~4095u;
    image=VirtualAlloc(NULL,allocation_size,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
    if(!image)goto done;
    memcpy(image,bytes,pe.OptionalHeader.SizeOfHeaders);
    for(size_t i=0;i<pe.FileHeader.NumberOfSections;++i) {
        IMAGE_SECTION_HEADER section;
        memcpy(&section,bytes+section_table+i*sizeof(section),sizeof(section));
        if(section.VirtualAddress>allocation_size||section.SizeOfRawData>allocation_size-section.VirtualAddress||
           section.PointerToRawData>(size_t)length||section.SizeOfRawData>(size_t)length-section.PointerToRawData)goto done;
        memcpy(image+section.VirtualAddress,bytes+section.PointerToRawData,section.SizeOfRawData);
    }
    result=!C07CheckImage(image,reason,sizeof(reason));
    printf("EXE file compatibility %s: %s (%s).\n",result?"REFUSED":"PASS",reason,path);
done:
    if(result)fprintf(stderr,"Not a verified compatible mapped EXE: %s\n",path);
    if(image)VirtualFree(image,0,MEM_RELEASE);
    free(bytes);fclose(file);return result;
}

int main(int argc,char **argv) {
    if(argc==3&&!strcmp(argv[1],"--check-file"))return check_file(argv[2]);
    assert(argc==1);test_fixtures();return 0;
}
