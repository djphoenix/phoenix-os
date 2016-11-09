//    PhoeniX OS Kernel library mutex functions
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

#include "kernlib.hpp"

void Mutex::lock() {
  bool ret_val = 0, old_val = 0, new_val = 1;
  do {
    asm volatile("lock cmpxchgb %1,%2":
        "=a"(ret_val):
        "r"(new_val),"m"(state),"0"(old_val):
        "memory");
  } while (ret_val);
}

void Mutex::release() {
  bool ret_val = 0, new_val = 0;
  asm volatile("lock xchgb %1,%2":
      "=a"(ret_val):
      "r"(new_val),"m"(state):
      "memory");
}
