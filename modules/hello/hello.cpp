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

#define MODDESC(k, v) \
  extern const char \
    __attribute__((section(".module"))) \
    __attribute__((aligned(1))) \
    module_ ## k[] = v; \

MODDESC(name, "Test/Hello");
MODDESC(version, "1.0");
MODDESC(description, "Prints \"Hello, world\" text");
MODDESC(requirements, "");
MODDESC(developer, "PhoeniX");

extern "C" { void module(); }

void module() {
}
