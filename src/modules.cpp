//    PhoeniX OS Modules subsystem
//    Copyright (C) 2013  PhoeniX
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "modules.hpp"

PMODULEINFO ModuleManager::loadElf(Stream *stream){
#pragma pack(push,1)
    struct {
        struct {uint magic; char eclass,data,version,osabi,abiversion,pad,rsvd[6];} ident;
        unsigned short type, machine;
        uint version;
        _uint64 entry, phoff, shoff;
        uint flags;
        unsigned short ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
    } Elf;
    ELF64SECT *Sections;
#pragma pack(pop)
    stream->read(&Elf,sizeof(Elf));
    if (
        (Elf.ident.magic != 'FLE\x7F') ||
        (Elf.ident.eclass != 2) ||
        (Elf.ident.data != 1) ||
        (Elf.ident.version != 1) ||
        (Elf.ident.osabi != 0) ||
        (Elf.ident.abiversion != 0) ||
        (Elf.type != 1) ||
        (Elf.version != 1) ||
        (Elf.ehsize != sizeof(Elf))
        ) return 0;
	PMODULEINFO mod = (PMODULEINFO)Memory::alloc(sizeof(MODULEINFO));
    mod->size = MAX(Elf.phoff + Elf.phentsize * Elf.phnum,
                    Elf.shoff + Elf.shentsize * Elf.shnum);
    stream->seek(Elf.shoff);
    Sections = (ELF64SECT*)Memory::alloc(sizeof(ELF64SECT)*Elf.shnum);
    stream->read(Sections,sizeof(ELF64SECT)*Elf.shnum);
    mod->psinfo.seg_cnt = 0;
    PROCSECT *segments = (PROCSECT*)Memory::alloc(sizeof(PROCSECT)*Elf.shnum);
    _uint64 symoff, symcount, symnsoff;
	for(int s = 0; s < Elf.shnum; s++){
        mod->size = MAX(mod->size,Sections[s].offset+Sections[s].size);
        if (Sections[s].type == 2) {
            symoff = Sections[s].offset;
            symcount = Sections[s].size / sizeof(ELF64SYM);
            symnsoff = Sections[Sections[s].link].offset;
		}
        if (Sections[s].flags & 2) {
            if (Sections[s].size == 0) continue;
            int x = -1;
            for (int i = 0; i < mod->psinfo.seg_cnt; i++) {
                if (Sections[s].offset == segments[i].offset+segments[i].size) {x=i; break;}
            }
            if (x == -1) {
                segments[mod->psinfo.seg_cnt].offset = Sections[s].offset;
                segments[mod->psinfo.seg_cnt].vaddr = 0;
                segments[mod->psinfo.seg_cnt].fsize = (Sections[s].type==1)?Sections[s].size:0;
                segments[mod->psinfo.seg_cnt].size = Sections[s].size;
                mod->psinfo.seg_cnt++;
            } else {
                segments[x].fsize += (Sections[s].type==1)?Sections[s].size:0;
                segments[x].size += Sections[s].size;
            }
        }
	}
    if (mod->psinfo.seg_cnt != 0) {
        mod->psinfo.segments = (PROCSECT*)Memory::alloc(sizeof(PROCSECT)*mod->psinfo.seg_cnt);
        Memory::copy((char*)mod->psinfo.segments,(char*)segments,sizeof(PROCSECT)*mod->psinfo.seg_cnt);
    }
    Memory::free(segments);
    
    ELF64SYM *Symbols = (ELF64SYM*)Memory::alloc(sizeof(ELF64SYM)*symcount);
    stream->seek(symoff,-1);
    stream->read(Symbols,sizeof(ELF64SYM)*symcount);
    
    for (int i=0;i<symcount;i++){
        if (((Symbols[i].info >> 4) & 0xF) == 0) continue;
        char* symname = stream->readstr(symnsoff+Symbols[i].name);
        ELF64SECT symsect = Sections[Symbols[i].shndx];
        char* sectname = stream->readstr(Sections[Elf.shstrndx].offset + symsect.name);
        
        ELF64SECT *relsect = 0;
        _uint64 off = (symsect.offset + Symbols[i].value);
        char* rsn = (char*)Memory::alloc(strlen(sectname)+6);
        Memory::copy(rsn,(char*)".rela",5);
        Memory::copy(&rsn[5],sectname,strlen(sectname));
        rsn[5+strlen(sectname)+1] =0;
        for (int s=1; s<Elf.shnum; s++) {
            char* sn = stream->readstr(Sections[Elf.shstrndx].offset + Sections[s].name);
            if (strcmp(sn,rsn)) {
                relsect = &Sections[s]; break;
            }
        }
        if (relsect == 0) {
            Memory::copy(&rsn[4],sectname,strlen(sectname));
            rsn[4+strlen(sectname)+1] =0;
            for (int s=1; s<Elf.shnum; s++) {
                char* sn = stream->readstr(Sections[Elf.shstrndx].offset + Sections[s].name);
                if (strcmp(sn,rsn)) {
                    relsect = &Sections[s]; break;
                }
            }
        }
        Memory::free(rsn);
        if (relsect != 0) {
            if (relsect->type == 4) {
                ELF64RELA *rela = (ELF64RELA*)Memory::alloc(relsect->size);
                stream->seek(relsect->offset,-1);
                stream->read(rela,relsect->size);
                for (int r=0; r<relsect->size/sizeof(ELF64RELA); r++) {
                    if (Symbols[i].value == rela[r].addr) {
                        if (rela[r].info.type == 1) {
                            off = Sections[Symbols[rela[r].info.sym].shndx].offset+rela[r].add;
                        } else {
                            print("\nUnknown reloc:\n");
                            printq(rela[r].addr);
                            print(" ");
                            printl(rela[r].info.sym);
                            print(" ");
                            printl(rela[r].info.type);
                            print(" ");
                            printq(rela[r].add);
                            print("\n");
                            for(;;);
                        }
                    }
                }
                Memory::free(rela);
            } else if (relsect->type == 9) {
                ELF64REL *rel = (ELF64REL*)Memory::alloc(relsect->size);
                stream->seek(relsect->offset,-1);
                stream->read(rel,relsect->size);
                for (int r=0; r<relsect->size/sizeof(ELF64REL); r++) {
                    if (Symbols[i].value == rel[r].addr) {
                        if (rel[r].info.type == 1) {
                            off = Sections[Symbols[rel[r].info.sym].shndx].offset;
                        } else {
                            print("\nUnknown reloc:\n");
                            printq(rel[r].addr);
                            print(" ");
                            printl(rel[r].info.sym);
                            print(" ");
                            printl(rel[r].info.type);
                            print("\n");
                            for(;;);
                        }
                    }
                }
                Memory::free(rel);
            }
        }
        if (strcmp("module_name",symname)) {
            mod->name = stream->readstr(off);
        } else if (strcmp("module_version",symname)) {
            mod->version = stream->readstr(off);
        } else if (strcmp("module_description",symname)) {
            mod->description = stream->readstr(off);
        } else if (strcmp("module_requirements",symname)) {
            mod->requirements = stream->readstr(off);
        } else if (strcmp("module_developer",symname)) {
            mod->developer = stream->readstr(off);
        }
        Memory::free(symname);
        Memory::free(sectname);
    }
    Memory::free(Symbols);
    Memory::free(Sections);
    
    return mod;
}

