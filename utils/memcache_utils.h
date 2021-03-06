/**
 *
 *  Copyright 2021 Netflix, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 *
 */

#pragma once

#include "utils/slice.h"
#include "utils/status.h"

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Default number of items to bulk get at a time
#define DEFAULT_BULK_GET_THRESHOLD 30

// Ignore a key if we tried to get it these many times unsuccessfully.
#define MAX_GET_ATTEMPTS 3

// Forward declaration.
class KeyFilter;

namespace memcachedumper {

class McData {
 public:
  McData(char *key, size_t keylen, int32_t expiry);
  McData(std::string& key, int32_t expiry);
  void setValue(const char* data_ptr, size_t size);
  void setValueLength(size_t value_len) { value_len_ = value_len; }
  void setFlags(uint16_t flags) { flags_ = flags; }

  void printValue();

  std::string& key() { return key_; }
  int32_t expiry() { return expiry_; }
  uint16_t flags() { return flags_; }

  char* Value() { return data_->mutable_data(); }
  size_t ValueLength() { return value_len_; }

  void MarkComplete() { complete_ = true; }

  void set_get_complete(bool get_complete) {
    ++get_attempts_;
    get_complete_ = get_complete;
  }
  inline bool get_complete() { return get_complete_; }
  // Returns 'false' if this McData is marked as incomplete, i.e. one or
  // more required fields are not present/completely entered.
  bool Complete() { return complete_; }

  // If we tried to get a key for MAX_GET_ATTEMPTS unsuccussfully, we
  // consider the key evicted or expired.
  bool PossiblyEvicted() { return get_attempts_ >= MAX_GET_ATTEMPTS; }

 private:
  std::string key_;
  int32_t expiry_;
  uint16_t flags_;
  size_t value_len_;
  std::unique_ptr<Slice> data_;

  bool get_complete_;
  bool complete_;
  int get_attempts_;
};


typedef std::unordered_map<std::string, std::unique_ptr<McData>> McDataMap;

class MemcachedUtils {
 public:
  static void SetReqId(std::string req_id);
  static void SetOutputDirPath(std::string output_dir_path);
  static void SetBulkGetThreshold(uint32_t bulk_get_threshold);
  static void SetMaxDataFileSize(uint64_t max_data_file_size);
  static void SetOnlyExpireAfter(int only_expire_after);
  static void SetDestIps(const std::vector<std::string>& dest_ips);
  static void SetAllIps(const std::vector<std::string>& all_ips);

  static std::string GetReqId() { return MemcachedUtils::req_id_; }
  static std::string OutputDirPath() { return MemcachedUtils::output_dir_path_; }
  static uint32_t BulkGetThreshold() { return MemcachedUtils::bulk_get_threshold_; }
  static uint64_t MaxDataFileSize() { return MemcachedUtils::max_data_file_size_; }
  static uint64_t OnlyExpireAfter() { return MemcachedUtils::only_expire_after_; }
  static std::string GetKeyFilePath();
  static std::string GetDataStagingPath();
  static std::string GetDataFinalPath();
  static std::vector<std::string>* GetDestIps();

  static std::string KeyFilePrefix();
  static std::string DataFilePrefix();

  // Initialize key filtering for use by individual tasks.
  // Must call SetDestIps() and SetAllIps() before using.
  // TODO: Consider avoiding this global state.
  static Status InitKeyFilter(uint32_t ketama_bucket_size);
  // Returns 'true' if key needs to be filtered out. 'false' otherwise.
  // Always returns 'false' if InitKeyFilter() isn't called before this.
  static bool FilterKey(const std::string& key);

  // Craft a bulk get command with the first 'BulkGetThreshold()' keys in
  // 'pending_keys' to send memcached.
  static std::string CraftBulkGetCommand(McDataMap* pending_keys);

  // Returns a string of the following format for 'key':
  // <keylen (2-bytes)> <key> <expiry (4-bytes)> <flag (4-bytes)> <datalen (4-bytes)>
  static std::string CraftMetadataString(McData* key) {
    std::string final_str;
    final_str.append(MemcachedUtils::ConvertIntToBytes(key->key().length(), 2));
    final_str.append(key->key().c_str());
    final_str.append(MemcachedUtils::ConvertIntToBytes(key->expiry(), 4));
    final_str.append(MemcachedUtils::ConvertUInt16ToBytes(key->flags(), 4));
    final_str.append(MemcachedUtils::ConvertIntToBytes(key->ValueLength(), 4));

    return final_str;
  }

  // Reads 'filename' and extracts IP:Port pairs from the file.
  // Assumes that the contents of the file are of the format of one IP:Port per line.
  // Eg:
  // <IP1>:<port1>
  // <IP2>:<port2>
  // ...
  static Status ExtractIPsFromFile(std::string filename,
      std::vector<std::string>& out_ips) {
    std::string ip;

    std::ifstream fhandle(filename);
    while(std::getline(fhandle, ip)) {
      out_ips.push_back(ip);
    }
    fhandle.close();
    return Status::OK();
  }

  static bool KeyExpiresSoon(time_t now, uint32_t key_expiry) {
    // TODO: Is this portable?
    return (key_expiry <= now + OnlyExpireAfter());
  }

  // Converts 'int_param' to its byte representation in a string, and returns
  // a string with 'out_bytes' number of bytes.
  static std::string ConvertIntToBytes(int int_param, int out_bytes) {
    std::vector<unsigned char> byte_array(out_bytes);
    for (int i = 0; i < out_bytes; i++) {
      byte_array[out_bytes - i - 1] = (int_param >> (i * 8));
    }
    std::string s(byte_array.begin(), byte_array.end());
    return s;
  }

