/*
===========================================================================
    Copyright (C) 2010-2013  Ninja and TheKelm

    This file is part of CoD4X18-Server source code.

    CoD4X18-Server source code is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    CoD4X18-Server source code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
===========================================================================
*/



#include <string.h>
#include <stdlib.h>
#include "elf.h"
#include "elfhlp.h"

#ifdef _WIN32
    #include <memoryapi.h>
#else

#endif

unsigned int ELFHlp_CountVirtualMemory(Elf32_Phdr* p, int segmentcount)
{
    int i;
    unsigned int alignment = 0;
    unsigned int dynamicsize = 0;

    unsigned int highestvirtaddr = 0;
    
    for(i = 0; i < segmentcount; ++i, ++p)
    {
        if(p->p_type == PT_DYNAMIC) //Also reserve the dynamic segment for easier symbol resolving later
        {
            dynamicsize = p->p_filesz + p->p_align;
            continue;
        }
        if(p->p_type != PT_LOAD)
        {
            continue;
        }
        alignment += p->p_align;
        if(p->p_vaddr >= highestvirtaddr)
        {
            highestvirtaddr = p->p_vaddr + p->p_memsz;
        }
    }
    return highestvirtaddr + alignment + dynamicsize;
}

uint32_t ELFHlp_GetDynValueForTag(Elf32_Ehdr *hdr, Elf32_Sword d_tag)
{
    int j, i;
    Elf32_Phdr *phdr, *p;
    phdr = (Elf32_Phdr *)(hdr->e_phoff + (unsigned char*)hdr);

    for(i = 0; i < hdr->e_phnum; ++i)
    {
        p = &phdr[i];
        if(p->p_type != PT_DYNAMIC)
        {
            continue;
        }

        Elf32_Dyn* dyn = (Elf32_Dyn*)((unsigned char*)hdr + p->p_offset);

        for(j = 0; dyn->d_tag != DT_NULL; ++j, ++dyn)
        {
            if(dyn->d_tag == d_tag)
            {
                return dyn->d_un.d_val;
            }
        }
    }
    return -1;
}

bool ELFHlp_ValidateDynSegments(unsigned char* filedata, unsigned int filesize)
{
    unsigned int i, j;
    Elf32_Ehdr *hdr;
    Elf32_Phdr *phdr, *p;
    hdr = (Elf32_Ehdr*)filedata;
    phdr = (Elf32_Phdr *)(hdr->e_phoff + filedata);

    for(i = 0; i < hdr->e_phnum; ++i)
    {
        p = &phdr[i];
        if(p->p_type != PT_DYNAMIC)
        {
            continue;
        }

        Elf32_Dyn* dyn = (Elf32_Dyn*)(filedata + p->p_offset);

        for(j = 0; j < p->p_filesz / sizeof(Elf32_Dyn) && dyn->d_tag != DT_NULL; ++j, ++dyn);
        if(dyn->d_tag != DT_NULL)
        {
            return false;
        }
    }
    return true;

}

uint32_t ELFHlp_elf_hash(const char* symname)
{
    const uint8_t* name = (uint8_t*)symname;
    uint32_t h = 0, g;
    for (; *name; name++) {
        h = (h << 4) + *name;
        if ((g = h & 0xf0000000)) {
            h ^= g >> 24;
        }
        h &= ~g;
    }
    return h;
}

uint32_t ELFHlp_gnu_hash(const char* symname) 
{
    uint32_t h = 5381;
    const uint8_t* name = (uint8_t*)symname;
    for (; *name; name++) {
        h = (h << 5) + h + *name;
    }

    return h;
}


Elf32_Sym* ELFHlp_LookupSymbol_gnu(struct gnu_hash_table* hashtab, Elf32_Sym* symboltable, unsigned int symentrysize, const char* stringtab, const char* name)
{
    const uint32_t* buckets = hashtab->data + hashtab->bloom_size;
    const uint32_t* chain = hashtab->data + hashtab->bloom_size + hashtab->nbuckets;


    const uint32_t namehash = ELFHlp_gnu_hash(name);

    uint32_t symix = buckets[namehash % hashtab->nbuckets];
    if (symix < hashtab->symoffset) {
        return NULL;
    }
    
    uint32_t hash;
    do
    {
        Elf32_Sym* symbol = (Elf32_Sym*)((char*)symboltable + symix * symentrysize);
        const char* symname = stringtab + symbol->st_name;
        hash = chain[symix - hashtab->symoffset];

        if(strcmp(symname, name) == 0)
        {
            return symbol;
        }
        symix++;
    }while (!(hash & 1));
    
    return NULL;
}


