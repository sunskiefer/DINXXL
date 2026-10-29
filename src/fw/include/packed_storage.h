#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "scene_types.h"

/**
 * Helpers for storing small unsigned values in packed memory layouts.
 *
 * All helpers below provide proxy/reference types so they can largely mimic the
 * behaviour of plain C-style arrays. They are intentionally conservative and
 * avoid dynamic allocation so they remain friendly to tightly constrained MCU
 * builds.
 */

// ---------- PackedNibbleArray -------------------------------------------------

template <size_t Length>
class PackedNibbleArray {
 public:
  class Reference {
   public:
    Reference(PackedNibbleArray& owner, size_t index)
        : owner_(owner), index_(index) {}

    Reference& operator=(uint8_t value) {
      owner_.set(index_, value);
      return *this;
    }

    Reference& operator=(const Reference& other) {
      return (*this = static_cast<uint8_t>(other));
    }

    operator uint8_t() const { return owner_.get(index_); }

   private:
    PackedNibbleArray& owner_;
    size_t index_;
  };

  PackedNibbleArray() { clear(); }

  void clear() { memset(storage_, 0, sizeof(storage_)); }

  uint8_t get(size_t index) const {
    const size_t byteIndex = index >> 1;
    const bool high = (index & 0x1u) != 0;
    const uint8_t byte = storage_[byteIndex];
    return high ? static_cast<uint8_t>(byte >> 4) : static_cast<uint8_t>(byte & 0x0Fu);
  }

  void set(size_t index, uint8_t value) {
    const size_t byteIndex = index >> 1;
    const bool high = (index & 0x1u) != 0;
    uint8_t& byte = storage_[byteIndex];
    if (high) {
      byte = static_cast<uint8_t>((byte & 0x0Fu) | ((value & 0x0Fu) << 4));
    } else {
      byte = static_cast<uint8_t>((byte & 0xF0u) | (value & 0x0Fu));
    }
  }

  Reference operator[](size_t index) { return Reference(*this, index); }
  uint8_t operator[](size_t index) const { return get(index); }

  uint8_t* raw() { return storage_; }
  const uint8_t* raw() const { return storage_; }

 private:
  static constexpr size_t kStorageBytes = (Length + 1) / 2;
  uint8_t storage_[kStorageBytes];
};

// ---------- PackedBitset ------------------------------------------------------

template <size_t Length>
class PackedBitset {
 public:
  class Reference {
   public:
    Reference(PackedBitset& owner, size_t index)
        : owner_(owner), index_(index) {}

    Reference& operator=(bool value) {
      owner_.set(index_, value);
      return *this;
    }

    Reference& operator=(const Reference& other) {
      return (*this = static_cast<bool>(other));
    }

    operator bool() const { return owner_.get(index_); }

   private:
    PackedBitset& owner_;
    size_t index_;
  };

  PackedBitset() { clear(); }

  void clear() { memset(storage_, 0, sizeof(storage_)); }

  bool get(size_t index) const {
    const size_t byteIndex = index >> 3;
    const uint8_t bitMask = static_cast<uint8_t>(1u << (index & 0x7u));
    return (storage_[byteIndex] & bitMask) != 0;
  }

  void set(size_t index, bool value) {
    const size_t byteIndex = index >> 3;
    const uint8_t bitMask = static_cast<uint8_t>(1u << (index & 0x7u));
    uint8_t& byte = storage_[byteIndex];
    if (value) {
      byte = static_cast<uint8_t>(byte | bitMask);
    } else {
      byte = static_cast<uint8_t>(byte & ~bitMask);
    }
  }

  Reference operator[](size_t index) { return Reference(*this, index); }
  bool operator[](size_t index) const { return get(index); }

  uint8_t* raw() { return storage_; }
  const uint8_t* raw() const { return storage_; }

 private:
  static constexpr size_t kStorageBytes = (Length + 7) / 8;
  uint8_t storage_[kStorageBytes];
};

// ---------- PackedNibbleMatrix (2D) -------------------------------------------

template <size_t Rows, size_t Cols>
class PackedNibbleMatrix {
 public:
  class RowProxy {
   public:
    RowProxy(PackedNibbleMatrix& owner, size_t row) : owner_(owner), row_(row) {}

