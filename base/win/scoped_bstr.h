// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WIN_SCOPED_BSTR_H_
#define BASE_WIN_SCOPED_BSTR_H_
#pragma once

#include <windows.h>
#include <oleauto.h>

#include "base/logging.h"
#include "base/string16.h"

namespace base {
namespace win {

// Manages a BSTR string pointer.
// The class interface is based on scoped_ptr.
class ScopedBstr {
 public:
  ScopedBstr() : bstr_(NULL) {
  }

  // Constructor to create a new BSTR.
  //
  // NOTE: Do not pass a BSTR to this constructor expecting ownership to
  // be transferred - even though it compiles! ;-)
  explicit ScopedBstr(const char16* non_bstr);
  ~ScopedBstr();

  // Give ScopedBstr ownership over an already allocated BSTR or NULL.
  // If you need to allocate a new BSTR instance, use |allocate| instead.
  void Reset(BSTR bstr = NULL);

  // Releases ownership of the BSTR to the caller.
  BSTR Release();

  // Creates a new BSTR from a 16-bit C-style string.
  //
  // If you already have a BSTR and want to transfer ownership to the
  // ScopedBstr instance, call |reset| instead.
  //
  // Returns a pointer to the new BSTR, or NULL if allocation failed.
  BSTR Allocate(const char16* str);

  // Allocates a new BSTR with the specified number of bytes.
  // Returns a pointer to the new BSTR, or NULL if allocation failed.
  BSTR AllocateBytes(size_t bytes);

  // Sets the allocated length field of the already-allocated BSTR to be
  // |bytes|.  This is useful when the BSTR was preallocated with e.g.
  // SysAllocStringLen or SysAllocStringByteLen (call |AllocateBytes|) and then
  // not all the bytes are being used.
  //
  // Note that if you want to set the length to a specific number of
  // characters, you need to multiply by sizeof(wchar_t).  Oddly, there's no
  // public API to set the length, so we do this ourselves by hand.
  //
  // NOTE: The actual allocated size of the BSTR MUST be >= bytes.  That
  // responsibility is with the caller.
  void SetByteLen(size_t bytes);

  // Swap values of two ScopedBstr's.
  void Swap(ScopedBstr& bstr2);

  // Retrieves the pointer address.
  // Used to receive BSTRs as out arguments (and take ownership).
  // The function DCHECKs on the current value being NULL.
  // Usage: GetBstr(bstr.Receive());
  BSTR* Receive();

  // Returns number of chars in the BSTR.
  size_t Length() const;

  // Returns the number of bytes allocated for the BSTR.
  size_t ByteLength() const;

  operator BSTR() const {
    return bstr_;
  }

 protected:
  BSTR bstr_;

 private:
  // Forbid comparison of ScopedBstr types.  You should never have the same
  // BSTR owned by two different scoped_ptrs.
  bool operator==(const ScopedBstr& bstr2) const;
  bool operator!=(const ScopedBstr& bstr2) const;
  DISALLOW_COPY_AND_ASSIGN(ScopedBstr);
};

}  // namespace win
}  // namespace base

#endif  // BASE_SCOPED_BSTR_H_
