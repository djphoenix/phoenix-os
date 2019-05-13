//    PhoeniX OS Multiboot subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "multiboot_info.hpp"

Multiboot::Payload *Multiboot::payload = 0;
Multiboot::Payload *Multiboot::getPayload() {
  return payload;
}
