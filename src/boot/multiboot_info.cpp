//    PhoeniX OS Multiboot subsystem
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

#include "multiboot_info.hpp"

MULTIBOOT_PAYLOAD *Multiboot::payload = 0;
MULTIBOOT_PAYLOAD *Multiboot::getPayload() {
  return payload;
}
