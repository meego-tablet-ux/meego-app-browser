// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/print_settings_initializer_mac.h"

#include "base/sys_string_conversions.h"
#include "printing/print_settings.h"

namespace printing {

// static
void PrintSettingsInitializerMac::InitPrintSettings(
    PMPrinter printer,
    PMPageFormat page_format,
    const PageRanges& new_ranges,
    bool print_selection_only,
    PrintSettings* print_settings) {
  DCHECK(print_settings);

  print_settings->set_printer_name(
      base::SysCFStringRefToWide(PMPrinterGetName(printer)));
  print_settings->set_device_name(
      base::SysCFStringRefToWide(PMPrinterGetID(printer)));
  print_settings->ranges = new_ranges;

  PMOrientation orientation = kPMPortrait;
  PMGetOrientation(page_format, &orientation);
  print_settings->set_landscape(orientation == kPMLandscape);
  print_settings->selection_only = print_selection_only;

  UInt32 resolution_count = 0;
  PMResolution best_resolution = { 72.0, 72.0 };
  OSStatus status = PMPrinterGetPrinterResolutionCount(printer,
                                                       &resolution_count);
  if (status == noErr) {
    // Resolution indexes are 1-based.
    for (uint32 i = 1; i <= resolution_count; ++i) {
      PMResolution resolution;
      PMPrinterGetIndexedPrinterResolution(printer, i, &resolution);
      if (resolution.hRes > best_resolution.hRes)
        best_resolution = resolution;
    }
  }
  int dpi = best_resolution.hRes;
  print_settings->set_dpi(dpi);

  DCHECK_EQ(dpi, best_resolution.vRes);

  // Get printable area and paper rects (in points)
  PMRect page_rect, paper_rect;
  PMGetAdjustedPageRect(page_format, &page_rect);
  PMGetAdjustedPaperRect(page_format, &paper_rect);

  // Device units are in points. Units per inch is 72.
  gfx::Size physical_size_device_units(
      (paper_rect.right - paper_rect.left),
      (paper_rect.bottom - paper_rect.top));
  gfx::Rect printable_area_device_units(
      (page_rect.left - paper_rect.left),
      (page_rect.top - paper_rect.top),
      (page_rect.right - page_rect.left),
      (page_rect.bottom - page_rect.top));

  print_settings->SetPrinterPrintableArea(physical_size_device_units,
                                          printable_area_device_units,
                                          72);
}

}  // namespace printing
