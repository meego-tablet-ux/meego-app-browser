// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/backend/print_backend.h"

#include <dlfcn.h>
#include <errno.h>
#include <gcrypt.h>
#include <pthread.h>

#include "base/file_util.h"
#include "base/lock.h"
#include "base/logging.h"
#include "base/singleton.h"
#include "base/string_number_conversions.h"
#include "base/values.h"
#include "googleurl/src/gurl.h"
#include "printing/backend/cups_helper.h"

GCRY_THREAD_OPTION_PTHREAD_IMPL;

namespace {

// Init GCrypt library (needed for CUPS) using pthreads.
// There exists a bug in CUPS library, where it crashed with: "ath.c:184:
// _gcry_ath_mutex_lock: Assertion `*lock == ((ath_mutex_t) 0)' failed."
// It happened when multiple threads tried printing simultaneously.
// Google search for 'gnutls thread safety' provided a solution that
// initialized gcrypt and gnutls.

// Initially, we linked with -lgnutls and simply called gnutls_global_init(),
// but this did not work well since we build one binary on Ubuntu Hardy and
// expect it to run on many Linux distros. (See http://crbug.com/46954)
// So instead we use dlopen() and dlsym() to dynamically load and call
// gnutls_global_init().

class GcryptInitializer {
 public:
  GcryptInitializer() {
    Init();
  }

 private:
  void Init() {
    gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
    const char* kGnuTlsFile = "libgnutls.so";
    void* gnutls_lib = dlopen(kGnuTlsFile, RTLD_NOW);
    if (!gnutls_lib) {
      LOG(ERROR) << "Cannot load " << kGnuTlsFile;
      return;
    }
    const char* kGnuTlsInitFuncName = "gnutls_global_init";
    int (*pgnutls_global_init)(void) = reinterpret_cast<int(*)()>(
        dlsym(gnutls_lib, kGnuTlsInitFuncName));
    if (!pgnutls_global_init) {
      LOG(ERROR) << "Could not find " << kGnuTlsInitFuncName
                 << " in " << kGnuTlsFile;
      return;
    }
    if ((*pgnutls_global_init)() != 0)
      LOG(ERROR) << "Gnutls initialization failed";
  }
};

}  // namespace

namespace printing {

static const char kCUPSPrinterInfoOpt[] = "printer-info";
static const char kCUPSPrinterStateOpt[] = "printer-state";
static const char kCUPSPrintServerURL[] = "print_server_url";

class PrintBackendCUPS : public PrintBackend {
 public:
  explicit PrintBackendCUPS(const GURL& print_server_url);
  virtual ~PrintBackendCUPS() {}

  // PrintBackend implementation.
  virtual void EnumeratePrinters(PrinterList* printer_list);

  virtual bool GetPrinterCapsAndDefaults(const std::string& printer_name,
                                         PrinterCapsAndDefaults* printer_info);

  virtual bool IsValidPrinter(const std::string& printer_name);

 private:
  // Following functions are wrappers around corresponding CUPS functions.
  // <functions>2()  are called when print server is specified, and plain
  // version in another case. There is an issue specifing CUPS_HTTP_DEFAULT
  // in the <functions>2(), it does not work in CUPS prior to 1.4.
  int GetDests(cups_dest_t** dests);
  FilePath GetPPD(const char* name);

