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
#include "readelf.hpp"

bool ModuleManager::parseModuleInfo(MODULEINFO *info, Process &process) {
	struct {
		uintptr_t entry, name, version, desc, reqs, dev;
	} symbols = {
		process.getSymbolByName("module"),
		process.getSymbolByName("module_name"),
		process.getSymbolByName("module_version"),
		process.getSymbolByName("module_description"),
		process.getSymbolByName("module_requirements"),
		process.getSymbolByName("module_developer")
	};
	MODULEINFO mod = {0, 0, 0, 0, 0};
	if ((symbols.entry == 0) ||
		(symbols.name == 0) ||
		(symbols.version == 0) ||
		(symbols.desc == 0) ||
		(symbols.reqs == 0) ||
		(symbols.dev == 0))
		return false;

	process.readData(&symbols.name, symbols.name, sizeof(uintptr_t));
	process.readData(&symbols.version, symbols.version, sizeof(uintptr_t));
	process.readData(&symbols.desc, symbols.desc, sizeof(uintptr_t));
	process.readData(&symbols.reqs, symbols.reqs, sizeof(uintptr_t));
	process.readData(&symbols.dev, symbols.dev, sizeof(uintptr_t));

	if ((symbols.entry == 0) ||
		(symbols.name == 0) ||
		(symbols.version == 0) ||
		(symbols.desc == 0) ||
		(symbols.reqs == 0) ||
		(symbols.dev == 0))
		return false;

	process.setEntryAddress(symbols.entry);
	
	mod.name = process.readString(symbols.name);
	mod.version = process.readString(symbols.version);
	mod.description = process.readString(symbols.desc);
	mod.requirements = process.readString(symbols.reqs);
	mod.developer = process.readString(symbols.dev);

	*info = mod;
	
	return true;
}

ModuleManager* ModuleManager::manager = 0;
void ModuleManager::loadStream(Stream *stream, bool start) {
	size_t size;
	MODULEINFO mod;
parse:
	mod = {0, 0, 0, 0, 0};
	Process *process = new Process();
	size = readelf(process, stream);
	if (size == 0) {
		process->remove();
		return;
	}
	if (!parseModuleInfo(&mod, *process)) {
		process->remove();
		return;
	}
	if(start) process->startup();
	stream->seek(size, -1);
	if (!stream->eof()) {
		stream = stream->substream();
		goto parse;
	}
}
void ModuleManager::parseInternal() {
	if((kernel_data.modules != 0) &&
	   (kernel_data.modules != kernel_data.modules_top)) {
		Stream *ms = new MemoryStream((void*)kernel_data.modules,
									  kernel_data.modules_top-kernel_data.modules);
		loadStream(ms, 1);
		delete ms;
	}
}
void ModuleManager::parseInitRD() {
	if(kernel_data.mods != 0) {
		PMODULE mod = kernel_data.mods;
		while(mod != 0) {
			Stream *ms = new MemoryStream((void*)mod->start,
										  ((_uint64)mod->end)-((_uint64)mod->start));
			loadStream(ms, 1);
			mod = (PMODULE)mod->next;
			delete ms;
		}
	}
}
void ModuleManager::init() {
	ModuleManager *mm = getManager();
	mm->parseInternal();
	mm->parseInitRD();
}
ModuleManager* ModuleManager::getManager() {
	if (manager) return manager;
	return manager = new ModuleManager();
}
ModuleManager::ModuleManager() {}
