#include "vgmStream.h"

#include <stdlib.h>
#include <string.h>

#include <miniz_tinfl.h>

VGMStream::VGMStream()
    : type_(Type::kNone),
      data_offset_(0),
      current_pos_(0),
      eof_(false),
      error_(false),
      dict_(nullptr),
      comp_size_(0),
      comp_pos_(0),
      comp_eof_(false),
      inflate_done_(false),
      dict_ofs_(0),
      pending_pos_(0),
      pending_size_(0),
      out_size_(0),
      out_pos_(0) {
}

VGMStream::~VGMStream() {
  close();
}

bool VGMStream::open(const String& path) {
  close();

  path_ = path;
  String lower = path;
  lower.toLowerCase();

  if (lower.endsWith(".vgz")) {
    type_ = Type::kGzip;
  } else {
    type_ = Type::kRaw;
  }

  file_ = SD.open(path.c_str());
  if (!file_) {
    type_ = Type::kNone;
    return false;
  }

  current_pos_ = 0;
  eof_ = false;
  error_ = false;

  if (type_ == Type::kGzip) {
    if (!parseGzipHeader()) {
      error_ = true;
      file_.close();
      type_ = Type::kNone;
      return false;
    }
    dict_ = static_cast<uint8_t*>(malloc(TINFL_LZ_DICT_SIZE));
    if (!dict_) {
      error_ = true;
      file_.close();
      type_ = Type::kNone;
      return false;
    }
    tinfl_init(&decomp_);
    comp_size_ = 0;
    comp_pos_ = 0;
    comp_eof_ = false;
    inflate_done_ = false;
    dict_ofs_ = 0;
    pending_pos_ = 0;
    pending_size_ = 0;
    out_size_ = 0;
    out_pos_ = 0;
    memset(dict_, 0, TINFL_LZ_DICT_SIZE);
  } else {
    data_offset_ = 0;
  }

  return true;
}

void VGMStream::close() {
  if (file_) {
    file_.close();
  }
  if (dict_) {
    free(dict_);
    dict_ = nullptr;
  }
  type_ = Type::kNone;
  path_ = "";
  data_offset_ = 0;
  current_pos_ = 0;
  eof_ = false;
  error_ = false;
  comp_size_ = 0;
  comp_pos_ = 0;
  comp_eof_ = false;
  inflate_done_ = false;
  dict_ofs_ = 0;
  pending_pos_ = 0;
  pending_size_ = 0;
  out_size_ = 0;
  out_pos_ = 0;
}

bool VGMStream::seek(uint32_t offset) {
  if (type_ == Type::kNone) {
    return false;
  }

  if (type_ == Type::kRaw) {
    if (!file_.seek(offset)) {
      return false;
    }
    current_pos_ = offset;
    return true;
  }

  if (!resetToStart()) {
    return false;
  }

  return skip(offset);
}

int VGMStream::read() {
  if (type_ == Type::kNone) {
    return -1;
  }

  if (type_ == Type::kRaw) {
    int value = file_.read();
    if (value >= 0) {
      current_pos_++;
    } else {
      eof_ = true;
    }
    return value;
  }

  if (out_pos_ >= out_size_) {
    if (!refillOutput()) {
      return -1;
    }
  }

  int byte_value = out_buf_[out_pos_++];
  current_pos_++;
  if (out_pos_ >= out_size_ && inflate_done_ && pending_size_ == 0) {
    eof_ = true;
  }
  return byte_value & 0xFF;
}

size_t VGMStream::readBytes(uint8_t* dest, size_t len) {
  if (type_ == Type::kNone) {
    return 0;
  }

  if (type_ == Type::kRaw) {
    size_t read_count = file_.read(dest, len);
    current_pos_ += read_count;
    if (read_count < len) {
      eof_ = true;
    }
    return read_count;
  }

  size_t total = 0;
  while (total < len) {
    if (out_pos_ >= out_size_) {
      if (!refillOutput()) {
        break;
      }
    }
    size_t remaining = len - total;
    size_t available = out_size_ - out_pos_;
    size_t chunk = remaining < available ? remaining : available;
    memcpy(dest + total, out_buf_ + out_pos_, chunk);
    out_pos_ += chunk;
    current_pos_ += chunk;
    total += chunk;
  }
  if (total < len && inflate_done_ && pending_size_ == 0) {
    eof_ = true;
  }
  return total;
}

bool VGMStream::resetToStart() {
  if (type_ == Type::kNone) {
    return false;
  }

  if (type_ == Type::kRaw) {
    if (!file_.seek(0)) {
      return false;
    }
  } else {
    if (!file_.seek(data_offset_)) {
      return false;
    }
    if (!dict_) {
      return false;
    }
    tinfl_init(&decomp_);
    comp_size_ = 0;
    comp_pos_ = 0;
    comp_eof_ = false;
    inflate_done_ = false;
    dict_ofs_ = 0;
    pending_pos_ = 0;
    pending_size_ = 0;
    out_size_ = 0;
    out_pos_ = 0;
    eof_ = false;
    error_ = false;
    memset(dict_, 0, TINFL_LZ_DICT_SIZE);
  }
  current_pos_ = 0;
  return true;
}