  GURL print_server_url_;
};

PrintBackendCUPS::PrintBackendCUPS(const GURL& print_server_url)
    : print_server_url_(print_server_url) {
}

void PrintBackendCUPS::EnumeratePrinters(PrinterList* printer_list) {
  DCHECK(printer_list);
  printer_list->clear();

  cups_dest_t* destinations = NULL;
  int num_dests = GetDests(&destinations);

  for (int printer_index = 0; printer_index < num_dests; printer_index++) {
    const cups_dest_t& printer = destinations[printer_index];

    PrinterBasicInfo printer_info;
    printer_info.printer_name = printer.name;

    const char* info = cupsGetOption(kCUPSPrinterInfoOpt,
        printer.num_options, printer.options);
    if (info != NULL)
      printer_info.printer_description = info;

    const char* state = cupsGetOption(kCUPSPrinterStateOpt,
        printer.num_options, printer.options);
    if (state != NULL)
      base::StringToInt(state, &printer_info.printer_status);

    // Store printer options.
    for (int opt_index = 0; opt_index < printer.num_options; opt_index++) {
      printer_info.options[printer.options[opt_index].name] =
          printer.options[opt_index].value;
    }

    printer_list->push_back(printer_info);
  }

  cupsFreeDests(num_dests, destinations);

  LOG(INFO) << "CUPS: Enumerated " << printer_list->size() << " printers.";
}

bool PrintBackendCUPS::GetPrinterCapsAndDefaults(
    const std::string& printer_name,
    PrinterCapsAndDefaults* printer_info) {
  DCHECK(printer_info);

  LOG(INFO) << "CUPS: Getting Caps and Defaults for: " << printer_name;

  FilePath ppd_path(GetPPD(printer_name.c_str()));
  // In some cases CUPS failed to get ppd file.
  if (ppd_path.empty()) {
    LOG(ERROR) << "CUPS: Failed to get PPD for: " << printer_name;
    return false;
  }

  std::string content;
  bool res = file_util::ReadFileToString(ppd_path, &content);

  file_util::Delete(ppd_path, false);

  if (res) {
    printer_info->printer_capabilities.swap(content);
    printer_info->caps_mime_type = "application/pagemaker";
    // In CUPS, printer defaults is a part of PPD file. Nothing to upload here.
    printer_info->printer_defaults.clear();
    printer_info->defaults_mime_type.clear();
  }

  return res;
}

bool PrintBackendCUPS::IsValidPrinter(const std::string& printer_name) {
  // This is not very efficient way to get specific printer info. CUPS 1.4
  // supports cupsGetNamedDest() function. However, CUPS 1.4 is not available
  // everywhere (for example, it supported from Mac OS 10.6 only).
  PrinterList printer_list;
  EnumeratePrinters(&printer_list);

  PrinterList::iterator it;
  for (it = printer_list.begin(); it != printer_list.end(); ++it)
    if (it->printer_name == printer_name)
      return true;
  return false;
}

scoped_refptr<PrintBackend> PrintBackend::CreateInstance(
    const DictionaryValue* print_backend_settings) {
  // Initialize gcrypt library.
  Singleton<GcryptInitializer>::get();

  std::string print_server_url_str;
  if (print_backend_settings) {
    print_backend_settings->GetString(kCUPSPrintServerURL,
                                      &print_server_url_str);
  }
  GURL print_server_url(print_server_url_str.c_str());
  return new PrintBackendCUPS(print_server_url);
}

int PrintBackendCUPS::GetDests(cups_dest_t** dests) {
  if (print_server_url_.is_empty()) {  // Use default (local) print server.
    return cupsGetDests(dests);
  } else {
    HttpConnectionCUPS http(print_server_url_);
    return cupsGetDests2(http.http(), dests);
  }
}

FilePath PrintBackendCUPS::GetPPD(const char* name) {
  // cupsGetPPD returns a filename stored in a static buffer in CUPS.
  // Protect this code with lock.
  static Lock ppd_lock;
  AutoLock ppd_autolock(ppd_lock);
  FilePath ppd_path;
  const char* ppd_file_path = NULL;
  if (print_server_url_.is_empty()) {  // Use default (local) print server.
    ppd_file_path = cupsGetPPD(name);
  } else {
    HttpConnectionCUPS http(print_server_url_);
    ppd_file_path = cupsGetPPD2(http.http(), name);
  }
  if (ppd_file_path)
    ppd_path = FilePath(ppd_file_path);
  return ppd_path;
}

}  // namespace printing
