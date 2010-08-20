# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'target_defaults': {
    'variables': {
      'base_target': 0,
      'base_extra_target': 0,
    },
    'target_conditions': [
      # This part is shared between the targets defined below.
      ['base_target==1', {
        'sources': [
          '../build/build_config.h',
          'third_party/dmg_fp/dmg_fp.h',
          'third_party/dmg_fp/dtoa.cc',
          'third_party/dmg_fp/g_fmt.cc',
          'third_party/icu/icu_utf.cc',
          'third_party/icu/icu_utf.h',
          'third_party/nspr/prtime.cc',
          'third_party/nspr/prtime.h',
          'at_exit.cc',
          'at_exit.h',
          'atomic_ref_count.h',
          'atomic_sequence_num.h',
          'atomicops.h',
          'atomicops_internals_x86_gcc.cc',
          'atomicops_internals_x86_msvc.h',
          'base_paths.cc',
          'base_paths.h',
          'base_paths_mac.h',
          'base_paths_mac.mm',
          'base_paths_posix.cc',
          'base_paths_win.cc',
          'base_paths_win.h',
          'base_switches.cc',
          'base_switches.h',
          'basictypes.h',
          'bits.h',
          'bzip2_error_handler.cc',
          'callback.h',
          'cancellation_flag.cc',
          'cancellation_flag.h',
          'chrome_application_mac.h',
          'chrome_application_mac.mm',
          'cocoa_protocols_mac.h',
          'command_line.cc',
          'command_line.h',
          'compiler_specific.h',
          'condition_variable.h',
          'condition_variable_posix.cc',
          'condition_variable_win.cc',
          'cpu.cc',
          'cpu.h',
          'debug_on_start.cc',
          'debug_on_start.h',
          'debug_util.cc',
          'debug_util.h',
          'debug_util_mac.cc',
          'debug_util_posix.cc',
          'debug_util_win.cc',
          'dir_reader_fallback.h',
          'dir_reader_linux.h',
          'dir_reader_posix.h',
          'environment.cc',
          'environment.h',
          'event_trace_consumer_win.h',
          'event_trace_controller_win.cc',
          'event_trace_controller_win.h',
          'event_trace_provider_win.cc',
          'event_trace_provider_win.h',
          'event_synthesis_gtk.cc',
          'event_synthesis_gtk.h',
          'file_path.cc',
          'file_path.h',
          'file_util.cc',
          'file_util.h',
          'file_util_deprecated.h',
          'file_util_linux.cc',
          'file_util_mac.mm',
          'file_util_posix.cc',
          'file_util_win.cc',
          'file_version_info.h',
          'file_version_info_mac.h',
          'file_version_info_mac.mm',
          'file_version_info_win.cc',
          'file_version_info_win.h',
          'fix_wp64.h',
          'float_util.h',
          'foundation_utils_mac.h',
          'global_descriptors_posix.cc',
          'global_descriptors_posix.h',
          'gtest_prod_util.h',
          'gtk_util.cc',
          'gtk_util.h',
          'hash_tables.h',
          'histogram.cc',
          'histogram.h',
          'iat_patch.cc',
          'iat_patch.h',
          'id_map.h',
          'json/json_reader.cc',
          'json/json_reader.h',
          'json/json_writer.cc',
          'json/json_writer.h',
          'json/string_escape.cc',
          'json/string_escape.h',
          'lazy_instance.cc',
          'lazy_instance.h',
          'leak_annotations.h',
          'leak_tracker.h',
          'linked_list.h',
          'linked_ptr.h',
          'lock.cc',
          'lock.h',
          'lock_impl.h',
          'lock_impl_posix.cc',
          'lock_impl_win.cc',
          'logging.cc',
          'logging.h',
          'logging_win.cc',
          'mac_util.h',
          'mac_util.mm',
          'mach_ipc_mac.h',
          'mach_ipc_mac.mm',
          'memory_debug.cc',
          'memory_debug.h',
          'message_loop.cc',
          'message_loop.h',
          'message_loop_proxy.h',
          'message_loop_proxy_impl.cc',
          'message_loop_proxy_impl.h',
          'message_pump.h',
          'message_pump_default.cc',
          'message_pump_default.h',
          'message_pump_win.cc',
          'message_pump_win.h',
          'mime_util.h',
          'mime_util_xdg.cc',
          'move.h',
          'native_library.h',
          'native_library_linux.cc',
          'native_library_mac.mm',
          'native_library_win.cc',
          'non_thread_safe.cc',
          'non_thread_safe.h',
          'nullable_string16.h',
          'object_watcher.cc',
          'object_watcher.h',
          'observer_list.h',
          'observer_list_threadsafe.h',
          'path_service.cc',
          'path_service.h',
          'pe_image.cc',
          'pe_image.h',
          'pickle.cc',
          'pickle.h',
          'platform_file.h',
          'platform_file_posix.cc',
          'platform_file_win.cc',
          'platform_thread.h',
          'platform_thread_mac.mm',
          'platform_thread_posix.cc',
          'platform_thread_win.cc',
          'port.h',
          'process.h',
          'process_linux.cc',
          'process_posix.cc',
          'process_util.cc',
          'process_util.h',
          'process_util_linux.cc',
          'process_util_mac.mm',
          'process_util_posix.cc',
          'process_util_win.cc',
          'process_win.cc',
          'profiler.cc',
          'profiler.h',
          'rand_util.cc',
          'rand_util.h',
          'rand_util_posix.cc',
          'rand_util_win.cc',
          'raw_scoped_refptr_mismatch_checker.h',
          'ref_counted.cc',
          'ref_counted.h',
          'ref_counted_memory.cc',
          'ref_counted_memory.h',
          'registry.cc',
          'registry.h',
          'resource_util.cc',
          'resource_util.h',
          'safe_strerror_posix.cc',
          'safe_strerror_posix.h',
          'scoped_aedesc.h',
          'scoped_bstr_win.cc',
          'scoped_bstr_win.h',
          'scoped_callback_factory.h',
          'scoped_cftyperef.h',
          'scoped_comptr_win.h',
          'scoped_handle.h',
          'scoped_handle_win.h',
          'scoped_nsautorelease_pool.h',
          'scoped_nsautorelease_pool.mm',
          'scoped_nsdisable_screen_updates.h',
          'scoped_nsobject.h',
          'scoped_open_process.h',
          'scoped_ptr.h',
          'scoped_temp_dir.cc',
          'scoped_temp_dir.h',
          'scoped_variant_win.cc',
          'scoped_variant_win.h',
          'scoped_vector.h',
          'sha1.h',
          'sha1_portable.cc',
          'sha1_win.cc',
          'shared_memory.h',
          'shared_memory_posix.cc',
          'shared_memory_win.cc',
          'simple_thread.cc',
          'simple_thread.h',
          'singleton.h',
          'spin_wait.h',
          'stack_container.h',
          'stats_counters.h',
          'stats_table.cc',
          'stats_table.h',
          'stl_util-inl.h',
          'string_number_conversions.cc',
          'string_number_conversions.h',
          'string_piece.cc',
          'string_piece.h',
          'string_split.cc',
          'string_split.h',
          'string_tokenizer.h',
          'string_util.cc',
          'string_util.h',
          'string_util_win.h',
          'stringprintf.cc',
          'stringprintf.h',
          'sys_info.h',
          'sys_info_chromeos.cc',
          'sys_info_freebsd.cc',
          'sys_info_linux.cc',
          'sys_info_mac.cc',
          'sys_info_openbsd.cc',
          'sys_info_posix.cc',
          'sys_info_win.cc',
          'sys_string_conversions.h',
          'sys_string_conversions_linux.cc',
          'sys_string_conversions_mac.mm',
          'sys_string_conversions_win.cc',
          'task.cc',
          'task.h',
          'template_util.h',
          'thread.cc',
          'thread.h',
          'thread_collision_warner.cc',
          'thread_collision_warner.h',
          'thread_local.h',
          'thread_local_posix.cc',
          'thread_local_storage.h',
          'thread_local_storage_posix.cc',
          'thread_local_storage_win.cc',
          'thread_local_win.cc',
          'time.cc',
          'time.h',
          'time_win.cc',
          'timer.cc',
          'timer.h',
          'trace_event_win.cc',
          'trace_event.cc',
          'trace_event.h',
          'tracked.cc',
          'tracked.h',
          'tracked_objects.cc',
          'tracked_objects.h',
          'tuple.h',
          'unix_domain_socket_posix.cc',
          'utf_offset_string_conversions.cc',
          'utf_offset_string_conversions.h',
          'utf_string_conversion_utils.cc',
          'utf_string_conversion_utils.h',
          'utf_string_conversions.cc',
          'utf_string_conversions.h',
          'values.cc',
          'values.h',
          'waitable_event.h',
          'waitable_event_posix.cc',
          'waitable_event_watcher.h',
          'waitable_event_watcher_posix.cc',
          'waitable_event_watcher_win.cc',
          'waitable_event_win.cc',
          'watchdog.cc',
          'watchdog.h',
          'weak_ptr.cc',
          'weak_ptr.h',
          'win_util.cc',
          'win_util.h',
          'windows_message_list.h',
          'wmi_util.cc',
          'wmi_util.h',
          'worker_pool.h',
          'worker_pool_linux.cc',
          'worker_pool_linux.h',
          'worker_pool_mac.h',
          'worker_pool_mac.mm',
          'worker_pool_win.cc',
          'xdg_util.h',
          'xdg_util.cc',
        ],
        'include_dirs': [
          '..',
        ],
        # These warnings are needed for the files in third_party\dmg_fp.
        'msvs_disabled_warnings': [
          4244, 4554, 4018, 4102,
        ],
        'mac_framework_dirs': [
          '$(SDKROOT)/System/Library/Frameworks/ApplicationServices.framework/Frameworks',
        ],
        'conditions': [
          [ 'OS != "linux" and OS != "freebsd" and OS != "openbsd" and OS != "solaris"', {
              'sources/': [
                ['exclude', '/xdg_[^/]*\\.cc$'],
              ],
              'sources!': [
                'atomicops_internals_x86_gcc.cc',
                'base_paths_posix.cc',
                'message_pump_glib.cc',
                'xdg_util.cc',
              ],
          }],
          [ 'OS != "linux"', {
              'sources!': [
                # Not automatically excluded by the *linux.cc rules.
                'gtk_util.cc',
                'gtk_util.h',
                'linux_util.cc',
                'setproctitle_linux.c',
                'setproctitle_linux.h',
              ],
            },
          ],
          [ 'OS != "mac"', {
              'sources!': [
                'scoped_aedesc.h'
              ],
          }],
          # For now, just test the *BSD platforms enough to exclude them.
          # Subsequent changes will include them further.
          [ 'OS != "freebsd"', {
              'sources/': [ ['exclude', '_freebsd\\.cc$'] ],
            },
          ],
          [ 'OS != "openbsd"', {
              'sources/': [ ['exclude', '_openbsd\\.cc$'] ],
            },
          ],
          [ 'OS == "win"', {
              'include_dirs': [
                '<(DEPTH)/third_party/wtl/include',
              ],
              'sources!': [
                'event_recorder_stubs.cc',
                'file_descriptor_shuffle.cc',
                'message_pump_libevent.cc',
                # Not using sha1_win.cc because it may have caused a
                # regression to page cycler moz.
                'sha1_win.cc',
                'string16.cc',
                'trace_event.cc',
              ],
          },],
        ],
      }],
      ['base_extra_target==1', {
        'sources': [
          'crypto/capi_util.cc',
          'crypto/capi_util.h',
          'crypto/cssm_init.cc',
          'crypto/cssm_init.h',
          'crypto/encryptor.h',
          'crypto/encryptor_mac.cc',
          'crypto/encryptor_nss.cc',
          'crypto/encryptor_win.cc',
          'crypto/rsa_private_key.h',
          'crypto/rsa_private_key.cc',
          'crypto/rsa_private_key_mac.cc',
          'crypto/rsa_private_key_nss.cc',
          'crypto/rsa_private_key_win.cc',
          'crypto/signature_creator.h',
          'crypto/signature_creator_mac.cc',
          'crypto/signature_creator_nss.cc',
          'crypto/signature_creator_win.cc',
          'crypto/signature_verifier.h',
          'crypto/signature_verifier_mac.cc',
          'crypto/signature_verifier_nss.cc',
          'crypto/signature_verifier_win.cc',
          'crypto/symmetric_key.h',
          'crypto/symmetric_key_mac.cc',
          'crypto/symmetric_key_nss.cc',
          'crypto/symmetric_key_win.cc',
          'third_party/nspr/prcpucfg.h',
          'third_party/nspr/prcpucfg_win.h',
          'third_party/nspr/prtypes.h',
          'third_party/nss/blapi.h',
          'third_party/nss/blapit.h',
          'third_party/nss/sha256.h',
          'third_party/nss/sha512.cc',
          'third_party/purify/pure.h',
          'third_party/purify/pure_api.c',
          'third_party/xdg_user_dirs/xdg_user_dir_lookup.cc',
          'third_party/xdg_user_dirs/xdg_user_dir_lookup.h',
          'auto_reset.h',
          'base64.cc',
          'base64.h',
          'base_drag_source.cc',
          'base_drag_source.h',
          'base_drop_target.cc',
          'base_drop_target.h',
          'data_pack.cc',
          'event_recorder.cc',
          'event_recorder.h',
          'event_recorder_stubs.cc',
          'field_trial.cc',
          'field_trial.h',
          'file_descriptor_shuffle.cc',
          'file_descriptor_shuffle.h',
          'hmac.h',
          'hmac_mac.cc',
          'hmac_nss.cc',
          'hmac_win.cc',
          'image_util.cc',
          'image_util.h',
          'keyboard_code_conversion.cc',
          'keyboard_code_conversion.h',
          'keyboard_code_conversion_gtk.cc',
          'keyboard_code_conversion_gtk.h',
          'keyboard_code_conversion_mac.mm',
          'keyboard_code_conversion_mac.h',
          'keyboard_codes.h',
          'keyboard_codes_win.h',
          'keyboard_codes_posix.h',
          'linux_util.cc',
          'linux_util.h',
          'md5.cc',
          'md5.h',
          'message_pump_glib.cc',
          'message_pump_glib.h',
          'message_pump_libevent.cc',
          'message_pump_libevent.h',
          'message_pump_mac.h',
          'message_pump_mac.mm',
          'nsimage_cache_mac.h',
          'nsimage_cache_mac.mm',
          'nss_util.cc',
          'nss_util.h',
          'setproctitle_linux.c',
          'setproctitle_linux.h',
          'sha2.cc',
          'sha2.h',
          'string16.cc',
          'string16.h',
          'sync_socket.h',
          'sync_socket_win.cc',
          'sync_socket_posix.cc',
          'time_mac.cc',
          'time_posix.cc',
          'version.cc',
          'version.h',
        ],
        'conditions': [
          [ 'OS == "linux" or OS == "freebsd" or OS == "openbsd" or OS == "solaris"', {
            'conditions': [
              [ 'chromeos==1', {
                  'sources/': [ ['include', '_chromeos\\.cc$'] ]
                },
              ],
            ],
            'defines': [
              'USE_SYMBOLIZE',
            ],
            'cflags': [
              '-Wno-write-strings',
            ],
          }, {  # OS != "linux" and OS != "freebsd" and OS != "openbsd" and OS != "solaris"
              'sources/': [
                ['exclude', '/xdg_user_dirs/'],
                ['exclude', '_nss\.cc$'],
              ],
          }],
          [ 'OS != "mac"', {
              'sources!': [
                'crypto/cssm_init.cc',
                'crypto/cssm_init.h',
              ],
          },],
          [ 'OS != "win"', {
              'sources!': [
                'third_party/purify/pure_api.c',
                'base_drag_source.cc',
                'base_drop_target.cc',
                'cpu.cc',
                'crypto/capi_util.h',
                'crypto/capi_util.cc',
                'debug_on_start.cc',
                'event_recorder.cc',
                'file_version_info.cc',
                'iat_patch.cc',
                'image_util.cc',
                'object_watcher.cc',
                'pe_image.cc',
                'registry.cc',
                'resource_util.cc',
                'win_util.cc',
                'wmi_util.cc',
              ],
          },],
        ],
      }],
    ],
  },
  'targets': [
    {
      'target_name': 'base',
      'type': '<(library)',
      'msvs_guid': '1832A374-8A74-4F9E-B536-69A699B3E165',
      'variables': {
        'base_target': 1,
        'base_extra_target': 1,
      },
      'dependencies': [
        '../third_party/modp_b64/modp_b64.gyp:modp_b64',
        'third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
      ],
      # TODO(gregoryd): direct_dependent_settings should be shared with the
      #  64-bit target, but it doesn't work due to a bug in gyp
      'direct_dependent_settings': {
        'include_dirs': [
          '..',
        ],
      },
      'conditions': [
        [ 'OS == "linux" or OS == "freebsd" or OS == "openbsd"', {
          'conditions': [
            [ 'linux_use_tcmalloc==0', {
                'defines': [
                  'NO_TCMALLOC',
                ],
                'direct_dependent_settings': {
                  'defines': [
                    'NO_TCMALLOC',
                  ],
                },
              },
            ],
            [ 'linux_use_tcmalloc==1', {
                'dependencies': [
                  'allocator/allocator.gyp:allocator',
                ],
              },
            ],
          ],
          'dependencies': [
            'symbolize',
            '../build/util/build_util.gyp:lastchange',
            '../build/linux/system.gyp:gtk',
            '../build/linux/system.gyp:nss',
            'xdg_mime',
          ],
          'export_dependent_settings': [
            '../build/linux/system.gyp:gtk',
          ],
        },],
        [ 'OS == "freebsd" or OS == "openbsd"', {
          'link_settings': {
            'libraries': [
              '-L/usr/local/lib -lexecinfo',
              ],
            },
          },
        ],
        [ 'OS == "linux"', {
          'link_settings': {
             'libraries': [
             # We need rt for clock_gettime().
             '-lrt',
             # For 'native_library_linux.cc'
             '-ldl',
           ],
         },
        }],
        [ 'OS == "mac"', {
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
              '$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
              '$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
              '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
              '$(SDKROOT)/System/Library/Frameworks/IOKit.framework',
              '$(SDKROOT)/System/Library/Frameworks/Security.framework',
            ],
          },
        },],
        [ 'OS == "mac" or OS == "win"', {
            'dependencies': [
              '../third_party/nss/nss.gyp:nss',
            ],
        },],
        [ 'OS != "win"', {
            'dependencies': ['../third_party/libevent/libevent.gyp:libevent'],
        },],
      ],
    },
  ],
  'conditions': [
    [ 'OS == "win"', {
      'targets': [
        {
          'target_name': 'base_nacl_win64',
          'type': '<(library)',
          'msvs_guid': 'CEE1F794-DC70-4FED-B7C4-4C12986672FE',
          'variables': {
            'base_target': 1,
          },
          'dependencies': [
            'third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations_win64',
          ],
          # TODO(gregoryd): direct_dependent_settings should be shared with the
          # 32-bit target, but it doesn't work due to a bug in gyp
          'direct_dependent_settings': {
            'include_dirs': [
              '..',
            ],
          },
          'defines': [
            '<@(nacl_win64_defines)',
          ],
          'sources': [
            'i18n/icu_util_nacl_win64.cc',
          ],
          'configurations': {
            'Common_Base': {
              'msvs_target_platform': 'x64',
            },
          },
        },
      ],
    }],
    [ 'OS == "linux" and internal_pdf', {
      'targets': [
        {
          'target_name': 'base_fpic',
          'type': '<(library)',
          'variables': {
            'base_target': 1,
            'base_extra_target': 1,
          },
          'cflags': [
            '-fPIC',
          ],
          # Official builds set -gstabs, which fails when building with -fPIC.
          # It's useless on 64 bits anyways because breakpad doesn't work then.
          'cflags!': [
            '-gstabs',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              '..',
            ],
          },
          'conditions': [
            [ 'OS == "linux" or OS == "freebsd" or OS == "openbsd"', {
              'dependencies': [
                '../build/util/build_util.gyp:lastchange',
                '../build/linux/system.gyp:gtk',
                '../build/linux/system.gyp:nss',
                'xdg_mime',
              ],
              'export_dependent_settings': [
                '../build/linux/system.gyp:gtk',
              ],
            },],
            ['OS == "linux"', {
              'link_settings': {
                'libraries': [
                  # We need rt for clock_gettime().
                  '-lrt',
                  # For 'native_library_linux.cc'
                  '-ldl',
                ],
              },
            }],
            [ 'OS == "mac" or OS == "win"', {
                'dependencies': [
                  '../third_party/nss/nss.gyp:nss',
                ],
            },],
            [ 'OS != "win"', {
                'dependencies': ['../third_party/libevent/libevent.gyp:libevent'],
            },],
          ],
        },
      ],
    }],
    [ 'OS == "linux" or OS == "freebsd" or OS == "openbsd" or OS == "solaris"', {
      'targets': [
        {
          'target_name': 'symbolize',
          'type': '<(library)',
          'variables': {
            'chromium_code': 0,
          },
          'conditions': [
            [ 'OS == "solaris"', {
              'include_dirs': [
                '/usr/gnu/include',
                '/usr/gnu/include/libelf',
              ],
            },],
          ],
          'cflags': [
            '-Wno-sign-compare',
          ],
          'cflags!': [
            '-Wextra',
          ],
          'sources': [
            'third_party/symbolize/symbolize.cc',
            'third_party/symbolize/demangle.cc',
          ],
        },
        {
          'target_name': 'xdg_mime',
          'type': '<(library)',
          'variables': {
            'chromium_code': 0,
          },
          'cflags!': [
            '-Wextra',
          ],
          'sources': [
            'third_party/xdg_mime/xdgmime.c',
            'third_party/xdg_mime/xdgmime.h',
            'third_party/xdg_mime/xdgmimealias.c',
            'third_party/xdg_mime/xdgmimealias.h',
            'third_party/xdg_mime/xdgmimecache.c',
            'third_party/xdg_mime/xdgmimecache.h',
            'third_party/xdg_mime/xdgmimeglob.c',
            'third_party/xdg_mime/xdgmimeglob.h',
            'third_party/xdg_mime/xdgmimeicon.c',
            'third_party/xdg_mime/xdgmimeicon.h',
            'third_party/xdg_mime/xdgmimeint.c',
            'third_party/xdg_mime/xdgmimeint.h',
            'third_party/xdg_mime/xdgmimemagic.c',
            'third_party/xdg_mime/xdgmimemagic.h',
            'third_party/xdg_mime/xdgmimeparent.c',
            'third_party/xdg_mime/xdgmimeparent.h',
          ],
        },
      ],
    }],
  ],
}
