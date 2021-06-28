//    PhoeniX OS Lists subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#pragma once
#include "pagetable.hpp"

#include "memop.hpp"

template<typename Item>
class List {
 private:
  static constexpr bool use_hp = sizeof(Item) > 0x100;
  static constexpr size_t pagesize = use_hp ? 0x200000 : 0x1000;
  static constexpr size_t items_per_page = (pagesize - 16) / sizeof(Item);
  static_assert(items_per_page > 0);

  struct Page {
    size_t count;
    Page *next;
    Item items[items_per_page];
  };
  static_assert(pagesize == sizeof(Page));

  Page *rootpage = nullptr;

  static inline Page* _allocPage() {
    if (use_hp) {
      return static_cast<Page*>(Pagetable::halloc(1, Pagetable::MemoryType::DATA_RW));
    } else {
      return static_cast<Page*>(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW));
    }
  }

 public:
  ~List() {
    while (rootpage != nullptr) {
      Page *p = rootpage;
      rootpage = p->next;
      Pagetable::free(p);
    }
  }

  inline Item& insert() {
    if (rootpage == nullptr) rootpage = _allocPage();
    Page *p = rootpage;
    for (;;) {
      if (p->count == items_per_page) {
        if (!p->next) p->next = _allocPage();
        p = p->next;
        continue;
      }
      return p->items[p->count++];
    }
  }
  inline void remove(size_t idx) {
    for (Page *p = rootpage; p != nullptr; p = p->next) {
      if (idx < p->count) {
        Memory::copy(p->items + idx, p->items + idx + 1, sizeof(Item) * ((--p->count) - idx));
        return;
      }
      idx -= p->count;
    }
  }

  inline void add(const Item &item) { insert() = item; }
  inline void add(Item &&item) { insert() = item; }

  inline size_t getCount() const {
    size_t count = 0;
    for (Page *p = rootpage; p != nullptr; p = p->next) count += p->count;
    return count;
  }

  inline Item& operator[] (size_t index) {
    for (Page *p = rootpage; p != nullptr; p = p->next) {
      if (index < p->count) return p->items[index];
      index -= p->count;
    }
    __builtin_unreachable();
  }
  inline const Item& operator[] (size_t index) const {
    for (Page *p = rootpage; p != nullptr; p = p->next) {
      if (index < p->count) return p->items[index];
      index -= p->count;
    }
    __builtin_unreachable();
  }
};
