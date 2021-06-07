#pragma once

#include <stdint.h>
#include <stddef.h>

namespace kcrypto {
  namespace hash {
    class SHA512 {
    private:
      static const uint64_t K[80];
      struct State {
        uint64_t h[8] {
          0x6a09e667f3bcc908,
          0xbb67ae8584caa73b,
          0x3c6ef372fe94f82b,
          0xa54ff53a5f1d36f1,
          0x510e527fade682d1,
          0x9b05688c2b3e6c1f,
          0x1f83d9abfb41bd6b,
          0x5be0cd19137e2179,
        };
        uint64_t bits = 0;
        uint8_t chunk[128];
      } __attribute__((aligned(16)));

      State state {};
      void _transform();
    public:
      void update(const void *ptr, size_t sz);
      void final();
      typedef uint8_t Digest[sizeof(State::h)] __attribute__((aligned(16)));
      void digest(Digest &digest) const;
    } __attribute__((aligned(16)));
  }
}
