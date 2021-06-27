//    PhoeniX OS Process subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "process.hpp"
#include "thread.hpp"

#include "memop.hpp"
#include "sprintf.hpp"
#include "rand.hpp"

using PTE = Pagetable::Entry;

Process::Process() :
    id(uint64_t(-1)), name(nullptr), entry(0),
    pagetable(nullptr), iomap { nullptr, nullptr },
    _aslrCode (RAND::get<uintptr_t>(0x400000llu, 0x800000llu) << 21),
    _aslrStack(RAND::get<uintptr_t>(0x200000llu, 0x400000llu) << 21)
{}

Process::~Process() {
  for (size_t i = 0; i < threads.getCount(); i++) delete threads[i];
  if (iomap[0]) Pagetable::free(iomap[0]);
  if (iomap[1]) Pagetable::free(iomap[1]);
  if (pagetable != nullptr) {
    PTE addr;
    for (uintptr_t ptx = 0; ptx < 512; ptx++) {
      addr = pagetable[ptx];
      if (!addr.present)
        continue;
      pagetable[ptx].present = 0;
      PTE *ppde = addr.getPTE();
      for (uintptr_t pdx = 0; pdx < 512; pdx++) {
        addr = ppde[pdx];
        if (!addr.present)
          continue;
        ppde[pdx].present = 0;
        PTE *pppde = addr.getPTE();
        for (uintptr_t pdpx = 0; pdpx < 512; pdpx++) {
          addr = pppde[pdpx];
          if (!addr.present)
            continue;
          pppde[pdpx].present = 0;
          if (addr.size == 0) {
            PTE *ppml4e = addr.getPTE();
            for (uintptr_t pml4x = 0; pml4x < 512; pml4x++) {
              addr = ppml4e[pml4x];
              if (!addr.present)
                continue;
              ppml4e[pml4x].present = 0;
              void *page = addr.getPtr();
              uintptr_t ptaddr = ((ptx << (12 + 9 + 9 + 9))
                  | (pdx << (12 + 9 + 9))
                  | (pdpx << (12 + 9)) | (pml4x << (12)));
              if (page == iomap[0]) continue;
              if (page == iomap[1]) continue;
              if (uintptr_t(page) == ptaddr) continue;
              Pagetable::free(page);
            }
            Pagetable::free(ppml4e);
          } else {
            void *page = addr.getPtr();
            uintptr_t ptaddr = ((ptx << (12 + 9 + 9 + 9))
                | (pdx << (12 + 9 + 9))
                | (pdpx << (12 + 9)));
            if (uintptr_t(page) == ptaddr) continue;
            Pagetable::free(page);
          }
        }
        Pagetable::free(pppde);
      }
      Pagetable::free(ppde);
    }
    Pagetable::free(pagetable);
  }
}

PTE* Process::addPage(uintptr_t vaddr, void* paddr, Pagetable::MemoryType type) {
  uint16_t ptx = (vaddr >> (12 + 9 + 9 + 9)) & 0x1FF;
  uint16_t pdx = (vaddr >> (12 + 9 + 9)) & 0x1FF;
  uint16_t pdpx = (vaddr >> (12 + 9)) & 0x1FF;
  uint16_t pml4x = (vaddr >> (12)) & 0x1FF;
  if (pagetable == nullptr) {
    pagetable = static_cast<PTE*>(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW));
    addPage(uintptr_t(pagetable), pagetable, Pagetable::MemoryType::USER_TABLE);
  }
  if (!pagetable[ptx].present) {
    pagetable[ptx] = PTE(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW), Pagetable::MemoryType::USER_TABLE);
    addPage(pagetable[ptx].getUintPtr(), pagetable[ptx].getPtr(), Pagetable::MemoryType::USER_TABLE);
  }
  PTE *pde = pagetable[ptx].getPTE();
  if (!pde[pdx].present) {
    pde[pdx] = PTE(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW), Pagetable::MemoryType::USER_TABLE);
    addPage(pde[pdx].getUintPtr(), pde[pdx].getPtr(), Pagetable::MemoryType::USER_TABLE);
  }
  PTE *pdpe = pde[pdx].getPTE();
  if (!pdpe[pdpx].present) {
    pdpe[pdpx] = PTE(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW), Pagetable::MemoryType::USER_TABLE);
    addPage(pdpe[pdpx].getUintPtr(), pdpe[pdpx].getPtr(), Pagetable::MemoryType::USER_TABLE);
  }
  PTE *pml4e = pdpe[pdpx].getPTE();
  pml4e[pml4x] = PTE(paddr, type);
  return &pml4e[pml4x];
}

