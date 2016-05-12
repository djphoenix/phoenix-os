//    PhoeniX OS Stub hello module
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

const char* module_name = "Test/Hello";
const char* module_version = "1.0";
const char* module_description = "Prints \"Hello, world\" text";
const char* module_requirements = "";
const char* module_developer = "PhoeniX";

extern "C" { void module(); }

void module() {
	for(;;) {}
}
