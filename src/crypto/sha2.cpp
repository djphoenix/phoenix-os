#include "crypto/sha2.hpp"

using namespace kcrypto::hash;

inline uint64_t ror(uint64_t a, uint32_t cnt) { asm volatile("ror %1, %0":"+r"(a):"i"(cnt)); return a; }
inline uint64_t revb(uint64_t a) { asm volatile("bswap %q0":"+r"(a)); return a; }

inline uint32_t ror(uint32_t a, uint32_t cnt) { asm volatile("ror %1, %0":"+r"(a):"i"(cnt)); return a; }
inline uint32_t revb(uint32_t a) { asm volatile("bswap %0":"+r"(a)); return a; }

const uint64_t _SHA2Config<512>::K[80] {
  0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc, 0x3956c25bf348b538, 
  0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118, 0xd807aa98a3030242, 0x12835b0145706fbe, 
  0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2, 0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235, 
  0xc19bf174cf692694, 0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65, 
  0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5, 0x983e5152ee66dfab, 
  0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4, 0xc6e00bf33da88fc2, 0xd5a79147930aa725, 
  0x06ca6351e003826f, 0x142929670a0e6e70, 0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 
  0x53380d139d95b3df, 0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b, 
  0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30, 0xd192e819d6ef5218, 
  0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8, 0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 
  0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8, 0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373, 
  0x682e6ff3d6b2b8a3, 0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec, 
  0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b, 0xca273eceea26619c, 
  0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178, 0x06f067aa72176fba, 0x0a637dc5a2c898a6, 
  0x113f9804bef90dae, 0x1b710b35131c471b, 0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc, 
  0x431d67c49c100d4c, 0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817,
};

const uint32_t _SHA2Config<256>::K[64] {
   0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
   0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
   0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
   0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
   0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
   0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
   0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
   0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

template<>
SHA512::SHA2() : state { .h = {
  0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
  0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0x1f83d9abfb41bd6b, 0x5be0cd19137e2179,
} } {}

template<>
SHA384::SHA2() : state { .h = {
  0xcbbb9d5dc1059ed8, 0x629a292a367cd507, 0x9159015a3070dd17, 0x152fecd8f70e5939, 
  0x67332667ffc00b31, 0x8eb44a8768581511, 0xdb0c2e0d64f98fa7, 0x47b5481dbefa4fa4,
} } {}

template<>
SHA512_256::SHA2() : state { .h = {
  0x22312194FC2BF72C, 0x9F555FA3C84C64C2, 0x2393B86B6F53B151, 0x963877195940EABD,
  0x96283EE2A88EFFE3, 0xBE5E1E2553863992, 0x2B0199FC2C85B8AA, 0x0EB72DDC81C52CA2,
} } {}

template<>
SHA512_224::SHA2() : state { .h = {
  0x8C3D37C819544DA2, 0x73E1996689DCD4D6, 0x1DFAB7AE32FF9C82, 0x679DD514582F9FCF,
  0x0F6D2B697BD44DA8, 0x77E36F7304C48942, 0x3F9D85A86A1D36C8, 0x1112E6AD91D692A1,
} } {}

template<>
SHA256::SHA2() : state { .h = {
  0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
  0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
} } {}

template<>
SHA224::SHA2() : state { .h = {
  0xC1059ED8, 0x367CD507, 0x3070DD17, 0xF70E5939,
  0xFFC00B31, 0x68581511, 0x64F98FA7, 0xBEFA4FA4,
} } {}

template<typename C>
void _SHA2Op<C>::_transform(State &state) {
  using W = typename C::word;
  W w[16] __attribute__((aligned(16)));
  W a, b, c, d, e, f, g, h, wi;
  W s0, s1, ch, maj, temp1;
  size_t i;

  a = state.h[0];
  b = state.h[1];
  c = state.h[2];
  d = state.h[3];
  e = state.h[4];
  f = state.h[5];
  g = state.h[6];
  h = state.h[7];

  const W *chunk64 = reinterpret_cast<const W*>(state.chunk);
  for (i = 0; i < C::rounds; i++) {
    if (i < 16) {
      w[i] = wi = revb(chunk64[i]);
    } else {
      s0 = ror(w[(i-15)%16], C::shifts[6]) ^ ror(w[(i-15)%16], C::shifts[ 7]) ^ (w[(i-15)%16] >> C::shifts[ 8]);
      s1 = ror(w[(i- 2)%16], C::shifts[9]) ^ ror(w[(i- 2)%16], C::shifts[10]) ^ (w[(i- 2)%16] >> C::shifts[11]);
      w[i%16] = wi = w[(i-16)%16] + s0 + w[(i-7)%16] + s1;
    }

    s0 = ror(a, C::shifts[0]) ^ ror(a, C::shifts[1]) ^ ror(a, C::shifts[2]);
    s1 = ror(e, C::shifts[3]) ^ ror(e, C::shifts[4]) ^ ror(e, C::shifts[5]);

    ch = (e & f) ^ ((~e) & g);
    maj = (a & b) ^ (a & c) ^ (b & c);

    temp1 = h + s1 + ch + C::K[i] + wi;
 
    h = g;
    g = f;
    f = e;
    e = d + temp1;
    d = c;
    c = b;
    b = a;
    a = temp1 + s0 + maj;
  }

  state.h[0] += a;
  state.h[1] += b;
  state.h[2] += c;
  state.h[3] += d;
  state.h[4] += e;
  state.h[5] += f;
  state.h[6] += g;
  state.h[7] += h;
}

template<typename C>
void _SHA2Op<C>::_update(State &state, const void *ptr, size_t num) {
  size_t byte = state.msgbytes % sizeof(state.chunk);
  const uint8_t *bytes = reinterpret_cast<const uint8_t*>(ptr);
  uint8_t *chunkb = reinterpret_cast<uint8_t*>(state.chunk);
  while (num--) {
    chunkb[byte++] = *(bytes++);
    state.msgbytes++;
    if (byte == sizeof(state.chunk)) {
      _transform(state);
      byte = 0;
    }
  }
}

static constexpr uint8_t fb1 = 0x80, fb0 = 0x00;

template<typename C>
void _SHA2Op<C>::_final(State &state) {
  uint64_t msgbits = revb(state.msgbytes << 3);
  _update(state, &fb1, 1);
  while ((state.msgbytes % sizeof(state.chunk)) != (sizeof(state.chunk) - 8)) _update(state, &fb0, 1);
  reinterpret_cast<uint32_t*>(state.chunk)[(sizeof(state.chunk) - 8) / 4] = uint32_t(msgbits);
  reinterpret_cast<uint32_t*>(state.chunk)[(sizeof(state.chunk) - 4) / 4] = uint32_t(msgbits >> 32);
  _transform(state);
  state.msgbytes = 0;
  for (auto &i : state.h) i = revb(i);
}

template struct kcrypto::hash::_SHA2Op<_SHA2Config<512>>;
template struct kcrypto::hash::_SHA2Op<_SHA2Config<256>>;