PTE* Process::addHPage(uintptr_t vaddr, void* paddr, Pagetable::MemoryType type) {
  uint16_t ptx = (vaddr >> (12 + 9 + 9 + 9)) & 0x1FF;
  uint16_t pdx = (vaddr >> (12 + 9 + 9)) & 0x1FF;
  uint16_t pdpx = (vaddr >> (12 + 9)) & 0x1FF;
  if (pagetable == nullptr) {
    pagetable = static_cast<PTE*>(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW));
    addPage(uintptr_t(pagetable), pagetable, Pagetable::MemoryType::USER_TABLE);
  }
  if (!pagetable[ptx].present) {
    pagetable[ptx] = PTE(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW), Pagetable::MemoryType::USER_TABLE);
    addPage(pagetable[ptx].getUintPtr(), pagetable[ptx].getPtr(), Pagetable::MemoryType::USER_TABLE);
  }
  PTE *pde = pagetable[ptx].getPTE();
  if (!pde[pdx].present) {
    pde[pdx] = PTE(Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW), Pagetable::MemoryType::USER_TABLE);
    addPage(pde[pdx].getUintPtr(), pde[pdx].getPtr(), Pagetable::MemoryType::USER_TABLE);
  }
  PTE *pdpe = pde[pdx].getPTE();
  pdpe[pdpx] = PTE(paddr, type);
  pdpe[pdpx].size = 1;
  return &pdpe[pdpx];
}

