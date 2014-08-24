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
PMODULEINFO load_module_elf(void* addr)
{
	// Parsing ELF
	PELF64HDR e_hdr = (PELF64HDR)addr;
	if(e_hdr->e_ident.EI_MAG != 'FLE\x7F') return (PMODULEINFO)0;
	if(e_hdr->e_ident.EI_CLASS != 2 /* ELFCLASS64 */) return (PMODULEINFO)0;
	if(e_hdr->e_ident.EI_DATA != 1 /* ELFDATA2LSB */) return (PMODULEINFO)0;
	if(e_hdr->e_ident.EI_VERSION != 1 /* EV_CURRENT */) return (PMODULEINFO)0;
	if(e_hdr->e_ident.EI_OSABI != 0 /* ELFOSABI_SYSV */) return (PMODULEINFO)0;
	if(e_hdr->e_ident.EI_ABIVERSION != 0) return (PMODULEINFO)0;

	if(e_hdr->e_type != 1 /* ET_REL */) return (PMODULEINFO)0;
	if(e_hdr->e_version != 1 /* EV_CURRENT */) return (PMODULEINFO)0;
	if(e_hdr->e_ehsize != sizeof(ELF64HDR)) return (PMODULEINFO)0;
	
	PELF64SECT sect = (PELF64SECT)((_uint64)addr + e_hdr->e_shoff);
	PMODULEINFO mod = (PMODULEINFO)malloc(sizeof(MODULEINFO));
    mod->size = MAX(
                    e_hdr->e_phoff+e_hdr->e_phentsize*e_hdr->e_phnum,
                    e_hdr->e_shoff+e_hdr->e_shentsize*e_hdr->e_shnum);
    PELF64SYM sym; int symcount;
    char* symnames;
	for(int s = 0; s < e_hdr->e_shnum; s++){
        mod->size = MAX(mod->size,sect[s].sh_offset+sect[s].sh_size);
        if (sect[s].sh_type == 2) {
            sym = (PELF64SYM)((_uint64)addr + sect[s].sh_offset);
            symcount = sect[s].sh_size / sizeof(ELF64SYM);
            symnames = (char*)((_uint64)addr + sect[sect[s].sh_link].sh_offset);
		}
	}
    for (int i=0;i<symcount;i++){
        if (((sym[i].st_info >> 4) & 0xF) == 0) continue;
        char* symname = &symnames[sym[i].st_name];
        PELF64SECT symsect = &sect[sym[i].st_shndx];
        char* sectname = (char*)((_uint64)addr + sect[e_hdr->e_shstrndx].sh_offset + symsect->sh_name);

        PELF64SECT relsect = 0;
        _uint64 off = ((_uint64)symsect->sh_offset + (_uint64)sym[i].st_value);
        char* rsn = (char*)malloc(strlen(sectname)+6);
        memcpy(rsn,(char*)".rela",5);
        memcpy(&rsn[5],sectname,strlen(sectname));
        rsn[5+strlen(sectname)+1] =0;
        for (int s=0; s<e_hdr->e_shnum; s++) {
            char* sn = (char*)((_uint64)addr + sect[e_hdr->e_shstrndx].sh_offset + sect[s].sh_name);
            if (strcmp(sn,rsn)) {
                relsect = &sect[s]; break;
            }
        }
        if (relsect == 0) {
            memcpy(&rsn[4],sectname,strlen(sectname));
            rsn[4+strlen(sectname)+1] =0;
            for (int s=0; s<e_hdr->e_shnum; s++) {
                char* sn = (char*)((_uint64)addr + sect[e_hdr->e_shstrndx].sh_offset + sect[s].sh_name);
                if (strcmp(sn,rsn)) {
                    relsect = &sect[s]; break;
                }
            }
        }
        mfree(rsn);
        if (relsect != 0) {
            if (relsect->sh_type == 4) {
                PELF64RELA rela = (PELF64RELA)((_uint64)addr+relsect->sh_offset);
                for (int r=0; r<relsect->sh_size/sizeof(ELF64RELA); r++) {
                    if ((_uint64)sym[i].st_value == rela[r].addr) {
                        if (rela[r].info.type == 1) {
                            off = (_uint64)sect[sym[rela[r].info.sym].st_shndx].sh_offset+rela[r].add;
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
            } else if (relsect->sh_type == 9) {
                PELF64REL rel = (PELF64REL)((_uint64)addr+relsect->sh_offset);
                for (int r=0; r<relsect->sh_size/sizeof(ELF64REL); r++) {
                    if ((_uint64)sym[i].st_value == rel[r].addr) {
                        if (rel[r].info.type == 1) {
                            off = (_uint64)sect[sym[rel[r].info.sym].st_shndx].sh_offset;
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
            }
        }

        if (strcmp("module_name",symname)) {
            mod->name = (char*)((_uint64)addr+off);
        } else if (strcmp("module_version",symname)) {
            mod->version = (char*)((_uint64)addr+off);
        } else if (strcmp("module_description",symname)) {
            mod->description = (char*)((_uint64)addr+off);
        } else if (strcmp("module_requirements",symname)) {
            mod->requirements = (char*)((_uint64)addr+off);
        } else if (strcmp("module_developer",symname)) {
            mod->developer = (char*)((_uint64)addr+off);
        }
    }
	return mod;
}
void* load_module(void* addr)
{
	PMODULEINFO mod;
	if((mod = load_module_elf(addr)) != 0){
        printq((_uint64)addr); print(" - ELF\n");
        print("Name: "); print(mod->name); print("\n");
        print("Version: "); print(mod->version); print("\n");
        print("Description: "); print(mod->description); print("\n");
        print("Requirements: "); print(mod->requirements); print("\n");
        print("Developer: "); print(mod->developer); print("\n");
        print("Size: "); printq(mod->size); print("\n");
        return (void*)((_uint64)addr + mod->size);
	} else {
		printq((_uint64)addr); print(" - ");
		print("Unrecognized\n");
	}
	return (void*)0;
}
void modules_init()
{
	if((kernel_data.modules != 0) && (kernel_data.modules != kernel_data.modules_top)){
		void *module = (void*)kernel_data.modules;
		print("Built-in modules:\n");
		while(((_uint64)module < kernel_data.modules_top) && ((_uint64)module != 0))
			module = load_module(module);
	}
	if(kernel_data.mods != 0){
		print("Modules in initRD:\n");
		PMODULE mod = kernel_data.mods;
		while(mod != 0){
			void *module = (void*)mod->start;
			while(((_uint64)module < (_uint64)mod->end) && ((_uint64)module != 0))
				module = load_module(module);
			mod = (PMODULE)mod->next;
		}
	}
	print("Modules loaded\n");
}