    typename PackedNibbleArray<Rows * Cols>::Reference operator[](size_t col) {
      return owner_.values_[row_ * Cols + col];
    }

    uint8_t operator[](size_t col) const {
      return owner_.values_.get(row_ * Cols + col);
    }

   private:
    PackedNibbleMatrix& owner_;
    size_t row_;
  };

  class ConstRowProxy {
   public:
    ConstRowProxy(const PackedNibbleMatrix& owner, size_t row)
        : owner_(owner), row_(row) {}

    uint8_t operator[](size_t col) const {
      return owner_.values_.get(row_ * Cols + col);
    }

   private:
    const PackedNibbleMatrix& owner_;
    size_t row_;
  };

  PackedNibbleMatrix() { values_.clear(); }

  RowProxy operator[](size_t row) { return RowProxy(*this, row); }
  ConstRowProxy operator[](size_t row) const { return ConstRowProxy(*this, row); }

  void clear() { values_.clear(); }

  uint8_t get(size_t row, size_t col) const {
    return values_.get(row * Cols + col);
  }

  void set(size_t row, size_t col, uint8_t value) {
    values_.set(row * Cols + col, value);
  }

  void clearRow(size_t row) {
    const size_t base = row * Cols;
    for (size_t col = 0; col < Cols; ++col) {
      values_.set(base + col, 0);
    }
  }

  void copyRow(size_t dstRow, size_t srcRow) {
    if (dstRow == srcRow) {
      return;
    }
    const size_t dstBase = dstRow * Cols;
    const size_t srcBase = srcRow * Cols;
    for (size_t col = 0; col < Cols; ++col) {
      values_.set(dstBase + col, values_.get(srcBase + col));
    }
  }

  uint8_t* raw() { return values_.raw(); }
  const uint8_t* raw() const { return values_.raw(); }

 private:
  PackedNibbleArray<Rows * Cols> values_;
};

// ---------- PackedBitMatrix (2D) ----------------------------------------------

template <size_t Rows, size_t Cols>
class PackedBitMatrix {
 public:
  class RowProxy {
   public:
    RowProxy(PackedBitMatrix& owner, size_t row) : owner_(owner), row_(row) {}

    typename PackedBitset<Rows * Cols>::Reference operator[](size_t col) {
      return owner_.bits_[row_ * Cols + col];
    }

    operator const PackedBitMatrix&() const = delete;

    bool operator[](size_t col) const {
      return owner_.bits_.get(row_ * Cols + col);
    }

   private:
    PackedBitMatrix& owner_;
    size_t row_;
  };

  class ConstRowProxy {
   public:
    ConstRowProxy(const PackedBitMatrix& owner, size_t row)
        : owner_(owner), row_(row) {}

    bool operator[](size_t col) const {
      return owner_.bits_.get(row_ * Cols + col);
    }

   private:
    const PackedBitMatrix& owner_;
    size_t row_;
  };

  PackedBitMatrix() { bits_.clear(); }

  RowProxy operator[](size_t row) { return RowProxy(*this, row); }
  ConstRowProxy operator[](size_t row) const { return ConstRowProxy(*this, row); }

  void clear() { bits_.clear(); }

  bool get(size_t row, size_t col) const { return bits_.get(row * Cols + col); }
  void set(size_t row, size_t col, bool value) {
    bits_.set(row * Cols + col, value);
  }

  void clearRow(size_t row) {
    const size_t base = row * Cols;
    for (size_t col = 0; col < Cols; ++col) {
      bits_.set(base + col, false);
    }
  }

  void copyRow(size_t dstRow, size_t srcRow) {
    if (dstRow == srcRow) {
      return;
    }
    const size_t dstBase = dstRow * Cols;
    const size_t srcBase = srcRow * Cols;
    for (size_t col = 0; col < Cols; ++col) {
      bits_.set(dstBase + col, bits_.get(srcBase + col));
    }
  }

 private:
  PackedBitset<Rows * Cols> bits_;
};

// ---------- RouteAutoStorage --------------------------------------------------