ModuleManager* ModuleManager::manager = 0;
void ModuleManager::loadStream(Stream *stream){
	PMODULEINFO mod;
	if((mod = loadElf(stream)) != 0){
        print("ELF Module \""); print(mod->name); print("\"\n");
        for (int i = 0; i < mod->psinfo.seg_cnt; i++) {
            print("SEG"); printb(i);
            print(" @"); printl(mod->psinfo.segments[i].vaddr);
            print(" f"); printl(mod->psinfo.segments[i].offset);
            print(" m"); printl(mod->psinfo.segments[i].size);
            print(" c"); printl(mod->psinfo.segments[i].fsize);
            print("\n");
        }
        stream->seek(mod->size,-1);
        if (!stream->eof()){
            Stream *sub = stream->substream();
            loadStream(sub);
            delete sub;
        }
        return;
	}
    print("Unrecognized module type\n");
}
void ModuleManager::parseInternal(){
	if((kernel_data.modules != 0) && (kernel_data.modules != kernel_data.modules_top)){
        Stream *ms = new MemoryStream((void*)kernel_data.modules,kernel_data.modules_top-kernel_data.modules);
        loadStream(ms);
        delete ms;
	}
}
void ModuleManager::parseInitRD(){
	if(kernel_data.mods != 0){
		PMODULE mod = kernel_data.mods;
		while(mod != 0){
            Stream *ms = new MemoryStream((void*)mod->start,((_uint64)mod->end)-((_uint64)mod->start));
            loadStream(ms);
            delete ms;
			mod = (PMODULE)mod->next;
		}
	}
}
void ModuleManager::init() {
    ModuleManager *mm = getManager();
    print("Built-in modules:\n");
    mm->parseInternal();
    print("Modules in initRD:\n");
    mm->parseInitRD();
	print("Modules loaded\n");
}
ModuleManager* ModuleManager::getManager(){
    if (manager) return manager;
    return manager = new ModuleManager();
}
ModuleManager::ModuleManager(){
}
