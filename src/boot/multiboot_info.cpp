//    PhoeniX OS Multiboot subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "multiboot_info.hpp"

Multiboot::Payload *Multiboot::payload = nullptr;
Multiboot::Payload *Multiboot::getPayload() {
  return payload;
}