class RouteAutoStorage {
 public:
  static constexpr size_t kPatternCount = 16;
  static constexpr size_t kStepCount = 64;
  static constexpr size_t kLaneCount = 7;
  static constexpr size_t kEntryCount = kPatternCount * kStepCount * kLaneCount;

  class LaneRef {
   public:
    LaneRef(RouteAutoStorage& owner, size_t index)
        : owner_(owner), index_(index) {}

    LaneRef& operator=(uint8_t value) {
      owner_.set(index_, value);
      return *this;
    }

    LaneRef& operator=(const LaneRef& other) {
      return (*this = static_cast<uint8_t>(other));
    }

    operator uint8_t() const { return owner_.get(index_); }

   private:
    RouteAutoStorage& owner_;
    size_t index_;
  };

  class LaneConstRef {
   public:
    LaneConstRef(const RouteAutoStorage& owner, size_t index)
        : owner_(owner), index_(index) {}

    operator uint8_t() const { return owner_.get(index_); }

   private:
    const RouteAutoStorage& owner_;
    size_t index_;
  };

  class StepProxy {
   public:
    StepProxy(RouteAutoStorage& owner, size_t baseIndex)
        : owner_(owner), baseIndex_(baseIndex) {}

    LaneRef operator[](size_t lane) {
      return LaneRef(owner_, baseIndex_ + lane);
    }

    uint8_t operator[](size_t lane) const {
      return owner_.get(baseIndex_ + lane);
    }

   private:
    RouteAutoStorage& owner_;
    size_t baseIndex_;
  };

  class StepConstProxy {
   public:
    StepConstProxy(const RouteAutoStorage& owner, size_t baseIndex)
        : owner_(owner), baseIndex_(baseIndex) {}

    uint8_t operator[](size_t lane) const {
      return owner_.get(baseIndex_ + lane);
    }

   private:
    const RouteAutoStorage& owner_;
    size_t baseIndex_;
  };

  class PatternProxy {
   public:
    PatternProxy(RouteAutoStorage& owner, size_t pattern)
        : owner_(owner), pattern_(pattern) {}

    StepProxy operator[](size_t step) {
      return StepProxy(owner_, RouteAutoStorage::index(pattern_, step, 0));
    }

    uint8_t operator[](size_t step) const = delete;

   private:
    RouteAutoStorage& owner_;
    size_t pattern_;
  };

  class PatternConstProxy {
   public:
    PatternConstProxy(const RouteAutoStorage& owner, size_t pattern)
        : owner_(owner), pattern_(pattern) {}

    StepConstProxy operator[](size_t step) const {
      return StepConstProxy(owner_, RouteAutoStorage::index(pattern_, step, 0));
    }

   private:
    const RouteAutoStorage& owner_;
    size_t pattern_;
  };

  RouteAutoStorage() { clear(); }

  PatternProxy operator[](size_t pattern) {
    return PatternProxy(*this, pattern);
  }

  PatternConstProxy operator[](size_t pattern) const {
    return PatternConstProxy(*this, pattern);
  }

  void clear() {
    values_.clear();
    present_.clear();
  }

  void clearPattern(size_t pattern) {
    const size_t start = pattern * kStepCount * kLaneCount;
    const size_t end = start + (kStepCount * kLaneCount);
    for (size_t idx = start; idx < end; ++idx) {
      values_.set(idx, 0);
      present_.set(idx, false);
    }
  }

  void copyPattern(size_t dstPattern, size_t srcPattern) {
    if (dstPattern == srcPattern) {
      return;
    }
    const size_t srcStart = srcPattern * kStepCount * kLaneCount;
    const size_t dstStart = dstPattern * kStepCount * kLaneCount;
    for (size_t i = 0; i < kStepCount * kLaneCount; ++i) {
      const size_t srcIndex = srcStart + i;
      const size_t dstIndex = dstStart + i;
      values_.set(dstIndex, values_.get(srcIndex));
      present_.set(dstIndex, present_.get(srcIndex));
    }
  }

  void exportStep(size_t pattern, size_t step, uint8_t* out) const {
    for (size_t lane = 0; lane < kLaneCount; ++lane) {
      out[lane] = get(index(pattern, step, lane));
    }
  }