uintptr_t Process::addSection(uintptr_t offset, SectionType type, size_t size) {
  if (size == 0)
    return 0;
  size_t pages = (size >> 12) + 1;
  uintptr_t vaddr;
  if (type != SectionTypeStack) {
    vaddr = _aslrCode + offset;
  } else {
    vaddr = _aslrStack;
  }

  for (uintptr_t caddr = vaddr; caddr < vaddr + size; caddr += 0x1000) {
    if (getPhysicalAddress(vaddr) != nullptr) {
      vaddr += 0x1000;
      caddr = vaddr - 0x1000;
    }
  }
  uintptr_t addr = vaddr;
  while (pages > 0) {
    Pagetable::MemoryType ptype;
    switch (type) {
      case SectionTypeCode: ptype = Pagetable::MemoryType::USER_CODE_RX; break;
      case SectionTypeROData: ptype = Pagetable::MemoryType::USER_DATA_RO; break;

      case SectionTypeData:
      case SectionTypeBSS:
      case SectionTypeStack:
      default:
        ptype = Pagetable::MemoryType::USER_DATA_RW;
        break;
    }
    if ((vaddr & 0x1FFFFF) == 0 && pages >= 0x200) {
      addHPage(vaddr, Pagetable::halloc(1, Pagetable::MemoryType::DATA_RW), ptype);
      vaddr += 0x200000;
      pages -= 0x200;
    } else {
      addPage(vaddr, Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW), ptype);
      vaddr += 0x1000;
      pages--;
    }
  }
  return addr;
}
void Process::setEntryAddress(uintptr_t ptr) {
  entry = ptr;
}
void Process::writeData(uintptr_t address, const void* src, size_t size) {
  const uint8_t *ptr = static_cast<const uint8_t*>(src);
  while (size > 0) {
    void *dest = getPhysicalAddress(address);
    size_t limit = 0x1000 - (uintptr_t(dest) & 0xFFF);
    if (limit > size) limit = size;
    Memory::copy(dest, ptr, limit);
    size -= limit;
    ptr += limit;
    address += limit;
  }
}
bool Process::readData(void* dst, uintptr_t address, size_t size) const {
  uint8_t *ptr = static_cast<uint8_t*>(dst);
  while (size > 0) {
    void *src = getPhysicalAddress(address);
    if (!src) return false;
    size_t limit = 0x1000 - (uintptr_t(src) & 0xFFF);
    if (limit > size) limit = size;
    Memory::copy(ptr, src, limit);
    size -= limit;
    ptr += limit;
    address += limit;
  }
  return true;
}
char *Process::readString(uintptr_t address) const {
  size_t length = 0;
  const uint8_t *src = static_cast<const uint8_t*>(getPhysicalAddress(address));
  if (!src) return nullptr;
  size_t limit = 0x1000 - (uintptr_t(src) & 0xFFF);
  while (limit-- && src[length] != 0) {
    length++;
    if (limit == 0) {
      src = static_cast<const uint8_t*>(getPhysicalAddress(address + length));
      limit += 0x1000;
    }
  }
  if (length == 0)
    return nullptr;
  char *buf = new char[length + 1]();
  if (!readData(buf, address, length + 1)) {
    delete[] buf;
    return nullptr;
  }
  return buf;
}
void *Process::getPhysicalAddress(uintptr_t ptr) const {
  if (pagetable == nullptr)
    return nullptr;
  PTE *addr = PTE::find(ptr, pagetable);
  if (!addr || !addr->present) return nullptr;
  uintptr_t off = ptr & (addr->size ? 0x1FFFFF : 0xFFF);
  return reinterpret_cast<void*>(addr->getUintPtr() + off);
}
void Process::allowIOPorts(uint16_t min, uint16_t max) {
  for (uint16_t i = 0; i <= (max - min); i++) {
    uint16_t port = min + i;
    size_t space = port / 8 / 0x1000;
    size_t byte = (port / 8) & 0xFFF;
    size_t bit = port % 8;
    uint8_t *map;
    if (!(map = reinterpret_cast<uint8_t*>(iomap[space]))) {
      map = reinterpret_cast<uint8_t*>(iomap[space] = Pagetable::alloc(1, Pagetable::MemoryType::DATA_RW));
      Memory::fill(map, 0xFF, 0x1000);
    }
    map[byte] &= uint8_t(~(1 << bit));
  }
}
const char *Process::getName() const { return name.get(); }
void Process::setName(const char *newname) {
  name.reset(newname ? klib::strdup(newname) : nullptr);
}

size_t Process::print_stacktrace(char *outbuf, size_t bufsz, uintptr_t base, const Process *process) {
  struct stackframe {
    struct stackframe* rbp;
    uintptr_t rip;
  } __attribute__((packed));

  Pagetable::Entry *pt;
  asm volatile("mov %%cr3, %q0":"=r"(pt)::);
  struct stackframe tmpframe;
  const struct stackframe *frame;
  if (base) {
    frame = reinterpret_cast<struct stackframe*>(base);
  } else {
    asm volatile("mov %%rbp, %q0":"=r"(frame)::);
  }
  size_t out = 0;
  while (frame != nullptr && bufsz > out) {
    bool pread = false;
    if (process) {
      if (
        process->readData(&tmpframe.rbp, uintptr_t(frame), sizeof(uintptr_t)) &&
        tmpframe.rbp &&
        process->readData(&tmpframe.rip, uintptr_t(frame)+sizeof(uintptr_t), sizeof(uintptr_t))
      ) {
        pread = true;
        frame = &tmpframe;
      }
    }
    if (!pread) {
      Pagetable::Entry *fpt1 = Pagetable::Entry::find(&frame->rbp, pt);
      Pagetable::Entry *fpt2 = Pagetable::Entry::find(&frame->rip, pt);
      if (!fpt1 || !fpt2 || !fpt1->present || !fpt2->present) break;
    }
    out += uintptr_t(snprintf(outbuf + out, bufsz - out, " [%p]:%p", reinterpret_cast<void*>(frame->rbp), reinterpret_cast<void*>(frame->rip)));
    frame = frame->rbp;
  }
  return out;
}
