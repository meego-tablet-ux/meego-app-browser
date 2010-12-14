# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'renderer',
      'type': '<(library)',
      'msvs_guid': '9301A569-5D2B-4D11-9332-B1E30AEACB8D',
      'dependencies': [
        'common',
        'common_net',
        'plugin',
        'chrome_resources',
        'chrome_strings',
        'safe_browsing_proto',
        '../ppapi/ppapi.gyp:ppapi_proxy',
        '../printing/printing.gyp:printing',
        '../skia/skia.gyp:skia',
        '../third_party/hunspell/hunspell.gyp:hunspell',
        '../third_party/cld/cld.gyp:cld',
        '../third_party/ffmpeg/ffmpeg.gyp:ffmpeg',
        '../third_party/icu/icu.gyp:icui18n',
        '../third_party/icu/icu.gyp:icuuc',
        '../third_party/npapi/npapi.gyp:npapi',
        '../third_party/WebKit/WebKit/chromium/WebKit.gyp:webkit',
        '../webkit/support/webkit_support.gyp:glue',
        '../webkit/support/webkit_support.gyp:webkit_resources',
      ],
      'include_dirs': [
        '..',
        '../third_party/cld',
      ],
      'defines': [
        '<@(nacl_defines)',
      ],
      'direct_dependent_settings': {
        'defines': [
          '<@(nacl_defines)',
        ],
      },
      'sources': [
        # TODO(jrg): to link ipc_tests, these files need to be in renderer.a.
        # But app/ is the wrong directory for them.
        # Better is to remove the dep of *_tests on renderer, but in the
        # short term I'd like the build to work.
        'renderer/autofill_helper.cc',
        'renderer/autofill_helper.h',
        'renderer/automation/dom_automation_controller.cc',
        'renderer/automation/dom_automation_controller.h',
        'renderer/automation/dom_automation_v8_extension.cc',
        'renderer/automation/dom_automation_v8_extension.h',
        'renderer/extensions/bindings_utils.cc',
        'renderer/extensions/bindings_utils.h',
        'renderer/extensions/chrome_app_bindings.cc',
        'renderer/extensions/chrome_app_bindings.h',
        'renderer/extensions/extension_renderer_info.cc',
        'renderer/extensions/extension_renderer_info.h',
        'renderer/extensions/event_bindings.cc',
        'renderer/extensions/event_bindings.h',
        'renderer/extensions/extension_process_bindings.cc',
        'renderer/extensions/extension_process_bindings.h',
        'renderer/extensions/js_only_v8_extensions.cc',
        'renderer/extensions/js_only_v8_extensions.h',
        'renderer/extensions/renderer_extension_bindings.cc',
        'renderer/extensions/renderer_extension_bindings.h',
        'renderer/external_popup_menu.cc',
        'renderer/external_popup_menu.h',
        'renderer/ggl/ggl.cc',
        'renderer/ggl/ggl.h',
        'renderer/loadtimes_extension_bindings.h',
        'renderer/loadtimes_extension_bindings.cc',
        'renderer/media/audio_renderer_impl.cc',
        'renderer/media/audio_renderer_impl.h',
        'renderer/media/gles2_video_decode_context.cc',
        'renderer/media/gles2_video_decode_context.h',
        'renderer/media/ipc_video_decoder.cc',
        'renderer/media/ipc_video_decoder.h',
        'renderer/net/predictor_queue.cc',
        'renderer/net/predictor_queue.h',
        'renderer/net/renderer_net_predictor.cc',
        'renderer/net/renderer_net_predictor.h',
        'renderer/resources/event_bindings.js',
        'renderer/resources/extension_apitest.js',
        'renderer/resources/extension_process_bindings.js',
        'renderer/resources/greasemonkey_api.js',
        'renderer/resources/json_schema.js',
        'renderer/resources/renderer_extension_bindings.js',
        'renderer/about_handler.cc',
        'renderer/about_handler.h',
        'renderer/audio_message_filter.cc',
        'renderer/audio_message_filter.h',
        'renderer/blocked_plugin.cc',
        'renderer/blocked_plugin.h',
        'renderer/cookie_message_filter.cc',
        'renderer/cookie_message_filter.h',
        'renderer/device_orientation_dispatcher.cc',
        'renderer/device_orientation_dispatcher.h',
        'renderer/devtools_agent.cc',
        'renderer/devtools_agent.h',
        'renderer/devtools_agent_filter.cc',
        'renderer/devtools_agent_filter.h',
        'renderer/devtools_client.cc',
        'renderer/devtools_client.h',
        'renderer/dom_ui_bindings.cc',
        'renderer/dom_ui_bindings.h',
        'renderer/extension_groups.h',
        'renderer/external_host_bindings.cc',
        'renderer/external_host_bindings.h',
        'renderer/external_extension.cc',
        'renderer/external_extension.h',
        'renderer/form_manager.cc',
        'renderer/form_manager.h',
        'renderer/geolocation_dispatcher.cc',
        'renderer/geolocation_dispatcher.h',
        'renderer/geolocation_dispatcher_old.cc',
        'renderer/geolocation_dispatcher_old.h',
        'renderer/gpu_channel_host.cc',
        'renderer/gpu_channel_host.h',
        'renderer/gpu_video_decoder_host.cc',
        'renderer/gpu_video_decoder_host.h',
        'renderer/gpu_video_service_host.cc',
        'renderer/gpu_video_service_host.h',
        'renderer/indexed_db_dispatcher.cc',
        'renderer/indexed_db_dispatcher.h',
        'renderer/localized_error.cc',
        'renderer/localized_error.h',
        'renderer/navigation_state.cc',
        'renderer/navigation_state.h',
        'renderer/notification_provider.cc',
        'renderer/notification_provider.h',
        'renderer/paint_aggregator.cc',
        'renderer/page_click_listener.h',
        'renderer/page_click_tracker.cc',
        'renderer/page_click_tracker.h',
        'renderer/page_load_histograms.cc',
        'renderer/page_load_histograms.h',
        'renderer/password_autocomplete_manager.cc',
        'renderer/password_autocomplete_manager.h',
        'renderer/pepper_devices.cc',
        'renderer/pepper_devices.h',
        'renderer/pepper_plugin_delegate_impl.cc',
        'renderer/pepper_plugin_delegate_impl.h',
        'renderer/pepper_scrollbar_widget.cc',
        'renderer/pepper_scrollbar_widget.h',
        'renderer/pepper_widget.cc',
        'renderer/pepper_widget.h',
        'renderer/plugin_channel_host.cc',
        'renderer/plugin_channel_host.h',
        'renderer/print_web_view_helper.cc',
        'renderer/print_web_view_helper.h',
        'renderer/print_web_view_helper_linux.cc',
        'renderer/print_web_view_helper_mac.mm',
        'renderer/print_web_view_helper_win.cc',
        'renderer/render_process.h',
        'renderer/render_process_impl.cc',
        'renderer/render_process_impl.h',
        'renderer/render_thread.cc',
        'renderer/render_thread.h',
        'renderer/render_view.cc',
        'renderer/render_view_linux.cc',
        'renderer/render_view.h',
        'renderer/render_widget.cc',
        'renderer/render_widget.h',
        'renderer/render_widget_fullscreen.cc',
        'renderer/render_widget_fullscreen.h',
        'renderer/render_widget_fullscreen_pepper.cc',
        'renderer/render_widget_fullscreen_pepper.h',
        'renderer/renderer_glue.cc',
        'renderer/renderer_histogram_snapshots.cc',
        'renderer/renderer_histogram_snapshots.h',
        'renderer/renderer_main.cc',
        'renderer/renderer_main_platform_delegate.h',
        'renderer/renderer_main_platform_delegate_linux.cc',
        'renderer/renderer_main_platform_delegate_mac.mm',
        'renderer/renderer_main_platform_delegate_win.cc',
        'renderer/renderer_sandbox_support_linux.cc',
        'renderer/renderer_sandbox_support_linux.h',
        'renderer/renderer_webapplicationcachehost_impl.cc',
        'renderer/renderer_webapplicationcachehost_impl.h',
        'renderer/renderer_webcookiejar_impl.cc',
        'renderer/renderer_webcookiejar_impl.h',
        'renderer/renderer_webidbcursor_impl.cc',
        'renderer/renderer_webidbcursor_impl.h',
        'renderer/renderer_webidbdatabase_impl.cc',
        'renderer/renderer_webidbdatabase_impl.h',
        'renderer/renderer_webidbfactory_impl.cc',
        'renderer/renderer_webidbfactory_impl.h',
        'renderer/renderer_webidbindex_impl.cc',
        'renderer/renderer_webidbindex_impl.h',
        'renderer/renderer_webidbobjectstore_impl.cc',
        'renderer/renderer_webidbobjectstore_impl.h',
        'renderer/renderer_webidbtransaction_impl.cc',
        'renderer/renderer_webidbtransaction_impl.h',
        'renderer/renderer_webkitclient_impl.cc',
        'renderer/renderer_webkitclient_impl.h',
        'renderer/renderer_webstoragearea_impl.cc',
        'renderer/renderer_webstoragearea_impl.h',
        'renderer/renderer_webstoragenamespace_impl.cc',
        'renderer/renderer_webstoragenamespace_impl.h',
        # TODO(noelutz): Find a better way to include these files
        '<(protoc_out_dir)/chrome/renderer/safe_browsing/client_model.pb.cc',
        '<(protoc_out_dir)/chrome/renderer/safe_browsing/client_model.pb.h',
        'renderer/safe_browsing/feature_extractor_clock.h',
        'renderer/safe_browsing/features.cc',
        'renderer/safe_browsing/features.h',
        'renderer/safe_browsing/phishing_classifier.cc',
        'renderer/safe_browsing/phishing_classifier.h',
        'renderer/safe_browsing/phishing_classifier_delegate.cc',
        'renderer/safe_browsing/phishing_classifier_delegate.h',
        'renderer/safe_browsing/phishing_dom_feature_extractor.cc',
        'renderer/safe_browsing/phishing_dom_feature_extractor.h',
        'renderer/safe_browsing/phishing_term_feature_extractor.cc',
        'renderer/safe_browsing/phishing_term_feature_extractor.h',
        'renderer/safe_browsing/phishing_thumbnailer.cc',
        'renderer/safe_browsing/phishing_thumbnailer.h',
        'renderer/safe_browsing/phishing_url_feature_extractor.cc',
        'renderer/safe_browsing/phishing_url_feature_extractor.h',
        'renderer/safe_browsing/scorer.cc',
        'renderer/safe_browsing/scorer.h',
        'renderer/search_extension.cc',
        'renderer/search_extension.h',
        'renderer/searchbox.cc',
        'renderer/searchbox.h',
        'renderer/searchbox_extension.cc',
        'renderer/searchbox_extension.h',
        'renderer/speech_input_dispatcher.cc',
        'renderer/speech_input_dispatcher.h',
        'renderer/spellchecker/spellcheck.cc',
        'renderer/spellchecker/spellcheck.h',
        'renderer/spellchecker/spellcheck_worditerator.cc',
        'renderer/spellchecker/spellcheck_worditerator.h',
        'renderer/translate_helper.cc',
        'renderer/translate_helper.h',
        'renderer/user_script_idle_scheduler.cc',
        'renderer/user_script_idle_scheduler.h',
        'renderer/user_script_slave.cc',
        'renderer/user_script_slave.h',
        'renderer/visitedlink_slave.cc',
        'renderer/visitedlink_slave.h',
        'renderer/webgraphicscontext3d_command_buffer_impl.cc',
        'renderer/webgraphicscontext3d_command_buffer_impl.h',
        'renderer/webplugin_delegate_proxy.cc',
        'renderer/webplugin_delegate_proxy.h',
        'renderer/webplugin_delegate_pepper.cc',
        'renderer/webplugin_delegate_pepper.h',
        'renderer/websharedworker_proxy.cc',
        'renderer/websharedworker_proxy.h',
        'renderer/websharedworkerrepository_impl.cc',
        'renderer/websharedworkerrepository_impl.h',
        'renderer/webworker_base.cc',
        'renderer/webworker_base.h',
        'renderer/webworker_proxy.cc',
        'renderer/webworker_proxy.h',
      ],
      'link_settings': {
        'mac_bundle_resources': [
          'renderer/renderer.sb',
        ],
      },
      'conditions': [
        ['disable_nacl!=1', {
          'dependencies': [
            'nacl',
          ],
          'sources': [
            'renderer/nacl_desc_wrapper_chrome.cc',
          ],
        }],
        # Linux-specific rules.
        ['OS=="linux"', {
          'conditions': [
            [ 'linux_use_tcmalloc==1', {
                'dependencies': [
                  '../base/allocator/allocator.gyp:allocator',
                ],
              },
            ],
          ],
          'dependencies': [
            '../build/linux/system.gyp:gtk',
            '../sandbox/sandbox.gyp:sandbox',
          ],
        }],
        # BSD-specific rules.
        ['OS=="openbsd" or OS=="freebsd"', {
          'dependencies': [
            '../build/linux/system.gyp:gtk',
          ],
        }],
        # Windows-specific rules.
        ['OS=="win"', {
          'include_dirs': [
            '<(DEPTH)/third_party/wtl/include',
          ],
          'conditions': [
            ['win_use_allocator_shim==1', {
              'dependencies': [
                '<(allocator_target)',
              ],
              'export_dependent_settings': [
                '<(allocator_target)',
              ],
            }],
          ],
        }],
        ['enable_gpu==1', {
          'dependencies': [
            '../gpu/gpu.gyp:gles2_c_lib',
          ],
          'sources': [
            'renderer/command_buffer_proxy.cc',
            'renderer/command_buffer_proxy.h',
          ],
        }],
        # We are migrating to client-based geolocation. Once the migration
        # has finished, ENABLE_CLIENT_BASED_GEOLOCATION will disappear.
        # See bugs:
        #     https://bugs.webkit.org/show_bug.cgi?id=45752 and
        #     http://code.google.com/p/chromium/issues/detail?id=59907
        ['"ENABLE_CLIENT_BASED_GEOLOCATION=1" in feature_defines', {
          'defines': [ 'ENABLE_CLIENT_BASED_GEOLOCATION=1' ]
        }],
      ],
    },
    {
      # Protobuf compiler / generator for the safebrowsing client model proto.
      'target_name': 'safe_browsing_proto',
      'type': 'none',
      'sources': [ 'renderer/safe_browsing/client_model.proto' ],
      'rules': [
        {
          'rule_name': 'genproto',
          'extension': 'proto',
          'inputs': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
          ],
          'variables': {
            # The protoc compiler requires a proto_path argument with the
            # directory containing the .proto file.
            # There's no generator variable that corresponds to this, so fake it.
            'rule_input_relpath': 'renderer/safe_browsing',
          },
          'outputs': [
            '<(protoc_out_dir)/chrome/<(rule_input_relpath)/<(RULE_INPUT_ROOT).pb.h',
            '<(protoc_out_dir)/chrome/<(rule_input_relpath)/<(RULE_INPUT_ROOT).pb.cc',
          ],
          'action': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '--proto_path=./<(rule_input_relpath)',
            './<(rule_input_relpath)/<(RULE_INPUT_ROOT)<(RULE_INPUT_EXT)',
            '--cpp_out=<(protoc_out_dir)/chrome/<(rule_input_relpath)',
          ],
          'message': 'Generating C++ code from <(RULE_INPUT_PATH)',
        },
      ],
      'dependencies': [
        '../third_party/protobuf/protobuf.gyp:protobuf_lite',
        '../third_party/protobuf/protobuf.gyp:protoc#host',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(protoc_out_dir)',
        ]
      },
      'export_dependent_settings': [
        '../third_party/protobuf/protobuf.gyp:protobuf_lite',
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