  void importStep(size_t pattern, size_t step, const uint8_t* in) {
    for (size_t lane = 0; lane < kLaneCount; ++lane) {
      set(index(pattern, step, lane), in[lane]);
    }
  }

 private:
  static constexpr size_t index(size_t pattern, size_t step, size_t lane) {
    return ((pattern * kStepCount) + step) * kLaneCount + lane;
  }

  uint8_t get(size_t flatIndex) const {
    if (!present_.get(flatIndex)) {
      return 0;
    }
    return static_cast<uint8_t>(values_.get(flatIndex) + 1);
  }

  void set(size_t flatIndex, uint8_t value) {
    if (value == 0) {
      present_.set(flatIndex, false);
      values_.set(flatIndex, 0);
    } else {
      present_.set(flatIndex, true);
      values_.set(flatIndex, static_cast<uint8_t>((value - 1u) & 0x0Fu));
    }
  }

  PackedNibbleArray<kEntryCount> values_;
  PackedBitset<kEntryCount> present_;
};

// ---------- RouteAutoUndoBuffer -----------------------------------------------

class RouteAutoUndoBuffer {
 public:
  static constexpr size_t kStepCount = 64;
  static constexpr size_t kLaneCount = 7;
  static constexpr size_t kEntryCount = kStepCount * kLaneCount;

  class LaneRef {
   public:
    LaneRef(RouteAutoUndoBuffer& owner, size_t index)
        : owner_(owner), index_(index) {}

    LaneRef& operator=(uint8_t value) {
      owner_.set(index_, value);
      return *this;
    }

    LaneRef& operator=(const LaneRef& other) {
      return (*this = static_cast<uint8_t>(other));
    }

    operator uint8_t() const { return owner_.get(index_); }

   private:
    RouteAutoUndoBuffer& owner_;
    size_t index_;
  };

  class LaneConstRef {
   public:
    LaneConstRef(const RouteAutoUndoBuffer& owner, size_t index)
        : owner_(owner), index_(index) {}

    operator uint8_t() const { return owner_.get(index_); }

   private:
    const RouteAutoUndoBuffer& owner_;
    size_t index_;
  };

  class StepProxy {
   public:
    StepProxy(RouteAutoUndoBuffer& owner, size_t baseIndex)
        : owner_(owner), baseIndex_(baseIndex) {}

    LaneRef operator[](size_t lane) {
      return LaneRef(owner_, baseIndex_ + lane);
    }

    uint8_t operator[](size_t lane) const {
      return owner_.get(baseIndex_ + lane);
    }

   private:
    RouteAutoUndoBuffer& owner_;
    size_t baseIndex_;
  };

  class StepConstProxy {
   public:
    StepConstProxy(const RouteAutoUndoBuffer& owner, size_t baseIndex)
        : owner_(owner), baseIndex_(baseIndex) {}

    uint8_t operator[](size_t lane) const {
      return owner_.get(baseIndex_ + lane);
    }

   private:
    const RouteAutoUndoBuffer& owner_;
    size_t baseIndex_;
  };

  RouteAutoUndoBuffer() { clear(); }

  StepProxy operator[](size_t step) {
    return StepProxy(*this, step * kLaneCount);
  }

  StepConstProxy operator[](size_t step) const {
    return StepConstProxy(*this, step * kLaneCount);
  }

  void clear() {
    values_.clear();
    present_.clear();
  }

 private:
  uint8_t get(size_t flatIndex) const {
    if (!present_.get(flatIndex)) {
      return 0;
    }
    return static_cast<uint8_t>(values_.get(flatIndex) + 1);
  }

  void set(size_t flatIndex, uint8_t value) {
    if (value == 0) {
      present_.set(flatIndex, false);
      values_.set(flatIndex, 0);
    } else {
      present_.set(flatIndex, true);
      values_.set(flatIndex, static_cast<uint8_t>((value - 1u) & 0x0Fu));
    }
  }

  PackedNibbleArray<kEntryCount> values_;
  PackedBitset<kEntryCount> present_;
};

// ---------- SceneBank ---------------------------------------------------------

