// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains the implementation of the command parser.

#include "gpu/command_buffer/service/precompile.h"
#include "gpu/command_buffer/service/cmd_parser.h"

namespace gpu {

CommandParser::CommandParser(void *shm_address,
                             size_t shm_size,
                             ptrdiff_t offset,
                             size_t size,
                             CommandBufferOffset start_get,
                             AsyncAPIInterface *handler)
    : get_(start_get),
      put_(start_get),
      handler_(handler) {
  // check proper alignments.
  DCHECK_EQ(0, (reinterpret_cast<intptr_t>(shm_address)) % 4);
  DCHECK_EQ(0, offset % 4);
  DCHECK_EQ(0u, size % 4);
  // check that the command buffer fits into the memory buffer.
  DCHECK_GE(shm_size, offset + size);
  char * buffer_begin = static_cast<char *>(shm_address) + offset;
  buffer_ = reinterpret_cast<CommandBufferEntry *>(buffer_begin);
  entry_count_ = size / 4;
}

// Process one command, reading the header from the command buffer, and
// forwarding the command index and the arguments to the handler.
// Note that:
// - validation needs to happen on a copy of the data (to avoid race
// conditions). This function only validates the header, leaving the arguments
// validation to the handler, so it can pass a reference to them.
// - get_ is modified *after* the command has been executed.
parse_error::ParseError CommandParser::ProcessCommand() {
  CommandBufferOffset get = get_;
  if (get == put_) return parse_error::kParseNoError;

  CommandHeader header = buffer_[get].value_header;
  if (header.size == 0) {
    DLOG(INFO) << "Error: zero sized command in command buffer";
    return parse_error::kParseInvalidSize;
  }

  if (static_cast<int>(header.size) + get > entry_count_) {
    DLOG(INFO) << "Error: get offset out of bounds";
    return parse_error::kParseOutOfBounds;
  }

  parse_error::ParseError result = handler_->DoCommand(
      header.command, header.size - 1, buffer_ + get);
  // TODO(gman): If you want to log errors this is the best place to catch them.
  //     It seems like we need an official way to turn on a debug mode and
  //     get these errors.
  if (result != parse_error::kParseNoError) {
    ReportError(header.command, result);
  }
  get_ = (get + header.size) % entry_count_;
  return result;
}

void CommandParser::ReportError(unsigned int command_id,
                                parse_error::ParseError result) {
  DLOG(INFO) << "Error: " << result << " for Command "
             << handler_->GetCommandName(command_id);
}

// Processes all the commands, while the buffer is not empty. Stop if an error
// is encountered.
parse_error::ParseError CommandParser::ProcessAllCommands() {
  while (!IsEmpty()) {
    parse_error::ParseError error = ProcessCommand();
    if (error) return error;
  }
  return parse_error::kParseNoError;
}

}  // namespace gpu
