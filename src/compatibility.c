#include "compatibility.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "compatibility_data.h"

static BOOL accessible(const BYTE *p,size_t length,BOOL executable) {
    uintptr_t cursor=(uintptr_t)p,end;
    if(length>UINTPTR_MAX-cursor) return FALSE;
    end=cursor+length;
    while(cursor<end) {
        MEMORY_BASIC_INFORMATION info;
        uintptr_t next;
        DWORD protection;
        if(!VirtualQuery((const void *)cursor,&info,sizeof(info)) ||
           info.State!=MEM_COMMIT || (info.Protect&(PAGE_GUARD|PAGE_NOACCESS))) return FALSE;
        protection=info.Protect&0xff;
        if(protection!=PAGE_READONLY && protection!=PAGE_READWRITE &&
           protection!=PAGE_WRITECOPY && protection!=PAGE_EXECUTE_READ &&
           protection!=PAGE_EXECUTE_READWRITE && protection!=PAGE_EXECUTE_WRITECOPY) return FALSE;
        if(executable && protection!=PAGE_EXECUTE_READ && protection!=PAGE_EXECUTE_READWRITE &&
           protection!=PAGE_EXECUTE_WRITECOPY) return FALSE;
        next=(uintptr_t)info.BaseAddress+info.RegionSize;
        if(next<=cursor) return FALSE;
        cursor=next;
    }
    return TRUE;
}

BOOL C07CheckImage(const BYTE *image,char *reason,size_t reason_size) {
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS32 pe;
    /* Original .data span includes the game mode/database singleton slots,
     * cookie, format/pitch and squad-count globals. Values may change. */
    const uint32_t data_rva=0x3e6000u,data_size=0x257748u;
    if(!reason || !reason_size) return FALSE;
    snprintf(reason,reason_size,"invalid/unreadable PE32 image");
    if(!image || !accessible(image,sizeof(dos),FALSE)) return FALSE;
    memcpy(&dos,image,sizeof(dos));
    if(dos.e_magic!=IMAGE_DOS_SIGNATURE || dos.e_lfanew<0 || dos.e_lfanew>0x100000 ||
       !accessible(image+(uint32_t)dos.e_lfanew,sizeof(pe),FALSE)) return FALSE;
    memcpy(&pe,image+(uint32_t)dos.e_lfanew,sizeof(pe));
    if(pe.Signature!=IMAGE_NT_SIGNATURE || pe.FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 ||
       pe.FileHeader.SizeOfOptionalHeader<sizeof(IMAGE_OPTIONAL_HEADER32) ||
       pe.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
       pe.OptionalHeader.ImageBase!=0x400000u || pe.OptionalHeader.SizeOfImage<data_rva+data_size) {
        snprintf(reason,reason_size,"incompatible PE32/x86 address layout (reference base 0x400000 required)");
        return FALSE;
    }
    for(size_t i=0;i<sizeof(c07_code_ranges)/sizeof(c07_code_ranges[0]);++i) {
        const C07CodeRange *r=&c07_code_ranges[i];
        if(r->rva>pe.OptionalHeader.SizeOfImage || r->length>pe.OptionalHeader.SizeOfImage-r->rva ||
           !accessible(image+r->rva,r->length,TRUE)) {
            snprintf(reason,reason_size,"required function VA 0x%08lx is missing/unreadable/non-executable",(unsigned long)r->va);
            return FALSE;
        }
        for(size_t j=0;j<r->length;++j) if(image[r->rva+j]!=r->bytes[j]) {
            snprintf(reason,reason_size,"required function VA 0x%08lx differs at RVA 0x%08lx",(unsigned long)r->va,(unsigned long)(r->rva+j));
            return FALSE;
        }
    }
    if(!accessible(image+data_rva,data_size,FALSE) || !accessible(image+0x3985f9u,125,FALSE)) {
        snprintf(reason,reason_size,"required global/data or Skill display table range is unavailable");
        return FALSE;
    }
    snprintf(reason,reason_size,"15 required functions, lineup entry and data ranges match reference layout");
    return TRUE;
}
