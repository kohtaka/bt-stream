// Copyright (c) 2013 Kazumasa Kohtaka. All rights reserved.
// This file is available under the MIT license.

#include "./stream_input.h"
#include <sstream>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include "./header_detection_state.h"

StreamInput::StreamInput(
    std::shared_ptr<Stream> stream,
    std::shared_ptr<Client> client)
  : stream_(stream),
    client_(client),
    running_(false),
    header_(),
    header_length_(0),
    chunk_size_(0),
    state_(nullptr),
    buffer_() {
  client_.get()->set_data(this);
}

StreamInput::~StreamInput(void) {
  std::printf("StreamInput deleted. %p\n", static_cast<void *>(this));
}

bool StreamInput::is_running(void) {
  return running_;
}

uint8_t *StreamInput::header(void) {
  return header_;
}

uint32_t StreamInput::header_length(void) {
  return header_length_;
}

void StreamInput::set_header(
    uint8_t *header,
    uint32_t header_length) {
  header_ = header;
  header_length_ = header_length;
}

void StreamInput::run(void) {
  running_ = true;

  // [TODO] postEvent stream_.publish(Stream::INPUT_START);

  change_state(std::shared_ptr<StreamInputState>(
        new HeaderDetectionState(this, stream_)));

  if (running_ && stream_.get()->is_running()) {
    client_.get()->start_reading(
        [] (uv_handle_s *handle, size_t suggested_size) -> uv_buf_t {
          Client *client = reinterpret_cast<Client *>(handle->data);
          StreamInput *self = reinterpret_cast<StreamInput *>(client->data());

          return self->on_alloc(suggested_size);
        },
        [] (uv_stream_t *stream, ssize_t nread, uv_buf_t buf) {
          Client *client = reinterpret_cast<Client *>(stream->data);
          StreamInput *self = reinterpret_cast<StreamInput *>(client->data());

          self->on_read(nread, buf);
        });
  }
}

void StreamInput::change_state(std::shared_ptr<StreamInputState> state) {
  state_.swap(state);
}

uv_buf_t StreamInput::on_alloc(size_t suggested_size) {
  return ::uv_buf_init(
      reinterpret_cast<char *>(buffer_.buffer.get() + buffer_.offset),
      buffer_.buffer_length - buffer_.offset);
}

void StreamInput::on_read(ssize_t nread, uv_buf_t buf) {
  // [TODO] stream.postEvent(TransferEvent.STREAM_INPUT);

  if (nread < 0) {
    running_ = false;

    client_.get()->close(
        [] (uv_handle_t *handle) {
          Client *client = reinterpret_cast<Client *>(handle->data);
          StreamInput *self = reinterpret_cast<StreamInput *>(client->data());

          self->on_close();
        });

  } else {
    client_.get()->parse_request(
        nread,
        buf,
        [] (http_parser *parser, const char *buf, size_t len) {
          Client *client = reinterpret_cast<Client *>(parser->data);
          StreamInput *self = reinterpret_cast<StreamInput *>(client->data());

          return self->on_body(reinterpret_cast<const uint8_t *>(buf), len);
        });
  }
}

int StreamInput::on_body(const uint8_t *buf, size_t len) {
  ::memmove(
      buffer_.buffer.get() + buffer_.offset,
      buf,
      len);

  buffer_.length += len;

  while (buffer_.length != 0) {
    uint32_t new_offset = state_.get()->process_data(
        buffer_.buffer.get(),
        0,
        buffer_.length);

    if (new_offset == 0) {
#if DEBUG_PUBLISHER
      std::cout << "=== DATA WAS PROCESSED. ===" << std::endl;
#endif
      break;
    }

    if (new_offset > buffer_.offset + buffer_.length) {
      std::printf("new_offset: %d, buffer_.offset: %d, buffer_.length: %d\n", new_offset, buffer_.offset, buffer_.length);
      throw std::exception();
    }

    buffer_.length -= new_offset;

    ::memmove(
        buffer_.buffer.get(),
        buffer_.buffer.get() + new_offset,
        buffer_.length);

    buffer_.offset = buffer_.length;
  }

  buffer_.offset = buffer_.length;

  return 0;
}

void StreamInput::on_close(void) {
  // [TODO] stream.postEvent(ServerEvent.INPUT_STOP);

  std::printf("===== CONNECTION WAS CLOSED. =====\n");
}

