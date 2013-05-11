#ifndef MODULES_H
#define MODULES_H
#include "pxlib.h"
#include "multiboot_info.h"
extern void modules_init();
typedef struct{
	void* entry;
} MODULEINFO, *PMODULEINFO;
#endif
