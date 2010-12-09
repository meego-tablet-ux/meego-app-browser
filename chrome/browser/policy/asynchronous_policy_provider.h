// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_ASYNCHRONOUS_POLICY_PROVIDER_H_
#define CHROME_BROWSER_POLICY_ASYNCHRONOUS_POLICY_PROVIDER_H_
#pragma once

#include "base/non_thread_safe.h"
#include "base/ref_counted.h"
#include "base/weak_ptr.h"
#include "chrome/browser/policy/configuration_policy_provider.h"

namespace policy {

class AsynchronousPolicyLoader;

// Policy provider that loads policy asynchronously. Providers should subclass
// from this class if loading the policy requires disk access or must for some
// other reason be performed on the file thread. The actual logic for loading
// policy is handled by a delegate passed at construction time.
class AsynchronousPolicyProvider
    : public ConfigurationPolicyProvider,
      public base::SupportsWeakPtr<AsynchronousPolicyProvider>,
      public NonThreadSafe {
 public:
  // Must be implemented by subclasses of the asynchronous policy provider to
  // provide the implementation details of how policy is loaded.
  class Delegate {
   public:
    virtual ~Delegate() {}

    virtual DictionaryValue* Load() = 0;
  };

  // Assumes ownership of |loader|.
  AsynchronousPolicyProvider(
      const PolicyDefinitionList* policy_list,
      scoped_refptr<AsynchronousPolicyLoader> loader);
  virtual ~AsynchronousPolicyProvider();

  // ConfigurationPolicyProvider implementation.
  virtual bool Provide(ConfigurationPolicyStoreInterface* store);

  // For tests to trigger reloads.
  scoped_refptr<AsynchronousPolicyLoader> loader();

 protected:
  // The loader object used internally.
  scoped_refptr<AsynchronousPolicyLoader> loader_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AsynchronousPolicyProvider);
};

}  // namespace policy

#endif  // CHROME_BROWSER_POLICY_ASYNCHRONOUS_POLICY_PROVIDER_H_
