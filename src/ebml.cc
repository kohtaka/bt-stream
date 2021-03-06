// Copyright (c) 2013 Kazumasa Kohtaka. All rights reserved.
// This file is available under the MIT license.

#include "./ebml.h"
#include <cstdio>
#include <cstdlib>

EBML::EBML(
    uint8_t *buffer,
    uint32_t offset,
    uint32_t length)
  : id_(0),
    size_(0),
    buffer_(buffer),
    length_(length),
    offset_(offset),
    data_offset_(0) {
#if DEBUG_EBML
  std::printf("=== EBML === ");
  for (uint32_t i = 0; i < 32 && i < length; ++i) {
    std::printf("%d ", *(buffer + offset + i));
  }
  std::printf("\n");

  std::printf("=== EBML === offset: %d, length: %d\n", offset, length);
#endif

  int64_t size_flag = 0x80;
  int64_t num = 0;
  while (
      ((num |= buffer[offset++] & 0xff) & size_flag) == 0 &&
      size_flag != 0) {
    num <<= 8;
    size_flag <<= 7;
  }
  id_ = num;

  size_flag = 0x80;
  num = 0;
  while (
      ((num |= buffer[offset++] & 0xff) & size_flag) == 0 &&
      size_flag != 0) {
    num <<= 8;
    size_flag <<= 7;
  }
  size_ = num ^ size_flag;

  data_offset_ = offset;

#if DEBUG_EBML
  std::printf("==> EBML::id: %ld, size %ld, data_offset: %d\n",
      id_, size_, data_offset_);
#endif
}

EBML::~EBML(void) {
#if DEBUG_EBML
  std::printf("EBML deleted.\n");
#endif
}

uint32_t EBML::load_unsigned(
    uint8_t *buffer,
    uint32_t offset,
    uint32_t length
) {
  int64_t num = 0;
  while (length > 0) {
    --length;
    num <<= 8;
    num |= buffer[offset++] & 0xff;
  }
  return num;
}

uint32_t EBML::load_EBML_unsigned(
    uint8_t *buffer,
    uint32_t offset,
    uint32_t length
) {
  int64_t size_flag = 0x80;
  int64_t num = 0;
  while (
      ((num |= buffer[offset++] & 0xff) & size_flag) == 0 &&
      size_flag != 0) {
    num <<= 8;
    size_flag <<= 7;
  }
  return num ^ size_flag;
}

int32_t EBML::load_EBML_signed(
    uint8_t *buffer,
    uint32_t offset,
    uint32_t length
) {
  int64_t size_flag = 0x80;
  int64_t num = 0;
  int64_t neg_bits = -1 << 7;
  while (
      ((num |= buffer[offset++] & 0xff) & size_flag) == 0 &&
      size_flag != 0) {
    num <<= 8;
    size_flag <<= 7;
    neg_bits <<= 7;
  }
  if ((num & size_flag >> 1) != 0) {
    num |= neg_bits;
  }
  return num;
}

int64_t EBML::id(void) {
  return id_;
}

int64_t EBML::data_size(void) {
  return size_;
}

int32_t EBML::element_size(void) {
  if (size_ == 0x1ffffffffffffffL) {
    return -1;
  }
  if (size_ >= 0x100000000L) {
    // [TODO] Handle the error.
  }
  return static_cast<int32_t>(data_offset_ - offset_ + size_);
}

uint8_t *EBML::buffer(void) {
  return buffer_;
}

uint32_t EBML::data_offset(void) {
  return data_offset_;
}

uint32_t EBML::element_offset(void) {
  return offset_;
}

int32_t EBML::end_offset(void) {
  if (size_ == 0x1ffffffffffffffL) {
    return -1;
  }
  if ((data_offset_ + size_) >= 0x100000000L) {
    // [TODO] Handle the error.
#if DEBUG_PUBLISHER
    std::printf("data_offset_: %d\n", data_offset_);
    std::printf("size_: %d\n", size_);
#endif
    ::exit(EXIT_FAILURE);
  }
  return data_offset_ + size_;
}

