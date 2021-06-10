#pragma once

#include <stdint.h>
#include <stddef.h>

namespace kcrypto {
  namespace hash {
    template<size_t sbits> struct _SHA2Config;
    template<> struct _SHA2Config<512> {
      using word = uint64_t;
      static constexpr size_t rounds = 80;
      static const word K[rounds];
      static constexpr uint32_t shifts[] = {
        28, 34, 39, 14, 18, 41,
         1,  8,  7, 19, 61,  6,
      };
    };
    template<> struct _SHA2Config<256> {
      using word = uint32_t;
      static constexpr size_t rounds = 64;
      static const word K[rounds];
      static constexpr uint32_t shifts[] = {
         2, 13, 22,  6, 11, 25,
         7, 18,  3, 17, 19, 10,
      };
    };
    template<typename W, size_t N> struct _SHA2State {
      W h[8];
      W chunk[16];
      uint64_t msgbytes = 0;
      void _transform(const W (&K)[N], const uint32_t (&shifts)[12]);
      void _update(const void *ptr, size_t num, const W (&K)[N], const uint32_t (&shifts)[12]);
      void _final(const W (&K)[N], const uint32_t (&shifts)[12]);
    };

    template<size_t sbits, size_t dbits = sbits> class SHA2 {
    private:
      using Config = _SHA2Config<sbits>;
      using Word = typename Config::word;
      _SHA2State<Word, Config::rounds> state {};
    public:
      SHA2();
      inline void update(const void *ptr, size_t sz) { state._update(ptr, sz, Config::K, Config::shifts); }
      void final() { state._final(Config::K, Config::shifts); }
      typedef uint8_t Digest[dbits/8] __attribute__((aligned(16)));
      const Digest& digest() const { return reinterpret_cast<const Digest&>(state.h); };
    } __attribute__((aligned(16)));
    using SHA512 = SHA2<512>;
    using SHA384 = SHA2<512, 384>;
    using SHA512_256 = SHA2<512, 256>;
    using SHA512_224 = SHA2<512, 224>;
    using SHA256 = SHA2<256>;
    using SHA224 = SHA2<256, 224>;
  }
}