class SceneBank {
 public:
  static constexpr size_t kPatternCount = 16;
  static constexpr size_t kSlotCount = 10;
  static constexpr size_t kEntryCount = kPatternCount * kSlotCount;
  static constexpr size_t kPackedPerScene =
      SCENE_VOLUME_COUNT + SCENE_MULT_COUNT + SCENE_DECAY_COUNT + 2;

  class SlotProxy {
   public:
    SlotProxy(SceneBank& owner, size_t pattern, size_t slot)
        : owner_(owner), pattern_(pattern), slot_(slot) {}

    SlotProxy& operator=(const SceneState& state) {
      owner_.store(pattern_, slot_, state);
      return *this;
    }

    operator SceneState() const {
      return owner_.load(pattern_, slot_);
    }

   private:
    SceneBank& owner_;
    size_t pattern_;
    size_t slot_;
  };

  class SlotConstProxy {
   public:
    SlotConstProxy(const SceneBank& owner, size_t pattern, size_t slot)
        : owner_(owner), pattern_(pattern), slot_(slot) {}

    operator SceneState() const {
      return owner_.load(pattern_, slot_);
    }

   private:
    const SceneBank& owner_;
    size_t pattern_;
    size_t slot_;
  };

  class PatternProxy {
   public:
    PatternProxy(SceneBank& owner, size_t pattern)
        : owner_(owner), pattern_(pattern) {}

    SlotProxy operator[](size_t slot) {
      return SlotProxy(owner_, pattern_, slot);
    }

   private:
    SceneBank& owner_;
    size_t pattern_;
  };

  class PatternConstProxy {
   public:
    PatternConstProxy(const SceneBank& owner, size_t pattern)
        : owner_(owner), pattern_(pattern) {}

    SlotConstProxy operator[](size_t slot) const {
      return SlotConstProxy(owner_, pattern_, slot);
    }

   private:
    const SceneBank& owner_;
    size_t pattern_;
  };

  SceneBank() { storage_.clear(); }

  PatternProxy operator[](size_t pattern) {
    return PatternProxy(*this, pattern);
  }

  PatternConstProxy operator[](size_t pattern) const {
    return PatternConstProxy(*this, pattern);
  }

  SceneState load(size_t pattern, size_t slot) const {
    SceneState state{};
    const size_t base = index(pattern, slot);
    state.mixVol = storage_.get(base);
    state.phonesVol = storage_.get(base + 1);
    for (size_t i = 0; i < SCENE_VOLUME_COUNT; ++i) {
      state.volumeFaders[i] = storage_.get(base + 2 + i);
    }
    for (size_t i = 0; i < SCENE_MULT_COUNT; ++i) {
      state.multiplierFaders[i] = storage_.get(base + 2 + SCENE_VOLUME_COUNT + i);
    }
    for (size_t i = 0; i < SCENE_DECAY_COUNT; ++i) {
      state.decayFaders[i] =
          storage_.get(base + 2 + SCENE_VOLUME_COUNT + SCENE_MULT_COUNT + i);
    }
    return state;
  }

  void store(size_t pattern, size_t slot, const SceneState& state) {
    const size_t base = index(pattern, slot);
    storage_.set(base, clampNibble(state.mixVol));
    storage_.set(base + 1, clampNibble(state.phonesVol));
    for (size_t i = 0; i < SCENE_VOLUME_COUNT; ++i) {
      storage_.set(base + 2 + i, clampNibble(state.volumeFaders[i]));
    }
    for (size_t i = 0; i < SCENE_MULT_COUNT; ++i) {
      storage_.set(base + 2 + SCENE_VOLUME_COUNT + i,
                   clampNibble(state.multiplierFaders[i]));
    }
    for (size_t i = 0; i < SCENE_DECAY_COUNT; ++i) {
      storage_.set(base + 2 + SCENE_VOLUME_COUNT + SCENE_MULT_COUNT + i,
                   clampNibble(state.decayFaders[i]));
    }
  }

  void clear() { storage_.clear(); }

 private:
    static uint8_t clampNibble(uint8_t value) {
      return static_cast<uint8_t>(value & 0x0Fu);
    }

    static constexpr size_t index(size_t pattern, size_t slot) {
      return (pattern * kSlotCount + slot) * kPackedPerScene;
    }

    PackedNibbleArray<kEntryCount * kPackedPerScene> storage_;
};
