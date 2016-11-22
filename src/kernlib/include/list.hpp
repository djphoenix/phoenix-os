//    PhoeniX OS Lists subsystem
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

#pragma once
#include "heap.hpp"

template<typename Item>
class List {
 private:
  Item *items = 0;
  size_t count = 0;
  size_t capacity = 0;
 public:
  ~List() { Heap::free(items); }

  Item& insert() {
    if (count == capacity) {
      capacity += MAX(256 / sizeof(Item), (size_t)1);
      items = static_cast<Item*>(Heap::realloc(items, sizeof(Item) * capacity));
    }
    return items[count++];
  }

  void add(const Item item) { insert() = item; }

  size_t getCount() { return count; }

  Item& operator[] (const size_t index) { return items[index]; }
};
