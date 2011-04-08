# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'ppapi_c',
      'type': 'none',
      'all_dependent_settings': {
        'include_dirs': [
          '..',
        ],
      },
      'sources': [
        'c/pp_bool.h',
        'c/pp_completion_callback.h',
        'c/pp_errors.h',
        'c/pp_input_event.h',
        'c/pp_instance.h',
        'c/pp_macros.h',
        'c/pp_module.h',
        'c/pp_point.h',
        'c/pp_rect.h',
        'c/pp_resource.h',
        'c/pp_size.h',
        'c/pp_stdint.h',
        'c/pp_time.h',
        'c/pp_var.h',
        'c/ppb.h',
        'c/ppb_audio.h',
        'c/ppb_audio_config.h',
        'c/ppb_core.h',
        'c/ppb_graphics_2d.h',
        'c/ppb_image_data.h',
        'c/ppb_instance.h',
        'c/ppb_messaging.h',
        'c/ppb_url_loader.h',
        'c/ppb_url_request_info.h',
        'c/ppb_url_response_info.h',
        'c/ppb_var.h',
        'c/ppp.h',
        'c/ppp_instance.h',
        'c/ppp_messaging.h',

        # Dev interfaces.
        'c/dev/pp_cursor_type_dev.h',
        'c/dev/pp_file_info_dev.h',
        'c/dev/pp_graphics_3d_dev.h',
        'c/dev/pp_video_dev.h',
        'c/dev/ppb_buffer_dev.h',
        'c/dev/ppb_char_set_dev.h',
        'c/dev/ppb_context_3d_dev.h',
        'c/dev/ppb_context_3d_trusted_dev.h',
        'c/dev/ppb_console_dev.h',
        'c/dev/ppb_cursor_control_dev.h',
        'c/dev/ppb_directory_reader_dev.h',
        'c/dev/ppb_file_chooser_dev.h',
        'c/dev/ppb_file_io_dev.h',
        'c/dev/ppb_file_io_trusted_dev.h',
        'c/dev/ppb_file_ref_dev.h',
        'c/dev/ppb_file_system_dev.h',
        'c/dev/ppb_find_dev.h',
        'c/dev/ppb_font_dev.h',
        'c/dev/ppb_font_list_dev.h',
        'c/dev/ppb_fullscreen_dev.h',
        'c/dev/ppb_graphics_3d_dev.h',
        'c/dev/ppb_opengles_dev.h',
        'c/dev/ppb_scrollbar_dev.h',
        'c/dev/ppb_surface_3d_dev.h',
        'c/dev/ppb_testing_dev.h',
        'c/dev/ppb_transport_dev.h',
        'c/dev/ppb_url_util_dev.h',
        'c/dev/ppb_video_decoder_dev.h',
        'c/dev/ppb_widget_dev.h',
        'c/dev/ppb_zoom_dev.h',
        'c/dev/ppp_cursor_control_dev.h',
        'c/dev/ppp_find_dev.h',
        'c/dev/ppp_graphics_3d_dev.h',
        'c/dev/ppp_scrollbar_dev.h',
        'c/dev/ppp_selection_dev.h',
        'c/dev/ppp_printing_dev.h',
        'c/dev/ppp_video_decoder_dev.h',
        'c/dev/ppp_widget_dev.h',
        'c/dev/ppp_zoom_dev.h',

        # Private interfaces.
        'c/private/ppb_flash.h',
        'c/private/ppb_flash_clipboard.h',
        'c/private/ppb_flash_file.h',
        'c/private/ppb_flash_menu.h',
        'c/private/ppb_flash_net_connector.h',
        'c/private/ppb_nacl_private.h',
        'c/private/ppb_pdf.h',
        'c/private/ppb_proxy_private.h',

        # Deprecated interfaces.
        'c/dev/deprecated_bool.h',
        'c/dev/ppb_var_deprecated.h',
        'c/dev/ppp_class_deprecated.h',

        # Trusted interfaces.
        'c/trusted/ppb_audio_trusted.h',
        'c/trusted/ppb_image_data_trusted.h',
        'c/trusted/ppb_broker_trusted.h',
        'c/trusted/ppb_url_loader_trusted.h',
      ],
    },
    {
      'target_name': 'ppapi_cpp_objects',
      'type': 'static_library',
      'dependencies': [
        'ppapi_c'
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'cpp/audio.cc',
        'cpp/audio.h',
        'cpp/audio_config.cc',
        'cpp/audio_config.h',
        'cpp/common.h',
        'cpp/completion_callback.h',
        'cpp/core.cc',
        'cpp/core.h',
        'cpp/graphics_2d.cc',
        'cpp/graphics_2d.h',
        'cpp/image_data.cc',
        'cpp/image_data.h',
        'cpp/instance.cc',
        'cpp/instance.h',
        'cpp/logging.h',
        'cpp/module.cc',
        'cpp/module.h',
        'cpp/module_impl.h',
        'cpp/non_thread_safe_ref_count.h',
        'cpp/paint_aggregator.cc',
        'cpp/paint_aggregator.h',
        'cpp/paint_manager.cc',
        'cpp/paint_manager.h',
        'cpp/point.h',
        'cpp/rect.cc',
        'cpp/rect.h',
        'cpp/resource.cc',
        'cpp/resource.h',
        'cpp/size.h',
        'cpp/url_loader.cc',
        'cpp/url_loader.h',
        'cpp/url_request_info.cc',
        'cpp/url_request_info.h',
        'cpp/url_response_info.cc',
        'cpp/url_response_info.h',
        'cpp/var.cc',
        'cpp/var.h',

        # Dev interfaces.
        'cpp/dev/buffer_dev.cc',
        'cpp/dev/buffer_dev.h',
        'cpp/dev/context_3d_dev.cc',
        'cpp/dev/context_3d_dev.h',
        'cpp/dev/directory_entry_dev.cc',
        'cpp/dev/directory_entry_dev.h',
        'cpp/dev/directory_reader_dev.cc',
        'cpp/dev/directory_reader_dev.h',
        'cpp/dev/file_chooser_dev.cc',
        'cpp/dev/file_chooser_dev.h',
        'cpp/dev/file_io_dev.cc',
        'cpp/dev/file_io_dev.h',
        'cpp/dev/file_ref_dev.cc',
        'cpp/dev/file_ref_dev.h',
        'cpp/dev/file_system_dev.cc',
        'cpp/dev/file_system_dev.h',
        'cpp/dev/find_dev.cc',
        'cpp/dev/find_dev.h',
        'cpp/dev/font_dev.cc',
        'cpp/dev/font_dev.h',
        'cpp/dev/fullscreen_dev.cc',
        'cpp/dev/fullscreen_dev.h',
        'cpp/dev/graphics_3d_client_dev.cc',
        'cpp/dev/graphics_3d_client_dev.h',
        'cpp/dev/graphics_3d_dev.cc',
        'cpp/dev/graphics_3d_dev.h',
        'cpp/dev/printing_dev.cc',
        'cpp/dev/printing_dev.h',
        'cpp/dev/scrollbar_dev.cc',
        'cpp/dev/scrollbar_dev.h',
        'cpp/dev/selection_dev.cc',
        'cpp/dev/selection_dev.h',
        'cpp/dev/surface_3d_dev.cc',
        'cpp/dev/surface_3d_dev.h',
        'cpp/dev/transport_dev.cc',
        'cpp/dev/transport_dev.h',
        'cpp/dev/url_util_dev.cc',
        'cpp/dev/url_util_dev.h',
        'cpp/dev/video_decoder_dev.cc',
        'cpp/dev/video_decoder_dev.h',
        'cpp/dev/widget_client_dev.cc',
        'cpp/dev/widget_client_dev.h',
        'cpp/dev/widget_dev.cc',
        'cpp/dev/widget_dev.h',
        'cpp/dev/zoom_dev.cc',
        'cpp/dev/zoom_dev.h',

        # Deprecated interfaces.
        'cpp/dev/scriptable_object_deprecated.h',
        'cpp/dev/scriptable_object_deprecated.cc',
      ],
      'conditions': [
        ['OS=="win"', {
          'msvs_guid': 'AD371A1D-3459-4E2D-8E8A-881F4B83B908',
          'msvs_settings': {
            'VCCLCompilerTool': {
              'AdditionalOptions': ['/we4244'],  # implicit conversion, possible loss of data
            },
          },
        }],
        ['OS=="linux"', {
          'cflags': ['-Wextra', '-pedantic'],
        }],
        ['OS=="mac"', {
          'xcode_settings': {
            'WARNING_CFLAGS': ['-Wextra', '-pedantic'],
           },
        }]
      ],
    },
    {
      'target_name': 'ppapi_cpp',
      'type': 'static_library',
      'dependencies': [
        'ppapi_c',
        'ppapi_cpp_objects',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'cpp/module_embedder.h',
        'cpp/ppp_entrypoints.cc',
      ],
      'conditions': [
        ['OS=="win"', {
          'msvs_guid': '057E7FA0-83C0-11DF-8395-0800200C9A66',
        }],
        ['OS=="linux"', {
          'cflags': ['-Wextra', '-pedantic'],
        }],
        ['OS=="mac"', {
          'xcode_settings': {
            'WARNING_CFLAGS': ['-Wextra', '-pedantic'],
           },
        }]
      ],
    },
  ],
}
