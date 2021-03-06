// Copyright (c) 2013 Kazumasa Kohtaka. All rights reserved.
// This file is available under the MIT license.

#ifndef SRC_STREAM_H_
#define SRC_STREAM_H_

#include <inttypes.h>
#include <string>
#include <memory>
#include "./movie_fragment.h"

class Stream {
 private:
  bool running_;
  std::shared_ptr<uint8_t> header_;
  uint32_t header_length_;
  uint32_t fragment_age_;
  std::shared_ptr<MovieFragment> fragment_;
 public:
  Stream(void);
  virtual ~Stream(void);
  bool is_running(void);
  uint32_t fragment_age(void);
  std::shared_ptr<MovieFragment> fragment(void);
  std::shared_ptr<uint8_t> header(void);
  uint32_t header_length(void);
  void set_header(std::shared_ptr<uint8_t> header, uint32_t header_length);
  void push_fragment(std::shared_ptr<MovieFragment> fragment);
};

#endif  // SRC_STREAM_H_

