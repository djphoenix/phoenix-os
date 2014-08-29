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
    mod->psinfo.sect_cnt = 0;
    PROCSECT *segments = (PROCSECT*)Memory::alloc(sizeof(PROCSECT)*Elf.shnum);
    PROCSECT *sections = (PROCSECT*)Memory::alloc(sizeof(PROCSECT)*Elf.shnum);
    int* secmap = (int*)Memory::alloc(sizeof(int)*Elf.shnum);
    _uint64 symoff, symcount, symnsoff;
	for(int s = 0; s < Elf.shnum; s++){
        mod->size = MAX(mod->size,Sections[s].offset+Sections[s].size);
        if (Sections[s].type == 2) {
            symoff = Sections[s].offset;
            symcount = Sections[s].size / sizeof(ELF64SYM);
            symnsoff = Sections[Sections[s].link].offset;
		}
        if (Sections[s].flags & 2) {
            secmap[s] = mod->psinfo.sect_cnt;
            sections[mod->psinfo.sect_cnt].offset = Sections[s].offset;
            sections[mod->psinfo.sect_cnt].vaddr = 0;
            sections[mod->psinfo.sect_cnt].fsize = (Sections[s].type==1)?Sections[s].size:0;
            sections[mod->psinfo.sect_cnt].size = Sections[s].size;
            mod->psinfo.sect_cnt++;
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
    if (mod->psinfo.sect_cnt != 0) {
        mod->psinfo.sections = (PROCSECT*)Memory::alloc(sizeof(PROCSECT)*mod->psinfo.sect_cnt);
        Memory::copy((char*)mod->psinfo.sections,(char*)sections,sizeof(PROCSECT)*mod->psinfo.sect_cnt);
    }
    Memory::free(segments);
    Memory::free(sections);
    
    ELF64SYM *Symbols = (ELF64SYM*)Memory::alloc(sizeof(ELF64SYM)*symcount);
    stream->seek(symoff,-1);
    stream->read(Symbols,sizeof(ELF64SYM)*symcount);
    
    mod->psinfo.sym_cnt = symcount;
    mod->psinfo.symbols = (PROCSYM*)Memory::alloc(sizeof(PROCSYM)*mod->psinfo.sym_cnt);
    
    for (int i=1;i<symcount;i++){
        char* symname = stream->readstr(symnsoff+Symbols[i].name);
        switch (Symbols[i].info & 0xF) {
            case(3):
                mod->psinfo.symbols[i].type = 0; break; // section
            case(0):
                mod->psinfo.symbols[i].type = 2; break; // link
            default:
                mod->psinfo.symbols[i].type = 1; // symbol
        }
        int symsect = Symbols[i].shndx < Elf.shnum ? secmap[Symbols[i].shndx] : 0;
        mod->psinfo.symbols[i].sect=symsect;
        mod->psinfo.symbols[i].name=symname;
        mod->psinfo.symbols[i].offset=Symbols[i].value;
        if(strcmp("module",symname)) mod->psinfo.entry_sym=i;
    }
    Memory::free(secmap);
    Memory::free(Symbols);
    Memory::free(Sections);
    
    return mod;
}

ModuleManager* ModuleManager::manager = 0;
void ModuleManager::loadStream(Stream *stream){
	PMODULEINFO mod;
	if((mod = loadElf(stream)) != 0){
        print("ELF Module:\n");
        for (int i = 0; i < mod->psinfo.sect_cnt; i++) {
            print("SECT"); printb(i);
            print(" @"); printl(mod->psinfo.sections[i].vaddr);
            print(" f"); printl(mod->psinfo.sections[i].offset);
            print(" m"); printl(mod->psinfo.sections[i].size);
            print(" c"); printl(mod->psinfo.sections[i].fsize);
            print("\n");
        }
        for (int i = 0; i < mod->psinfo.seg_cnt; i++) {
            print("SEG"); printb(i);
            print(" @"); printl(mod->psinfo.segments[i].vaddr);
            print(" f"); printl(mod->psinfo.segments[i].offset);
            print(" m"); printl(mod->psinfo.segments[i].size);
            print(" c"); printl(mod->psinfo.segments[i].fsize);
            print("\n");
        }
        for (int i = 0; i < mod->psinfo.sym_cnt; i++) {
            print("SYM"); printb(i);
            print(" @"); printl(mod->psinfo.symbols[i].offset);
            print(" s"); prints(mod->psinfo.symbols[i].sect);
            print(" t"); prints(mod->psinfo.symbols[i].type);
            if (mod->psinfo.symbols[i].name != 0) {
                print(" ");
                print(mod->psinfo.symbols[i].name);
            }
            print("\n");
        }
        print("Entry symbol: "); printl(mod->psinfo.entry_sym); print("\n");
        print("\n");
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
