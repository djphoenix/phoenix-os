#include "crypto.hpp"
#include "kernlib.hpp"

static inline uint64_t ror(uint64_t a, uint64_t cnt) { asm volatile("rorq %1, %q0":"+r"(a):"i"(cnt)); return a; }
static inline uint64_t revb(uint64_t a) { asm volatile("bswap %q0":"+r"(a)); return a; }

constexpr uint64_t kcrypto::hash::SHA512::K[80] {
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

void kcrypto::hash::SHA512::_transform() {
  uint64_t w[80] __attribute__((aligned(16)));
  uint64_t a, b, c, d, e, f, g, h;
  uint64_t s0, s1, ch, maj, temp1, temp2;

  Memory::copy_aligned(w, state.chunk, sizeof(state.chunk));
  for (auto &i : w) i = revb(i);

  for (size_t i = 16; i < 80; i++) {
    s0 = ror(w[i-15],  1) ^ ror(w[i-15],  8) ^ (w[i-15] >> 7);
    s1 = ror(w[i- 2], 19) ^ ror(w[i- 2], 61) ^ (w[i- 2] >> 6);
    w[i] = w[i-16] + s0 + w[i-7] + s1;
  }

  a = state.h[0];
  b = state.h[1];
  c = state.h[2];
  d = state.h[3];
  e = state.h[4];
  f = state.h[5];
  g = state.h[6];
  h = state.h[7];

  for (size_t i = 0; i < 80; i++) {
    s1 = ror(e, 14) ^ ror(e, 18) ^ ror(e, 41);
    ch = (e & f) ^ ((~e) & g);
    temp1 = h + s1 + ch + K[i] + w[i];
    s0 = ror(a, 28) ^ ror(a, 34) ^ ror(a, 39);
    maj = (a & b) ^ (a & c) ^ (b & c);
    temp2 = s0 + maj;
 
    h = g;
    g = f;
    f = e;
    e = d + temp1;
    d = c;
    c = b;
    b = a;
    a = temp1 + temp2;
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

void kcrypto::hash::SHA512::update(const void *ptr, size_t num) {
  size_t byte = (state.bits & 0x3FF) >> 3;
  while (num) {
    size_t chunksize = klib::min(num, sizeof(state.chunk) - byte);
    Memory::copy(state.chunk + byte, ptr, chunksize);
    state.bits += chunksize << 3;
    byte += chunksize;
    num -= chunksize;
    if (byte == sizeof(state.chunk)) {
      _transform();
      byte = 0;
    }
  }
}

void kcrypto::hash::SHA512::final() {
  uint64_t msgbits = revb(state.bits);
  size_t byte = (state.bits & 0x3FF) >> 3;
  state.chunk[byte++] = 0x80; state.bits += 8;
  while (byte != (1024-128)/8) {
    state.chunk[byte++] = 0x00; state.bits += 8;
    if (byte == sizeof(state.chunk)) {
      _transform();
      byte = 0;
    }
  }
  Memory::zero(state.chunk + byte, 8); byte += 8; state.bits += 64;
  Memory::copy(state.chunk + byte, &msgbits, sizeof(msgbits)); byte += 8; state.bits += 64;
  _transform();
  for (auto &h : state.h) h = revb(h);
}

void kcrypto::hash::SHA512::digest(Digest &digest) const {
  Memory::copy(digest, state.h, sizeof(state.h));
}
