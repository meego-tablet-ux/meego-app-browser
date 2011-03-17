# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'content_renderer',
      'msvs_guid': '9AAA8CF2-9B3D-4895-8CB9-D70BBD125EAD',
      'type': '<(library)',
      'dependencies': [
        'content_common',
        '../skia/skia.gyp:skia',
        '../third_party/ffmpeg/ffmpeg.gyp:ffmpeg',
        '../third_party/libjingle/libjingle.gyp:libjingle',
        '../third_party/npapi/npapi.gyp:npapi',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'renderer/active_notification_tracker.cc',
        'renderer/active_notification_tracker.h',
        'renderer/audio_device.cc',
        'renderer/audio_device.h',
        'renderer/audio_message_filter.cc',
        'renderer/audio_message_filter.h',
        'renderer/content_renderer_client.h',
        'renderer/cookie_message_filter.cc',
        'renderer/cookie_message_filter.h',
        'renderer/device_orientation_dispatcher.cc',
        'renderer/device_orientation_dispatcher.h',
        'renderer/geolocation_dispatcher.cc',
        'renderer/geolocation_dispatcher.h',
        'renderer/ggl.cc',
        'renderer/ggl.h',
        'renderer/gpu_channel_host.cc',
        'renderer/gpu_channel_host.h',
        'renderer/gpu_video_decoder_host.cc',
        'renderer/gpu_video_decoder_host.h',
        'renderer/gpu_video_service_host.cc',
        'renderer/gpu_video_service_host.h',
        'renderer/indexed_db_dispatcher.cc',
        'renderer/indexed_db_dispatcher.h',
        'renderer/media/audio_renderer_impl.cc',
        'renderer/media/audio_renderer_impl.h',
        'renderer/media/gles2_video_decode_context.cc',
        'renderer/media/gles2_video_decode_context.h',
        'renderer/media/ipc_video_decoder.cc',
        'renderer/media/ipc_video_decoder.h',
        'renderer/notification_provider.cc',
        'renderer/notification_provider.h',
        'renderer/p2p/ipc_network_manager.cc',
        'renderer/p2p/ipc_network_manager.h',
        'renderer/p2p/ipc_socket_factory.cc',
        'renderer/p2p/ipc_socket_factory.h',
        'renderer/p2p/socket_client.cc',
        'renderer/p2p/socket_client.h',
        'renderer/p2p/socket_dispatcher.cc',
        'renderer/p2p/socket_dispatcher.h',
        'renderer/plugin_channel_host.cc',
        'renderer/plugin_channel_host.h',
        'renderer/speech_input_dispatcher.cc',
        'renderer/speech_input_dispatcher.h',
        'renderer/webgraphicscontext3d_command_buffer_impl.cc',
        'renderer/webgraphicscontext3d_command_buffer_impl.h',
        'renderer/webplugin_delegate_proxy.cc',
        'renderer/webplugin_delegate_proxy.h',
        'renderer/websharedworker_proxy.cc',
        'renderer/websharedworker_proxy.h',
        'renderer/websharedworkerrepository_impl.cc',
        'renderer/websharedworkerrepository_impl.h',
        'renderer/webworker_base.cc',
        'renderer/webworker_base.h',
        'renderer/webworker_proxy.cc',
        'renderer/webworker_proxy.h',
      ],
      'conditions': [
        ['enable_gpu==1', {
          'dependencies': [
            '../gpu/gpu.gyp:gles2_c_lib',
          ],
          'sources': [
            'renderer/command_buffer_proxy.cc',
            'renderer/command_buffer_proxy.h',
          ],
        }],
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
          'dependencies': [
            '../build/linux/system.gyp:gtk',
          ],
        }],
      ],
    },
  ],
}
