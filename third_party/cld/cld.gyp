# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'includes': [
    '../../build/common.gypi',
  ],
  'conditions': [
    ['OS=="win"', {
      'targets': [
        {
          'target_name': 'cld',
          'type': '<(library)',
          'dependencies': [
            '../../base/base.gyp:base',
          ],
          'msvs_disabled_warnings': [4005, 4006, 4018, 4244, 4309, 4800],
          'defines': [
            'CLD_WINDOWS',
          ],
          'sources': [
            'bar/common/scopedlibrary.h',
            'bar/common/scopedptr.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/cldutil.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/cldutil.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/cldutil_dbg.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/cldutil_dbg_empty.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/compact_lang_det.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/compact_lang_det.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/compact_lang_det_generated_cjkbis_0.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/compact_lang_det_generated_ctjkvz.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/compact_lang_det_generated_longwords8_0.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/compact_lang_det_generated_meanscore.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/compact_lang_det_generated_quads_128.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/compact_lang_det_impl.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/compact_lang_det_impl.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/ext_lang_enc.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/ext_lang_enc.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/getonescriptspan.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/getonescriptspan.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/letterscript_enum.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/letterscript_enum.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/subsetsequence.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/subsetsequence.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/tote.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/tote.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/unittest_data.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/utf8propjustletter.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/utf8propletterscriptnum.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/utf8scannotjustletterspecial.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_basictypes.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_commandlineflags.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_macros.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_google.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_htmlutils.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_htmlutils_windows.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_logging.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_scoped_ptr.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_scopedptr.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_strtoint.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_unicodetext.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_unicodetext.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_unilib.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_unilib_windows.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_utf.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_utf8statetable.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_utf8statetable.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_utf8utils.h',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/cld_utf8utils_windows.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/normalizedunicodetext.cc',
            'bar/toolbar/cld/i18n/encodings/compact_lang_det/win/normalizedunicodetext.h',
            'bar/toolbar/cld/i18n/encodings/internal/encodings.cc',
            'bar/toolbar/cld/i18n/encodings/proto/encodings.pb.h',
            'bar/toolbar/cld/i18n/encodings/public/encodings.h',
            'bar/toolbar/cld/i18n/languages/internal/languages.cc',
            'bar/toolbar/cld/i18n/languages/proto/languages.pb.h',
            'bar/toolbar/cld/i18n/languages/public/languages.h',
            'base/casts.h',
            'base/commandlineflags.h',
            'base/stl_decl.h',
            'base/global_strip_options.h',
            'base/logging.h',
            'base/macros.h',
            'base/crash.h',
            'base/dynamic_annotations.h',
            'base/scoped_ptr.h',
            'base/stl_decl_msvc.h',
            'base/log_severity.h',
            'base/strtoint.h',
            'base/vlog_is_on.h',
            'base/type_traits.h',
            'base/template_util.h',
          ],
          'direct_dependent_settings': {
            'defines': [
              'CLD_WINDOWS',
              'COMPILER_MSVC',
            ],
          },
        },],
      },
    ],
  ],
}
