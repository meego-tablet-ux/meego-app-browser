// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_SCOPED_VARIANT_WIN_H_
#define BASE_SCOPED_VARIANT_WIN_H_

#include <windows.h>
#include <oleauto.h>

#include "base/basictypes.h"  // needed to pick up OS_WIN
#include "base/logging.h"

// Scoped VARIANT class for automatically freeing a COM VARIANT at the
// end of a scope.  Additionally provides a few functions to make the
// encapsulated VARIANT easier to use.
// Instead of inheriting from VARIANT, we take the containment approach
// in order to have more control over the usage of the variant and guard
// against memory leaks.
class ScopedVariant {
 public:
  // Declaration of a global variant variable that's always VT_EMPTY
  static const VARIANT kEmptyVariant;

  // Default constructor.
  ScopedVariant() {
    // This is equivalent to what VariantInit does, but less code.
    var_.vt = VT_EMPTY;
  }

  // Constructor to create a new VT_BSTR VARIANT.
  // NOTE: Do not pass a BSTR to this constructor expecting ownership to
  // be transferred
  explicit ScopedVariant(const wchar_t* str);

  // Creates a new VT_BSTR variant of a specified length.
  explicit ScopedVariant(const wchar_t* str, UINT length);

  // Creates a new integral type variant and assigns the value to
  // VARIANT.lVal (32 bit sized field).
  explicit ScopedVariant(int value, VARTYPE vt = VT_I4);

  // VT_DISPATCH
  explicit ScopedVariant(IDispatch* dispatch);

  // VT_UNKNOWN
  explicit ScopedVariant(IUnknown* unknown);

  // Copies the variant.
  explicit ScopedVariant(const VARIANT& var);

  ~ScopedVariant();

  inline VARTYPE type() const {
    return var_.vt;
  }

  // Give ScopedVariant ownership over an already allocated VARIANT.
  void Reset(const VARIANT& var = kEmptyVariant);

  // Releases ownership of the VARIANT to the caller.
  VARIANT Release();

  // Swap two ScopedVariant's.
  void Swap(ScopedVariant& var);

  // Returns a copy of the variant.
  VARIANT Copy() const;

  // The return value is 0 if the variants are equal, 1 if this object is
  // greater than |var|, -1 if it is smaller.
  int Compare(const VARIANT& var, bool ignore_case = false) const;

  // Retrieves the pointer address.
  // Used to receive a VARIANT as an out argument (and take ownership).
  // The function DCHECKs on the current value being empty/null.
  // Usage: GetVariant(var.receive());
  VARIANT* Receive();

  void Set(const wchar_t* str);

  // Setters for simple types.
  void Set(int8 i8);
  void Set(uint8 ui8);
  void Set(int16 i16);
  void Set(uint16 ui16);
  void Set(int32 i32);
  void Set(uint32 ui32);
  void Set(int64 i64);
  void Set(uint64 ui64);
  void Set(float r32);
  void Set(double r64);
  void Set(bool b);

  // Creates a copy of |var| and assigns as this instance's value.
  // Note that this is different from the Reset() method that's used to
  // free the current value and assume ownership.
  void Set(const VARIANT& var);

  // COM object setters
  void Set(IDispatch* disp);
  void Set(IUnknown* unk);

  // SAFEARRAY support
  void Set(SAFEARRAY* array);

  // Special setter for DATE since DATE is a double and we already have
  // a setter for double.
  void SetDate(DATE date);

  // Allows const access to the contained variant without DCHECKs etc.
  // This support is necessary for the V_XYZ (e.g. V_BSTR) set of macros to
  // work properly but still doesn't allow modifications since we want control
  // over that.
  const VARIANT* operator&() const {
    return &var_;
  }

  // Like other scoped classes (e.g scoped_refptr, ScopedComPtr, ScopedBstr)
  // we support the assignment operator for the type we wrap.
  ScopedVariant& operator=(const VARIANT& var);

  // A hack to pass a pointer to the variant where the accepting
  // function treats the variant as an input-only, read-only value
  // but the function prototype requires a non const variant pointer.
  // There's no DCHECK or anything here.  Callers must know what they're doing.
  VARIANT* AsInput() const {
    // The nature of this function is const, so we declare
    // it as such and cast away the constness here.
    return const_cast<VARIANT*>(&var_);
  }

  // Allows the ScopedVariant instance to be passed to functions either by value
  // or by const reference.
  operator const VARIANT&() const {
    return var_;
  }

  // Used as a debug check to see if we're leaking anything.
  static bool IsLeakableVarType(VARTYPE vt);

 protected:
  VARIANT var_;

 private:
  // Comparison operators for ScopedVariant are not supported at this point.
  // Use the Compare method instead.
  bool operator==(const ScopedVariant& var) const;
  bool operator!=(const ScopedVariant& var) const;
  DISALLOW_COPY_AND_ASSIGN(ScopedVariant);
};

#endif  // BASE_SCOPED_VARIANT_WIN_H_