bool VGMStream::skip(uint32_t bytes) {
  if (type_ == Type::kNone) {
    return false;
  }

  if (type_ == Type::kRaw) {
    if (!file_.seek(current_pos_ + bytes)) {
      return false;
    }
    current_pos_ += bytes;
    return true;
  }

  while (bytes > 0) {
    int value = read();
    if (value < 0) {
      return false;
    }
    bytes--;
  }
  return true;
}

bool VGMStream::parseGzipHeader() {
  uint8_t fixed_header[10];
  if (file_.read(fixed_header, sizeof(fixed_header)) != sizeof(fixed_header)) {
    return false;
  }
  if (fixed_header[0] != 0x1F || fixed_header[1] != 0x8B || fixed_header[2] != 0x08) {
    return false;
  }

  uint8_t flags = fixed_header[3];

  if (flags & 0x04) {  // FEXTRA
    uint8_t len_bytes[2];
    if (file_.read(len_bytes, 2) != 2) {
      return false;
    }
    uint16_t extra_len = static_cast<uint16_t>(len_bytes[0]) | (static_cast<uint16_t>(len_bytes[1]) << 8);
    while (extra_len--) {
      if (file_.read() < 0) {
        return false;
      }
    }
  }

  if (flags & 0x08) {  // FNAME
    int c = 0;
    do {
      c = file_.read();
      if (c < 0) {
        return false;
      }
    } while (c != 0);
  }

  if (flags & 0x10) {  // FCOMMENT
    int c = 0;
    do {
      c = file_.read();
      if (c < 0) {
        return false;
      }
    } while (c != 0);
  }

  if (flags & 0x02) {  // FHCRC
    if (file_.read(fixed_header, 2) != 2) {
      return false;
    }
  }

  data_offset_ = static_cast<uint32_t>(file_.position());
  return true;
}

bool VGMStream::refillCompressed() {
  if (comp_eof_) {
    return false;
  }

  comp_size_ = file_.read(comp_buf_, kCompBufSize);
  comp_pos_ = 0;
  if (comp_size_ == 0) {
    comp_eof_ = true;
    return false;
  }
  return true;
}

bool VGMStream::copyPendingOutput() {
  out_pos_ = 0;
  out_size_ = 0;

  if (pending_size_ == 0) {
    return false;
  }
  if (!dict_) {
    error_ = true;
    return false;
  }

  size_t chunk = pending_size_ < kOutBufSize ? pending_size_ : kOutBufSize;
  size_t untilEnd = TINFL_LZ_DICT_SIZE - pending_pos_;
  if (chunk > untilEnd) {
    chunk = untilEnd;
  }

  memcpy(out_buf_, dict_ + pending_pos_, chunk);
  pending_pos_ = (pending_pos_ + chunk) & (TINFL_LZ_DICT_SIZE - 1);
  pending_size_ -= chunk;
  out_size_ = chunk;
  return true;
}

bool VGMStream::refillOutput() {
  if (type_ != Type::kGzip || error_) {
    return false;
  }
  if (!dict_) {
    error_ = true;
    return false;
  }

  out_pos_ = 0;
  out_size_ = 0;

  if (copyPendingOutput()) {
    return true;
  }

  if (inflate_done_) {
    eof_ = true;
    return false;
  }

  while (out_size_ == 0) {
    if (comp_pos_ >= comp_size_ && !comp_eof_) {
      if (!refillCompressed()) {
        // No more compressed data, signal end to decompressor.
      }
    }

    size_t in_bytes = (comp_pos_ < comp_size_) ? (comp_size_ - comp_pos_) : 0;
    if (!in_bytes && comp_eof_) {
      // Allow tinfl to finish with empty input buffer.
    } else if (!in_bytes) {
      // Need more data to continue.
      if (!refillCompressed()) {
        break;
      }
      in_bytes = comp_size_ - comp_pos_;
    }

    size_t out_bytes = TINFL_LZ_DICT_SIZE - dict_ofs_;
    const mz_uint8* in_ptr = (in_bytes > 0) ? (comp_buf_ + comp_pos_) : comp_buf_;

    mz_uint32 flags = 0;
    if (!comp_eof_) {
      flags |= TINFL_FLAG_HAS_MORE_INPUT;
    }

    tinfl_status status = tinfl_decompress(&decomp_, in_ptr, &in_bytes,
                                           dict_,
                                           dict_ + dict_ofs_,
                                           &out_bytes, flags);
    comp_pos_ += in_bytes;

    if (status < 0) {
      error_ = true;
      return false;
    }

    if (out_bytes > 0) {
      pending_pos_ = dict_ofs_;
      pending_size_ = out_bytes;
      dict_ofs_ = (dict_ofs_ + out_bytes) & (TINFL_LZ_DICT_SIZE - 1);
      copyPendingOutput();
    }

    if (status == TINFL_STATUS_DONE) {
      inflate_done_ = true;
      if (out_size_ == 0 && pending_size_ == 0) {
        eof_ = true;
        return false;
      }
      return true;
    }

    if (out_size_ > 0) {
      return true;
    }

    if (comp_eof_ && status == TINFL_STATUS_NEEDS_MORE_INPUT) {
      eof_ = true;
      return false;
    }
  }

  return out_size_ > 0;
}
