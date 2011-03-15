// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_VALUE_CONVERSIONS_H_
#define BASE_VALUE_CONVERSIONS_H_
#pragma once

// This file contains methods to convert a |FilePath| to a |Value| and back.

class FilePath;
class StringValue;
class Value;

namespace base {

// The caller takes ownership of the returned value.
StringValue* CreateFilePathValue(const FilePath& in_value);
bool GetValueAsFilePath(const Value& value, FilePath* file_path);

}  // namespace

#endif  // BASE_VALUE_CONVERSIONS_H_
