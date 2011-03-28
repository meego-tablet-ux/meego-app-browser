// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_client_auth_handler.h"

#include <cert.h>
//#include <gtk/gtk.h>

#include <string>
#include <vector>

#include "ui/base/l10n/l10n_util.h"
#include "base/i18n/time_formatting.h"
#include "base/logging.h"
#include "base/nss_util.h"
#include "base/utf_string_conversions.h"
//#include "chrome/browser/ui/gtk/certificate_viewer.h"
//#include "chrome/browser/ui/gtk/gtk_util.h"
#include "chrome/third_party/mozilla_security_manager/nsNSSCertHelper.h"
#include "chrome/third_party/mozilla_security_manager/nsNSSCertificate.h"
#include "chrome/third_party/mozilla_security_manager/nsUsageArrayHelper.h"
#include "ui/gfx/native_widget_types.h"
#include "grit/generated_resources.h"
#include "net/base/x509_certificate.h"

///////////////////////////////////////////////////////////////////////////////
// SSLClientAuthHandler platform specific implementation:

void SSLClientAuthHandler::DoSelectCertificate() {
}
