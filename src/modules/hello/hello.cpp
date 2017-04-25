//    PhoeniX OS Stub hello module
//    Copyright © 2017 Yury Popov a.k.a. PhoeniX

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

extern "C" { void module(); void puts(const char *); void exit(int); }

void module() {
  puts("Hello, ");
  puts("world!\n");
  exit(0);
}
