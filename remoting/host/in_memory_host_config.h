// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_IN_MEMORY_HOST_CONFIG_H_
#define REMOTING_HOST_IN_MEMORY_HOST_CONFIG_H_

#include <string>

#include "base/lock.h"
#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "remoting/host/host_config.h"

class DictionaryValue;
class Task;

namespace remoting {

// In-memory host config. Used by unittests.
class InMemoryHostConfig : public MutableHostConfig {
 public:
  InMemoryHostConfig();
  virtual ~InMemoryHostConfig();

  // MutableHostConfig interface.
  virtual bool GetString(const std::string& path, std::string* out_value);

  virtual void Update(Task* task);

  virtual void SetString(const std::string& path, const std::string& in_value);

 private:
  Lock lock_;
  scoped_ptr<DictionaryValue> values_;

  DISALLOW_COPY_AND_ASSIGN(InMemoryHostConfig);
};

}  // namespace remoting

#endif  // REMOTING_HOST_IN_MEMORY_HOST_CONFIG_H_
