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

#include "modules.h"
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
	
	printq((_uint64)addr); print(" - "); print("ELF\n");
	
	PELF64SECT sect = (PELF64SECT)((_uint64)addr + e_hdr->e_shoff);
	PMODULEINFO mod = (PMODULEINFO)malloc(sizeof(MODULEINFO));
	mod->section_count = 0;
	for(int i = 0; i < e_hdr->e_shnum; i++)
		if((sect[i].sh_type == 1)||(sect[i].sh_type == 8))
			mod->section_count++;
	mod->sections = (PSECTION)malloc(mod->section_count*sizeof(SECTION));
	int i = 0, s = 0;
	return mod;
	while(i < mod->section_count){
		if((sect[s].sh_type == 1)||(sect[s].sh_type == 8)){
			mod->sections[i].offset = (void*)((_uint64)addr + sect[s].sh_offset);
			mod->sections[i].size = sect[s].sh_size;
			mod->sections[i].name = strcpy((char*)((_uint64)addr + sect[e_hdr->e_shstrndx].sh_offset + sect[s].sh_name));
			i++;
		}
		s++;
	}
	return mod;
}
void* load_module(void* addr)
{
	PMODULEINFO elf = load_module_elf(addr);
	if(elf != 0){
		
	} else {
		printq((_uint64)addr); print(" - ");
		print("Unrecognized\n");
	}
	return (void*)0;
}
void process_loop()
{
	for(;;) asm("hlt");
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
