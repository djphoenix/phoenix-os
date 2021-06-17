//    PhoeniX OS Lists subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "heap.hpp"

#include "memop.hpp"

template<typename Item>
class List {
 private:
  Item *items = nullptr;
  size_t count = 0;
  size_t capacity = 0;
  static constexpr size_t cap_inc = sizeof(Item) < 256 ? 1 : (256 / sizeof(Item));

 public:
  ~List() { Heap::free(items); }

  inline Item& insert() {
    if (count == capacity) {
      capacity += cap_inc;
      items = static_cast<Item*>(Heap::realloc(items, sizeof(Item) * capacity));
    }
    return items[count++];
  }
  inline void remove(size_t idx) {
    Memory::copy(items + idx, items + idx + 1, sizeof(Item) * ((--count) - idx));
  }

  inline void add(const Item &item) { insert() = item; }

  inline size_t getCount() const { return count; }

  inline Item& operator[] (const size_t index) { return items[index]; }
  inline const Item& operator[] (const size_t index) const { return items[index]; }
};
