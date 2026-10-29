// Small LZSS packer and base64, for the project state text (the host hands the plugin an 8 KB text chunk).
#pragma once
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>

namespace pack {

// Format: 4-byte little-endian raw length, then groups of 8 items led by a flag byte (bit set = literal byte,
// clear = 2-byte match: 12-bit distance, 4-bit length-3 (3..18)).
inline std::string lzss(const std::string& in) {
  std::string out;
  uint32_t n = (uint32_t)in.size();
  out.append((const char*)&n, 4);
  const int WIN = 4095, MINM = 3, MAXM = 18, HB = 14, CHAIN = 24;
  std::vector<int> head(1 << HB, -1), prev(in.size() ? in.size() : 1, -1);
  auto hash = [&](size_t i) { return (((uint8_t)in[i] << 6) ^ ((uint8_t)in[i + 1] << 3) ^ (uint8_t)in[i + 2]) & ((1 << HB) - 1); };
  size_t i = 0;
  while (i < in.size()) {
    size_t flag_pos = out.size();
    out.push_back(0);
    uint8_t flags = 0;
    for (int b = 0; b < 8 && i < in.size(); b++) {
      int best_len = 0, best_dist = 0;
      if (i + MINM <= in.size()) {
        int h = hash(i), c = head[h], tries = 0;
        while (c >= 0 && (int)i - c <= WIN && tries++ < CHAIN) {
          int l = 0;
          while (l < MAXM && i + l < in.size() && in[c + l] == in[i + l]) l++;
          if (l > best_len) { best_len = l; best_dist = (int)i - c; if (l == MAXM) break; }
          c = prev[c];
        }
      }
      size_t adv = 1;
      if (best_len >= MINM) {
        out.push_back((char)(best_dist & 0xFF));
        out.push_back((char)(((best_dist >> 8) & 0x0F) | ((best_len - MINM) << 4)));
        adv = best_len;
      } else {
        flags |= (uint8_t)(1 << b);
        out.push_back(in[i]);
      }
      for (size_t k = 0; k < adv; k++) {
        if (i + k + MINM <= in.size()) { int h = hash(i + k); prev[i + k] = head[h]; head[h] = (int)(i + k); }
      }
      i += adv;
    }
    out[flag_pos] = (char)flags;
  }
  return out;
}

inline bool unlzss(const std::string& in, std::string& out) {
  out.clear();
  if (in.size() < 4) return false;
  uint32_t n;
  memcpy(&n, in.data(), 4);
  if (n > (64u << 20)) return false;
  out.reserve(n);
  size_t p = 4;
  while (out.size() < n && p < in.size()) {
    uint8_t flags = (uint8_t)in[p++];
    for (int b = 0; b < 8 && out.size() < n; b++) {
      if (flags & (1 << b)) {
        if (p >= in.size()) return false;
        out.push_back(in[p++]);
      } else {
        if (p + 1 >= in.size()) return false;
        int d = (uint8_t)in[p] | ((uint8_t)in[p + 1] & 0x0F) << 8;
        int l = ((uint8_t)in[p + 1] >> 4) + 3;
        p += 2;
        if (d == 0 || (size_t)d > out.size()) return false;
        for (int k = 0; k < l && out.size() < n; k++) out.push_back(out[out.size() - d]);
      }
    }
  }
  return out.size() == n;
}

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
inline std::string b64(const std::string& in) {
  std::string o;
  size_t i = 0;
  for (; i + 2 < in.size(); i += 3) {
    uint32_t v = ((uint8_t)in[i] << 16) | ((uint8_t)in[i + 1] << 8) | (uint8_t)in[i + 2];
    o += B64[v >> 18]; o += B64[(v >> 12) & 63]; o += B64[(v >> 6) & 63]; o += B64[v & 63];
  }
  if (i + 1 == in.size()) {
    uint32_t v = (uint8_t)in[i] << 16;
    o += B64[v >> 18]; o += B64[(v >> 12) & 63]; o += "==";
  } else if (i + 2 == in.size()) {
    uint32_t v = ((uint8_t)in[i] << 16) | ((uint8_t)in[i + 1] << 8);
    o += B64[v >> 18]; o += B64[(v >> 12) & 63]; o += B64[(v >> 6) & 63]; o += '=';
  }
  return o;
}
inline bool unb64(const char* s, std::string& out) {
  out.clear();
  uint32_t v = 0;
  int bits = 0;
  for (; *s; s++) {
    int c = *s, d;
    if (c >= 'A' && c <= 'Z') d = c - 'A';
    else if (c >= 'a' && c <= 'z') d = c - 'a' + 26;
    else if (c >= '0' && c <= '9') d = c - '0' + 52;
    else if (c == '+') d = 62;
    else if (c == '/') d = 63;
    else if (c == '=') break;
    else return false;
    v = (v << 6) | d;
    bits += 6;
    if (bits >= 8) { bits -= 8; out.push_back((char)((v >> bits) & 0xFF)); }
  }
  return true;
}
}  // namespace pack
