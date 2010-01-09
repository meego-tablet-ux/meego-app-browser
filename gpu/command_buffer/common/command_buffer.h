// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_COMMON_COMMAND_BUFFER_H_
#define GPU_COMMAND_BUFFER_COMMON_COMMAND_BUFFER_H_

#include "base/basictypes.h"
#include "gpu/command_buffer/common/buffer.h"

namespace gpu {

// Common interface for CommandBuffer implementations.
class CommandBuffer {
 public:
  CommandBuffer() {
  }

  virtual ~CommandBuffer() {
  }

  // Initialize the command buffer with the given size (number of command
  // entries).
  virtual bool Initialize(int32 size) = 0;

  // Gets the ring buffer for the command buffer.
  virtual Buffer GetRingBuffer() = 0;

  virtual int32 GetSize() = 0;

  // The writer calls this to update its put offset. This function returns the
  // reader's most recent get offset. Does not return until after the put offset
  // change callback has been invoked. Returns -1 if the put offset is invalid.
  virtual int32 SyncOffsets(int32 put_offset) = 0;

  // Returns the current get offset. This can be called from any thread.
  virtual int32 GetGetOffset() = 0;

  // Sets the current get offset. This can be called from any thread.
  virtual void SetGetOffset(int32 get_offset) = 0;

  // Returns the current put offset. This can be called from any thread.
  virtual int32 GetPutOffset() = 0;

  // Create a transfer buffer and return a handle that uniquely
  // identifies it or -1 on error.
  virtual int32 CreateTransferBuffer(size_t size) = 0;

  // Destroy a transfer buffer and recycle the handle.
  virtual void DestroyTransferBuffer(int32 id) = 0;

  // Get the transfer buffer associated with a handle.
  virtual Buffer GetTransferBuffer(int32 handle) = 0;

  // Get the current token value. This is used for by the writer to defer
  // changes to shared memory objects until the reader has reached a certain
  // point in the command buffer. The reader is responsible for updating the
  // token value, for example in response to an asynchronous set token command
  // embedded in the command buffer. The default token value is zero.
  virtual int32 GetToken() = 0;

  // Allows the reader to update the current token value.
  virtual void SetToken(int32 token) = 0;

  // Get the current parse error and reset it to zero. Zero means no error. Non-
  // zero means error. The default error status is zero.
  virtual int32 ResetParseError() = 0;

  // Allows the reader to set the current parse error.
  virtual void SetParseError(int32 parse_error) = 0;

  // Returns whether the command buffer is in the error state.
  virtual bool GetErrorStatus() = 0;

  // Allows the reader to set the error status. Once in an error state, the
  // command buffer cannot recover and ceases to process commands.
  virtual void RaiseErrorStatus() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CommandBuffer);
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_COMMON_COMMAND_BUFFER_H_