Elf32_Sym* ELFHlp_LookupSymbol(struct elf_hash_table* hashtab, Elf32_Sym* symboltable, unsigned int symentrysize, const char* stringtab, const char* name)
{

    const uint32_t* buckets = hashtab->data;
    const uint32_t* chain = hashtab->data + hashtab->nbuckets;


    const uint32_t namehash = ELFHlp_elf_hash(name);

    uint32_t symix = buckets[namehash % hashtab->nbuckets];
    
    while (symix)
    {
        Elf32_Sym* symbol = (Elf32_Sym*)((char*)symboltable + symix * symentrysize);
        const char* symname = stringtab + symbol->st_name;

        if(strcmp(symname, name) == 0)
        {
            return symbol;
        }
        symix = chain[symix];
    }
    return NULL;
}


Elf32_Sym* ELFHlp_GetSymbolForName(Elf32_Ehdr* hdr, const char* symname)
{
    uint32_t symtab = ELFHlp_GetDynValueForTag(hdr, DT_SYMTAB);
    uint32_t symtabentlen = ELFHlp_GetDynValueForTag(hdr, DT_SYMENT);
    uint32_t strtab = ELFHlp_GetDynValueForTag(hdr, DT_STRTAB);
    uint32_t strtablen = ELFHlp_GetDynValueForTag(hdr, DT_STRSZ);

    if((signed int)symtab == -1 || (signed int)symtabentlen == -1 || (signed int)strtab == -1 || (signed int)strtablen == -1)
    {
        return NULL;
    }

    const char* stringtable = (char*)hdr + strtab;
    Elf32_Sym* symboltable = (Elf32_Sym*)((char*)hdr + symtab);

    uint32_t symhashtab = ELFHlp_GetDynValueForTag(hdr, DT_HASH);

    Elf32_Sym* symbol = NULL;

    if((signed int)symhashtab != -1)
    {

        symbol = ELFHlp_LookupSymbol((struct elf_hash_table*)((char*)hdr + symhashtab), 
                                                symboltable, symtabentlen, stringtable, symname);
    }else{
        symhashtab = ELFHlp_GetDynValueForTag(hdr, DT_GNU_HASH);
        if((signed int)symhashtab == -1)
        {
            return NULL;
        }
        symbol = ELFHlp_LookupSymbol_gnu((struct gnu_hash_table*)((char*)hdr + symhashtab), 
                                                symboltable, symtabentlen, stringtable, symname);
    }
    return symbol;
}


void ELFHlp_PrintSymbolNames(Elf32_Ehdr* hdr, void (*printf)(const char* fmt, ...))
{
    printf("Begin test dynamic redirect\n");
    Elf32_Sym* symbol;
    uint32_t symtab = ELFHlp_GetDynValueForTag(hdr, DT_SYMTAB);
    uint32_t symtabentlen = ELFHlp_GetDynValueForTag(hdr, DT_SYMENT);
    uint32_t strtab = ELFHlp_GetDynValueForTag(hdr, DT_STRTAB);
    uint32_t strtablen = ELFHlp_GetDynValueForTag(hdr, DT_STRSZ);

    uint32_t pltreltab = ELFHlp_GetDynValueForTag(hdr, DT_JMPREL);
    uint32_t pltreltabsize = ELFHlp_GetDynValueForTag(hdr, DT_PLTRELSZ);
/*
    uint32_t pltgot = ELFHlp_GetDynValueForTag(hdr, DT_PLTRELSZ);
    uint32_t pltrel = ELFHlp_GetDynValueForTag(hdr, DT_PLTREL);
*/

    if((signed int)symtab == -1 || (signed int)symtabentlen == -1 || (signed int)strtab == -1 || (signed int)strtablen == -1 || 
        (signed int)pltreltab == -1 || (signed int)pltreltabsize == -1)
    {
        return;
    }

    const char* stringtable = (char*)hdr + strtab;
    Elf32_Sym* symboltable = (Elf32_Sym*)((char*)hdr + symtab);
    Elf32_Rel* pltreltable = (Elf32_Rel*)((char*)hdr + pltreltab);
    Elf32_Rel* pltrel;

    unsigned int i;
    for(i = 0, pltrel = pltreltable; i < pltreltabsize / sizeof(Elf32_Rel); ++i, ++pltrel)
    {
        if(ELF32_R_TYPE(pltrel->r_info) != ELF_R_386_JMP_SLOT) //test if this is something with PLT
        {
            continue;
        }
        unsigned int symix = ELF32_R_SYM(pltrel->r_info);
        symbol = (Elf32_Sym*)((char*)symboltable + symix * symtabentlen);
        printf("%i: %s GOT address: 0x%x\n", i, stringtable + symbol->st_name, pltrel->r_offset);

        
    }

}