  // Converts 'uint16_param' to its byte representation in a string, and returns
  // a string with 'out_bytes' number of bytes.
  static std::string ConvertUInt16ToBytes(uint16_t uint16_param, int out_bytes) {
    std::vector<unsigned char> byte_array(out_bytes);
    for (int i = 0; i < out_bytes; i++) {
      byte_array[out_bytes - i - 1] = (uint16_param >> (i * 8));
    }
    std::string s(byte_array.begin(), byte_array.end());
    return s;
  }

 private:
  static std::string req_id_;
  static std::string output_dir_path_;
  static uint32_t bulk_get_threshold_;
  static uint64_t max_data_file_size_;
  static int only_expire_after_;

  static std::vector<std::string> dest_ips_;
  static std::vector<std::string> all_ips_;

  static KeyFilter* kf_;
};


class MetaBufferSlice : public Slice {
 public:
  MetaBufferSlice(const char* d, size_t n)
    : Slice(d, n),
      pending_data_(d),
      start_copy_pos_(0),
      slice_end_(d + n) {

  }

  char* buf_begin_fill() {
    return const_cast<char*>(data() + start_copy_pos_);
  }

  size_t free_bytes() {
    return size() - start_copy_pos_;
  }

  const char* next_key_pos() {
    const char* pos = strstr(pending_data_, "key=");
    if (pos && pos < slice_end_) {
      MarkProcessedUntil(pos + 4); // Skip 'key='
      return pos;
    }
    return nullptr;
  }
  const char* next_exp_pos() {
    const char* pos = strstr(pending_data_, "exp=");
    if (pos && pos < slice_end_) {
      MarkProcessedUntil(pos + 4); // Skip 'exp='
      return pos;
    }
    return nullptr;
  }
  const char* next_la_pos() {
    const char* pos = strstr(pending_data_, "la=");
    if (pos && pos < slice_end_) {
      MarkProcessedUntil(pos + 3); // Skip 'la='
      return pos;
    }
    return nullptr;
  }
  const char* next_newline() {
    const char* pos = strstr(pending_data_, "\n");
    if (pos && pos < slice_end_) {
      MarkProcessedUntil(pos + 1); // Skip '\n'
      return pos;
    }
    return nullptr;
  }

  void CopyRemainingToStart(const char* copy_from) {
    //ASSERT(copy_from > data() && copy_from < data() + size());

    size_t num_bytes_to_copy = data() + size() - copy_from;
    //ASSERT(num_bytes_to_copy < (copy_from - data());

    memcpy(const_cast<char*>(data()), copy_from, num_bytes_to_copy);
    start_copy_pos_ = num_bytes_to_copy;

    // Zero out the remaining bytes so that we don't accidentally parse them again.
    bzero(const_cast<char*>(data() + start_copy_pos_), free_bytes());
  }

  const char* pending_data() { return pending_data_; }

 private:
  inline void MarkProcessedUntil(const char *buf_ptr) {
    pending_data_ = buf_ptr;
  }
 
  const char* pending_data_;
  uint32_t start_copy_pos_;

  // Points to the end of the slice.
  const char* const slice_end_;
};

class DataBufferSlice : public Slice {
 public:
  DataBufferSlice(const char* d, size_t n)
    : Slice(d, n),
      parse_state_(ResponseFormatState::VALUE_DELIM),
      pending_data_(d),
      start_copy_pos_(0) {

  }

  enum class ResponseFormatState {
    VALUE_DELIM = 0,
    KEY_NAME = 1,
    FLAGS = 2,
    NUM_DATA_BYTES = 3,
    DATA = 4,
    TOTAL_NUM_STATES = 5
  };

  char* buf_begin_fill() {
    return const_cast<char*>(data() + start_copy_pos_);
  }

  size_t free_bytes() {
    return size() - start_copy_pos_;
  }

  const char* next_value_delim() {
    //const char* pos = strstr(pending_data_, "VALUE ");
    int32_t n_pending = bytes_pending();
    const char* pos = static_cast<const char*>(memmem(pending_data_, n_pending, "VALUE ", 6));
    if (pos) MarkProcessedUntil(pos + 6); // Skip 'VALUE '
    return pos;
  }
  const char* next_whitespace() {
    const char* pos = strstr(pending_data_, " ");
    if (pos) MarkProcessedUntil(pos + 1); // Skip ' '
    return pos;
  }
  const char* next_crlf() {
    const char* pos = strstr(pending_data_, "\r\n");
    if (pos) MarkProcessedUntil(pos + 2); // Skip '\r\n'
    return pos;
  }
  const char* process_value(size_t value_size) {
    if (pending_data_ + value_size > data() + size()) {
      return nullptr;
    }
    MarkProcessedUntil(pending_data_ + value_size + 2);
    return pending_data_ + value_size;
  }

  const char* pending_data() { return pending_data_; }

  int32_t bytes_pending() { return &data()[size()] - pending_data_; }

  bool reached_end() {
    return !strncmp(&data()[size() - 5], "END\r\n", 5);
  }
  bool reached_error() {
    return !strncmp(&data()[size() - 7], "ERROR\r\n", 7);
  }

  ResponseFormatState parse_state() { return parse_state_; }

 private:

  inline void StepStateMachine() {
    int16_t cur_state = static_cast<int16_t>(parse_state_);
    int16_t total_states = static_cast<int16_t>(ResponseFormatState::TOTAL_NUM_STATES);

    parse_state_ = static_cast<ResponseFormatState>((cur_state + 1) % total_states);
  }

  inline void MarkProcessedUntil(const char *buf_ptr) {
    pending_data_ = buf_ptr;
    StepStateMachine();
  }
 
  ResponseFormatState parse_state_;
  const char* pending_data_;
  uint32_t start_copy_pos_;
};

} // namespace memcachedumper
