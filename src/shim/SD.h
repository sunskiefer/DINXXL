// In-memory SD card for the firmware's File/SD API. The "card" is saved with the host project.
#pragma once
#include <Arduino.h>
#include <map>
#include <memory>
#include <vector>
#include <string>

typedef int oflag_t;
#define FILE_READ 0
#define FILE_WRITE 1

namespace vfs {
struct Node { std::vector<uint8_t> data; };
std::map<std::string, std::shared_ptr<Node>>& files();
std::vector<std::string>& dirs();
}

class File {
 public:
  File() {}
  operator bool() const { return open_; }
  bool operator!() const { return !open_; }
  size_t size() const { return isDir_ ? 0 : (node_ ? node_->data.size() : 0); }
  int available() const { return node_ && pos_ < node_->data.size() ? (int)(node_->data.size() - pos_) : 0; }
  int read() { if (!node_ || pos_ >= node_->data.size()) return -1; return node_->data[pos_++]; }
  int peek() { if (!node_ || pos_ >= node_->data.size()) return -1; return node_->data[pos_]; }
  int read(void* buf, size_t n);
  size_t write(const uint8_t* b, size_t n);
  size_t write(uint8_t b) { return write(&b, 1); }
  size_t write(const char* s) { return write((const uint8_t*)s, strlen(s)); }
  bool seek(uint32_t p) { pos_ = p; return true; }
  bool seekSet(uint32_t p) { pos_ = p; return true; }
  uint32_t position() const { return (uint32_t)pos_; }
  void flush() {}
  void close() { open_ = false; node_.reset(); }
  bool isDirectory() const { return isDir_; }
  bool isDir() const { return isDir_; }
  String getName() const { return String(name_.c_str()); }
  size_t getName(char* b, size_t n) const { strncpy(b, name_.c_str(), n); b[n-1] = 0; return strlen(b); }
  String name() const { return String(name_.c_str()); }
  File openNextFile();
  void rewindDirectory() { idx_ = 0; }
 private:
  friend class BlastBeatsSDClass;
  bool open_ = false, isDir_ = false, write_ = false;
  std::shared_ptr<vfs::Node> node_;
  size_t pos_ = 0, idx_ = 0;
  std::string name_, path_;
};

class BlastBeatsSDClass {
 public:
  bool begin(uint8_t) { return true; }
  bool begin(uint32_t, uint8_t) { return true; }
  void end() {}
  File open(const char* path, oflag_t mode = FILE_READ);
  File open(const String& p, oflag_t m = FILE_READ) { return open(p.c_str(), m); }
  bool exists(const char* path);
  bool exists(const String& p) { return exists(p.c_str()); }
  bool remove(const char* path);
  bool remove(const String& p) { return remove(p.c_str()); }
  bool rename(const char* a, const char* b);
  bool mkdir(const char* path);
  bool rmdir(const char* path);
};
extern BlastBeatsSDClass SD;
