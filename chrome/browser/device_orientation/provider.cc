// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/device_orientation/provider.h"

#include "base/logging.h"
#include "chrome/browser/chrome_thread.h"
#include "chrome/browser/device_orientation/data_fetcher.h"
#include "chrome/browser/device_orientation/provider_impl.h"

namespace device_orientation {

Provider* Provider::GetInstance() {
  if (!instance_) {
    DCHECK(ChromeThread::CurrentlyOn(ChromeThread::IO));
    const ProviderImpl::DataFetcherFactory default_factories[] = { NULL };

    instance_ = new ProviderImpl(MessageLoop::current(), default_factories);
  }
  return instance_;
}

void Provider::SetInstanceForTests(Provider* provider) {
  DCHECK(!instance_);
  instance_ = provider;
}

Provider* Provider::instance_ = NULL;

} //  namespace device_orientation
