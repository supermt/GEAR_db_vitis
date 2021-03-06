// Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved.
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#ifndef ROCKSDB_LITE

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "db/dbformat.h"
#include "file/random_access_file_reader.h"
#include "gear_table_file_reader.h"
#include "gear_table_index.h"
#include "memory/arena.h"
#include "rocksdb/env.h"
#include "rocksdb/iterator.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/table.h"
#include "rocksdb/table_properties.h"
#include "table/gear_block/gear_table_factory.h"
#include "table/table_reader.h"

namespace ROCKSDB_NAMESPACE {

class Block;
struct BlockContents;
class BlockHandle;
class Footer;
struct Options;
class RandomAccessFile;
struct ReadOptions;
class TableCache;
class TableReader;
class InternalKeyComparator;
class GearTableKeyDecoder;
class GetContext;
class GearTableFileReader;

extern const uint32_t kGearTableFixedKeyLength;
extern const uint32_t kGearTableFixedValueLength;

class GearTableReader : public TableReader {
 public:
  static Status Open(const ImmutableCFOptions& ioptions,
                     const EnvOptions& env_options,
                     const InternalKeyComparator& internal_comparator,
                     std::unique_ptr<RandomAccessFileReader>&& file,
                     uint64_t file_size,
                     std::unique_ptr<TableReader>* table_reader,
                     const bool immortal_table,
                     const SliceTransform* prefix_extractor);

  InternalIterator* NewIterator(const ReadOptions&,
                                const SliceTransform* prefix_extractor,
                                Arena* arena, bool skip_filters,
                                TableReaderCaller caller,
                                size_t compaction_readahead_size = 0,
                                bool allow_unprepared_value = false) override;

  void Prepare(const Slice& target) override;

  Status Get(const ReadOptions& readOptions, const Slice& key,
             GetContext* get_context, const SliceTransform* prefix_extractor,
             bool skip_filters = false) override;

  uint64_t ApproximateOffsetOf(const Slice& key,
                               TableReaderCaller caller) override;

  uint64_t ApproximateSize(const Slice& start, const Slice& end,
                           TableReaderCaller caller) override;

  uint32_t GetIndexSize() const { return index_.GetIndexSize(); }

  std::shared_ptr<const TableProperties> GetTableProperties() const override {
    return table_properties_;
  }

  virtual size_t ApproximateMemoryUsage() const override {
    return arena_.MemoryAllocatedBytes();
  }

  GearTableReader(const ImmutableCFOptions& ioptions,
                  std::unique_ptr<RandomAccessFileReader>&& file,
                  const EnvOptions& env_options,
                  const InternalKeyComparator& internal_comparator,
                  EncodingType encoding_type, uint64_t file_size,
                  const TableProperties* table_properties,
                  const SliceTransform* prefix_extractor);
  virtual ~GearTableReader();
  void SetupForCompaction() override{};
  void SetupForCompaction(std::string* all_data_blocks) override;
  std::string SetupForCompactionHW() override;
  std::string full_data_blocks;

 protected:
  Status MmapDataIfNeeded();

 private:
  const InternalKeyComparator internal_comparator_;
  EncodingType encoding_type_;
  // represents plain table's current status.
  Status status_;

  GearTableIndexReader index_;
  GearTableFileReader* file_reader_;
  //  bool full_scan_mode_;

  // data_start_offset_ and data_end_offset_ defines the range of the
  // sst file that stores data.
  const uint32_t user_key_len_;
  const SliceTransform* prefix_extractor_;

  static const size_t kNumInternalBytes = 8;
  uint64_t entry_counts_in_file_;

  Arena arena_;

  const ImmutableCFOptions& ioptions_;
  std::unique_ptr<Cleanable> dummy_cleanable_;
  uint64_t file_size_;
  uint64_t attached_index_file_size_;

 protected:  // for testing
  std::shared_ptr<const TableProperties> table_properties_;

 private:
  bool IsFixedLength() const {
    return true;  // Gear Table is always with the fixed length key/value
  }

  size_t GetFixedInternalKeyLength() const {
    return user_key_len_ + kNumInternalBytes;
  }

  Slice GetPrefix(const Slice& target) const {
    assert(target.size() >= 8);  // target is internal key
    return GetPrefixFromUserKey(GetUserKey(target));
  }

  Slice GetPrefix(const ParsedInternalKey& target) const {
    return GetPrefixFromUserKey(target.user_key);
  }

  Slice GetUserKey(const Slice& key) const {
    return Slice(key.data(), key.size() - kNumInternalBytes);
  }

  Slice GetPrefixFromUserKey(const Slice& user_key) const {
    if (!IsTotalOrderMode()) {
      return prefix_extractor_->Transform(user_key);
    } else {
      // Use empty slice as prefix if prefix_extractor is not set.
      // In that case,
      // it falls back to pure binary search and
      // total iterator seek is supported.
      return Slice();
    }
  }

  friend class TableCache;
  friend class GearTableIterator;

  Status GetOffset(const Slice& target, uint32_t* offset) const;

  bool IsTotalOrderMode() const { return (prefix_extractor_ == nullptr); }

  // No copying allowed
  explicit GearTableReader(const TableReader&) = delete;
  void operator=(const TableReader&) = delete;
  static Status ReadProperties(RandomAccessFileReader* file, uint64_t file_size,
                               TableProperties** properties);
  static const int table_prop_size = 13;
};
}  // namespace ROCKSDB_NAMESPACE
#endif  // ROCKSDB_LITE
