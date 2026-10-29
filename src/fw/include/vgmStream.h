#pragma once

#include <Arduino.h>
#include <SD.h>

#include <miniz_tinfl.h>

class VGMStream {
 public:
  VGMStream();
  ~VGMStream();

  bool open(const String& path);
  void close();

  bool seek(uint32_t offset);
  int read();
  size_t readBytes(uint8_t* dest, size_t len);

  bool good() const { return !error_; }
  bool eof() const { return eof_ && out_pos_ >= out_size_ && pending_size_ == 0; }
  bool isCompressed() const { return type_ == Type::kGzip; }

 private:
  enum class Type { kNone, kRaw, kGzip };

  bool resetToStart();
  bool skip(uint32_t bytes);
  bool parseGzipHeader();
  bool refillOutput();
  bool refillCompressed();
  bool copyPendingOutput();

  Type type_;
  String path_;
  File file_;

  // gzip state
  uint32_t data_offset_;
  uint32_t current_pos_;
  bool eof_;
  bool error_;
  uint8_t* dict_;

  tinfl_decompressor decomp_;

  static const size_t kCompBufSize = 512;
  static const size_t kOutBufSize = 512;

  uint8_t comp_buf_[kCompBufSize];
  size_t comp_size_;
  size_t comp_pos_;
  bool comp_eof_;
  bool inflate_done_;
  size_t dict_ofs_;
  size_t pending_pos_;
  size_t pending_size_;

  uint8_t out_buf_[kOutBufSize];
  size_t out_size_;
  size_t out_pos_;
};