unsigned char* ELFHlp_LoadAndAllocImage(unsigned char* filedata, unsigned int filesize)
{
    Elf32_Ehdr *hdr;
    Elf32_Phdr *phdr, *p;
    int i;
    
    if(filesize < sizeof(Elf32_Ehdr)){
        return NULL;
    }

    hdr = (Elf32_Ehdr*)filedata;
    if(hdr->e_ident[0] != ELFMAG0 || hdr->e_ident[1] != ELFMAG1 || hdr->e_ident[2] != ELFMAG2 || hdr->e_ident[3] != ELFMAG3){
        return NULL;
    }
    if(hdr->e_type != ET_DYN){
        return NULL;
    }

    if(hdr->e_phoff + hdr->e_phnum * sizeof(Elf32_Phdr) > filesize)
    {
        return NULL;
    }

    if(hdr->e_machine != EM_386 && hdr->e_machine != EM_486) //i386 or i486 machine
    {
        return NULL;
    }

    phdr = (Elf32_Phdr *)(hdr->e_phoff + filedata);


    unsigned int virtualmemsize = ELFHlp_CountVirtualMemory(phdr, hdr->e_phnum);

    if(virtualmemsize == 0)
    {
        return NULL;
    }

    unsigned char* imagestart = (unsigned char*)VirtualAlloc(NULL, virtualmemsize, MEM_COMMIT, PAGE_READWRITE);

    Elf32_Addr dynamicsegmentstart = 0;

    for(i = 0; i < hdr->e_phnum; ++i) //Get highest virtual address used by load segments
    {
        p = &phdr[i];
        if(p->p_type != PT_LOAD)
        {
            continue;
        }
        if(p->p_vaddr + p->p_memsz > dynamicsegmentstart)
        {
            dynamicsegmentstart = p->p_vaddr + p->p_memsz;
        }
    }


    for(i = 0; i < hdr->e_phnum; ++i) //copy the dynamic segment
    {
        p = &phdr[i];
        if(p->p_type != PT_DYNAMIC)
        {
            continue;
        }
        if(p->p_filesz < 1)
        {
            continue;
        }

        if(dynamicsegmentstart % p->p_align != 0)
        {
            unsigned int ialignmask = p->p_align -1;
            unsigned int alignmask = ~ialignmask;
            dynamicsegmentstart = (dynamicsegmentstart + p->p_align) & alignmask;
        }
        memcpy(imagestart + dynamicsegmentstart, filedata + p->p_offset, p->p_filesz);
        p->p_offset = dynamicsegmentstart; //update the program header so this copy of dynamic segement can be found in memory
    }

    for(i = 0; i < hdr->e_phnum; ++i) //Copy the load segments
    {
        p = &phdr[i];
        if(p->p_type != PT_LOAD)
        {
            continue;
        }
        unsigned int ialignmask = p->p_align -1;
        unsigned int alignmask = ~ialignmask;
        unsigned char* segmentstart = (unsigned char*)((unsigned int)(imagestart + p->p_vaddr) & alignmask);

        if(p->p_filesz > 0)
        {
            if(p->p_vaddr + p->p_memsz > virtualmemsize || p->p_memsz < p->p_filesz)
            {
                VirtualFree(imagestart, 0, MEM_RELEASE);
                return NULL;
            }
            memcpy(imagestart + p->p_vaddr, filedata + p->p_offset, p->p_filesz);
        }
   		DWORD oldprotect;
		DWORD newprotect = PAGE_READONLY;
        if(p->p_flags & PF_X && p->p_flags & PF_W)
        {
            newprotect = PAGE_EXECUTE_READWRITE;
        }
        else if(p->p_flags & PF_X)
        {
            newprotect = PAGE_EXECUTE_READ;
        }
        else if(p->p_flags & PF_W)
        {
            newprotect = PAGE_READWRITE;
        }
        unsigned int segmentlen = (p->p_vaddr & ialignmask) + p->p_memsz;
        if(segmentlen & ialignmask)
        {
            segmentlen = (segmentlen & alignmask) + p->p_align;
        }
        VirtualProtect(segmentstart, segmentlen, newprotect, &oldprotect);
    }

    if(!ELFHlp_ValidateDynSegments(imagestart, filesize))
    {
        VirtualFree(imagestart, 0, MEM_RELEASE);
        return NULL;
    }
    return imagestart;
}

void ELFHlp_FreeImage(unsigned char* image)
{
    if(image == NULL)
    {
        return;
    }
    VirtualFree(image, 0, MEM_RELEASE);
}

void* ELFHlp_GetProcAddress(const unsigned char* image, const char *procname) //similar like Win32 GetProcAddress()
{
    Elf32_Ehdr *hdr;
    hdr = (Elf32_Ehdr*)image;
    Elf32_Sym *symbol = ELFHlp_GetSymbolForName(hdr, procname);
    if(symbol == NULL)
    {
        return NULL;
    }

//    printf("bind info %d, type info %d, other %d size %d value 0x%x\n", ELF32_ST_BIND(symbol->st_info), ELF32_ST_TYPE(symbol->st_info), symbol->st_other, symbol->st_size, symbol->st_value);

    if(ELF32_ST_BIND(symbol->st_info) != STB_GLOBAL || ELF32_ST_TYPE(symbol->st_info) != STT_FUNC || ELF32_ST_VISIBILITY(symbol->st_other) != STV_DEFAULT)
    {
        return NULL;
    }

    void* entrypoint = (void*)(symbol->st_value + (unsigned int)image);
    return entrypoint;
}

