// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains the autocomplete provider for built-in URLs,
// such as about:settings.
//
// For more information on the autocomplete system in general, including how
// the autocomplete controller and autocomplete providers work, see
// chrome/browser/autocomplete.h.

#ifndef CHROME_BROWSER_AUTOCOMPLETE_BUILTIN_PROVIDER_H_
#define CHROME_BROWSER_AUTOCOMPLETE_BUILTIN_PROVIDER_H_
#pragma once

#include <string>
#include <vector>

#include "chrome/browser/autocomplete/autocomplete.h"

class BuiltinProvider : public AutocompleteProvider {
 public:
  BuiltinProvider(ACProviderListener* listener, Profile* profile);

  // AutocompleteProvider
  virtual void Start(const AutocompleteInput& input, bool minimal_changes);

 private:
  struct Builtin {
   public:
    Builtin(const string16& path, const GURL& url);

    string16 path;
    GURL url;
  };

  static const int kRelevance;

  std::vector<Builtin> builtins_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(BuiltinProvider);
};

#endif  // CHROME_BROWSER_AUTOCOMPLETE_BUILTIN_PROVIDER_H_
