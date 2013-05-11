#include "modules.h"
typedef struct {
	struct {
		long EI_MAG;
		char EI_CLASS;
		char EI_DATA;
		char EI_VERSION;
		char EI_OSABI;
		char EI_ABIVERSION;
		char EI_PAD;
		char reserved[6];
	} e_ident;
	unsigned short e_type; /* Object file type */
	unsigned short e_machine; /* Machine type */
	unsigned long e_version; /* Object file version */
	void* e_entry; /* Entry point address */
	_uint64 e_phoff; /* Program header offset */
	_uint64 e_shoff; /* Section header offset */
	unsigned long e_flags; /* Processor-specific flags */
	unsigned short e_ehsize; /* ELF header size */
	unsigned short e_phentsize; /* Size of program header entry */
	unsigned short e_phnum; /* Number of program header entries */
	unsigned short e_shentsize; /* Size of section header entry */
	unsigned short e_shnum; /* Number of section header entries */
	unsigned short e_shstrndx; /* Section name string table index */
} ELF64HDR, *PELF64HDR;
typedef struct
{
	unsigned long sh_name;
	unsigned long sh_type;
	_uint64 sh_flags;
	_uint64 sh_addr;
	_uint64 sh_offset;
	_uint64 sh_size;
	unsigned long sh_link;
	unsigned long sh_info;
	_uint64 sh_addralign;
	_uint64 sh_entsize;
} ELF64SECT, *PELF64SECT;
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
	print("#\tST\tSA\t\t\tSO\t\t\tSS\t\t\tSN\n");
	for(int i = 0; i < e_hdr->e_shnum; i++){
		printb(i);
		print("\t");
		printb(sect[i].sh_type);
		print("\t");
		printl(sect[i].sh_addr);
		print("\t");
		printl(sect[i].sh_offset);
		print("\t");
		printl(sect[i].sh_size);
		print("\t");
		print((char*)((_uint64)addr + sect[e_hdr->e_shstrndx].sh_offset + sect[i].sh_name));
		print("\n");
	}
	return (PMODULEINFO)0;
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
}