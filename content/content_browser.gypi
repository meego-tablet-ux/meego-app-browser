# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'content_browser',
      'type': '<(library)',
      'dependencies': [
        # Don't include now since it's empty and so will cause a linker error.
        #'content_common',
        '../app/app.gyp:app_resources',
        '../skia/skia.gyp:skia',
        # TabContents uses zoom constants and functions from WebKit::WebView.
        '../third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:webkit',
        '../ui/ui.gyp:ui_base',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'browser/renderer_host/accelerated_surface_container_mac.cc',
        'browser/renderer_host/accelerated_surface_container_mac.h',
        'browser/renderer_host/accelerated_surface_container_manager_mac.cc',
        'browser/renderer_host/accelerated_surface_container_manager_mac.h',
        'browser/renderer_host/async_resource_handler.cc',
        'browser/renderer_host/async_resource_handler.h',
        'browser/renderer_host/audio_renderer_host.cc',
        'browser/renderer_host/audio_renderer_host.h',
        'browser/renderer_host/audio_sync_reader.cc',
        'browser/renderer_host/audio_sync_reader.h',
        'browser/renderer_host/backing_store.cc',
        'browser/renderer_host/backing_store.h',
        'browser/renderer_host/backing_store_mac.h',
        'browser/renderer_host/backing_store_mac.mm',
        'browser/renderer_host/backing_store_manager.cc',
        'browser/renderer_host/backing_store_manager.h',
        'browser/renderer_host/backing_store_skia.cc',
        'browser/renderer_host/backing_store_skia.h',
        'browser/renderer_host/backing_store_win.cc',
        'browser/renderer_host/backing_store_win.h',
        'browser/renderer_host/backing_store_x.cc',
        'browser/renderer_host/backing_store_x.h',
        'browser/renderer_host/blob_message_filter.cc',
        'browser/renderer_host/blob_message_filter.h',
        'browser/renderer_host/buffered_resource_handler.cc',
        'browser/renderer_host/buffered_resource_handler.h',        
        'browser/renderer_host/cross_site_resource_handler.cc',
        'browser/renderer_host/cross_site_resource_handler.h',
        'browser/renderer_host/database_message_filter.cc',
        'browser/renderer_host/database_message_filter.h',
        'browser/renderer_host/file_utilities_message_filter.cc',
        'browser/renderer_host/file_utilities_message_filter.h',
        'browser/renderer_host/global_request_id.h',
        'browser/renderer_host/gpu_message_filter.cc',
        'browser/renderer_host/gpu_message_filter.h',
        'browser/renderer_host/pepper_file_message_filter.cc',
        'browser/renderer_host/pepper_file_message_filter.h',
        'browser/renderer_host/pepper_message_filter.cc',
        'browser/renderer_host/pepper_message_filter.h',
        'browser/renderer_host/redirect_to_file_resource_handler.cc',
        'browser/renderer_host/redirect_to_file_resource_handler.h',
        'browser/renderer_host/render_message_filter.cc',
        'browser/renderer_host/render_message_filter.h',
        'browser/renderer_host/render_message_filter_gtk.cc',
        'browser/renderer_host/render_message_filter_mac.mm',
        'browser/renderer_host/render_message_filter_win.cc',
        'browser/renderer_host/render_process_host.cc',
        'browser/renderer_host/render_process_host.h',
        'browser/renderer_host/render_sandbox_host_linux.cc',
        'browser/renderer_host/render_sandbox_host_linux.h',
        'browser/renderer_host/render_view_host.cc',
        'browser/renderer_host/render_view_host.h',
        'browser/renderer_host/render_view_host_delegate.cc',
        'browser/renderer_host/render_view_host_delegate.h',
        'browser/renderer_host/render_view_host_factory.cc',
        'browser/renderer_host/render_view_host_factory.h',
        'browser/renderer_host/render_view_host_notification_task.h',
        'browser/renderer_host/render_widget_fullscreen_host.cc',
        'browser/renderer_host/render_widget_fullscreen_host.h',
        'browser/renderer_host/render_widget_helper.cc',
        'browser/renderer_host/render_widget_helper.h',
        'browser/renderer_host/render_widget_host.cc',
        'browser/renderer_host/render_widget_host.h',
        'browser/renderer_host/render_widget_host_view.cc',
        'browser/renderer_host/render_widget_host_view.h',
        'browser/renderer_host/resource_dispatcher_host.cc',
        'browser/renderer_host/resource_dispatcher_host.h',
        'browser/renderer_host/resource_dispatcher_host_request_info.cc',
        'browser/renderer_host/resource_dispatcher_host_request_info.h',     
        'browser/renderer_host/resource_handler.h',
        'browser/renderer_host/resource_message_filter.cc',
        'browser/renderer_host/resource_message_filter.h',
        'browser/renderer_host/resource_queue.cc',
        'browser/renderer_host/resource_queue.h',
        'browser/renderer_host/resource_request_details.cc',
        'browser/renderer_host/resource_request_details.h',
        'browser/renderer_host/socket_stream_dispatcher_host.cc',
        'browser/renderer_host/socket_stream_dispatcher_host.h',
        'browser/renderer_host/socket_stream_host.cc',
        'browser/renderer_host/socket_stream_host.h',
        'browser/renderer_host/sync_resource_handler.cc',
        'browser/renderer_host/sync_resource_handler.h',
        'browser/renderer_host/x509_user_cert_resource_handler.cc',
        'browser/renderer_host/x509_user_cert_resource_handler.h',
        'browser/site_instance.cc',
        'browser/site_instance.h',
        'browser/tab_contents/background_contents.cc',
        'browser/tab_contents/background_contents.h',
        'browser/tab_contents/constrained_window.h',
        'browser/tab_contents/infobar_delegate.cc',
        'browser/tab_contents/infobar_delegate.h',
        'browser/tab_contents/interstitial_page.cc',
        'browser/tab_contents/interstitial_page.h',
        'browser/tab_contents/language_state.cc',
        'browser/tab_contents/language_state.h',
        'browser/tab_contents/navigation_controller.cc',
        'browser/tab_contents/navigation_controller.h',
        'browser/tab_contents/navigation_entry.cc',
        'browser/tab_contents/navigation_entry.h',
        'browser/tab_contents/page_navigator.h',
        'browser/tab_contents/provisional_load_details.cc',
        'browser/tab_contents/provisional_load_details.h',
        'browser/tab_contents/render_view_host_manager.cc',
        'browser/tab_contents/render_view_host_manager.h',
        'browser/tab_contents/tab_contents.cc',
        'browser/tab_contents/tab_contents.h',
        'browser/tab_contents/tab_contents_delegate.cc',
        'browser/tab_contents/tab_contents_delegate.h',
        'browser/tab_contents/tab_contents_observer.cc',
        'browser/tab_contents/tab_contents_observer.h',
        'browser/tab_contents/tab_contents_view.cc',
        'browser/tab_contents/tab_contents_view.h',
      ],
      'conditions': [
        ['OS=="win"', {
          'msvs_guid': '639DB58D-32C2-435A-A711-65A12F62E442',
        }],
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
          'dependencies': [
            '../build/linux/system.gyp:gtk',
          ],
        }],
      ],
    },
  ],
}
