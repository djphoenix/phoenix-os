//    PhoeniX OS Lists subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "heap.hpp"

template<typename Item>
class List {
 private:
  Item *items = nullptr;
  size_t count = 0;
  size_t capacity = 0;
 public:
  ~List() { Heap::free(items); }

  Item& insert() {
    if (count == capacity) {
      capacity += klib::max(256 / sizeof(Item), size_t(1));
      items = static_cast<Item*>(Heap::realloc(items, sizeof(Item) * capacity));
    }
    return items[count++];
  }
  void remove(size_t idx) {
    Memory::copy(items + idx, items + idx + 1, sizeof(Item) * ((--count) - idx));
  }

  void add(const Item &item) { insert() = item; }

  size_t getCount() { return count; }

  Item& operator[] (const size_t index) { return items[index]; }
};
