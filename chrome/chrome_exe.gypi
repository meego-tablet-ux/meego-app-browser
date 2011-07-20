# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'target_defaults': {
    'variables': {
      'chrome_exe_target': 0,
      'qml_plugin_target': 0,
      'use_system_xdg_utils': 0,
    },
    'target_conditions': [
      ['qml_plugin_target==1', {
        'defines': [
          'BUILD_QML_PLUGIN=1',
        ],
        'sources': [
          # .cc, .h, and .mm files under app that are used on all
          # platforms, including both 32-bit and 64-bit Windows.
          # Test files are not included.
          'app/chrome_lib_main_qt.cc',
          'app/chrome_lib_main_qt.h',
          'app/scoped_ole_initializer.h',
          # TODO(bradnelson): once automatic generation of 64 bit targets on
          # Windows is ready, take this out and add a dependency on
          # content_common.gypi.
          '../content/common/content_switches.cc',
          '../content/common/content_switches.h',
          'app/browser_object_plugin.cc',
        ],
        'conditions': [
          ['OS!="win"', {
              'source!': ['app/client_util.cc',]
            },
          ],
        ],
       },
      ],
      ['chrome_exe_target==1', {
        'sources': [
          # .cc, .h, and .mm files under app that are used on all
          # platforms, including both 32-bit and 64-bit Windows.
          # Test files are not included.
          'app/breakpad_win.cc',
          'app/breakpad_win.h',
          'app/chrome_exe_main_gtk.cc',
          'app/chrome_exe_main_mac.mm',
          'app/chrome_lib_main_qt.cc',
          'app/chrome_lib_main_qt.h',
          'app/chrome_exe_main_win.cc',
          'app/chrome_exe_main_qt.cc',
          'app/chrome_exe_resource.h',
          'app/client_util.cc',
          'app/client_util.h',
          'app/hard_error_handler_win.cc',
          'app/hard_error_handler_win.h',
          'app/scoped_ole_initializer.h',
          # TODO(bradnelson): once automatic generation of 64 bit targets on
          # Windows is ready, take this out and add a dependency on
          # content_common.gypi.
          '../content/common/content_switches.cc',
          '../content/common/content_switches.h',
        ],
        'mac_bundle_resources': [
          'app/app-Info.plist',
        ],
        # TODO(mark): Come up with a fancier way to do this.  It should only
        # be necessary to list app-Info.plist once, not the three times it is
        # listed here.
        'mac_bundle_resources!': [
          'app/app-Info.plist',
        ],
        'xcode_settings': {
          'CHROMIUM_STRIP_SAVE_FILE': 'app/app.saves',
          'INFOPLIST_FILE': 'app/app-Info.plist',
        },
        'conditions': [
          ['OS=="win"', {
            'sources': [
              'app/chrome_exe.rc',
              'app/chrome_exe_version.rc.version',
            ],
            'include_dirs': [
              '<(SHARED_INTERMEDIATE_DIR)/chrome',
            ],
            # TODO(scottbyer): This is a temporary workaround.  The right fix
            # is to change the output file to be in $(IntDir) for this project
            # and the .dll project and use the hardlink script to link it back
            # to $(OutDir).
            'configurations': {
              'Debug_Base': {
                'msvs_settings': {
                  'VCLinkerTool': {
                    'LinkIncremental': '1',
                  },
                },
              },
            },
            'msvs_settings': {
              'VCLinkerTool': {
                'DelayLoadDLLs': [
                  'dbghelp.dll',
                  'dwmapi.dll',
                  'uxtheme.dll',
                  'ole32.dll',
                  'oleaut32.dll',
                ],
                # Set /SUBSYSTEM:WINDOWS for chrome.exe itself.
                'SubSystem': '2',
              },
              'VCManifestTool': {
                'AdditionalManifestFiles': '$(ProjectDir)\\app\\chrome.exe.manifest',
              },
            },
            'actions': [
              {
                'action_name': 'version',
                'variables': {
                  'template_input_path': 'app/chrome_exe_version.rc.version',
                },
                'conditions': [
                  [ 'branding == "Chrome"', {
                    'variables': {
                       'branding_path': 'app/theme/google_chrome/BRANDING',
                    },
                  }, { # else branding!="Chrome"
                    'variables': {
                       'branding_path': 'app/theme/chromium/BRANDING',
                    },
                  }],
                ],
                'inputs': [
                  '<(template_input_path)',
                  '<(version_path)',
                  '<(branding_path)',
                ],
                'outputs': [
                  '<(SHARED_INTERMEDIATE_DIR)/chrome/chrome_exe_version.rc',
                ],
                'action': [
                  'python',
                  '<(version_py_path)',
                  '-f', '<(version_path)',
                  '-f', '<(branding_path)',
                  '<(template_input_path)',
                  '<@(_outputs)',
                ],
                'process_outputs_as_sources': 1,
                'message': 'Generating version information in <(_outputs)'
              },
              {
                'action_name': 'first_run',
                'inputs': [
                    'app/FirstRun',
                ],
                'outputs': [
                    '<(PRODUCT_DIR)/First Run',
                ],
                'action': ['cp', '-f', '<@(_inputs)', '<@(_outputs)'],
                'message': 'Copy first run complete sentinel file',
              },
            ],
          }, {  # 'OS!="win"
            'sources!': [
              'app/client_util.cc',
            ]
          }],
        ],
      }],
    ],
  },

  'targets': [
    {
      'target_name': 'qmlplugin',
      'type': 'executable',
      'product_name': 'meego-browser-wrapper',
      'variables': {
        'use_system_xdg_utils%': 0
      },
      'defines': [
        'BUILD_QML_PLUGIN=1',
       ],
      'sources': [
         'app/chrome_exe_main_qt.cc',
      ],
      'dependencies': [
        'BrowserWrapper',
        'BrowserPlugin',
        'chrome_misc',
      ],
      #'copies': [
      #  {
      #    'destination': '<(PRODUCT_DIR)/MeeGo/App/Browser',
      #    'files': [
      #      'app/qmldir',
      #    ],
      #  },
      #],
     'actions': [
      {
        'action_name': 'mkdir',
        'inputs': [],
        'outputs': ['<(PRODUCT_DIR)/MeeGo/App/Browser',],
        'action': [
          'mkdir',
          '-p',
          '<@(_outputs)',
        ],
      },
      {
        'action_name': 'mkdir2',
        'inputs': [],
        'outputs': ['<(PRODUCT_DIR)/MeeGo/App/BrowserWrapper',],
        'action': [
          'mkdir',
          '-p',
          '<@(_outputs)',
        ],
      },
      {
        'action_name': 'softlink',
        'inputs': [ '<(PRODUCT_DIR)/lib.target/libBrowserPlugin.so', ],
        'outputs': [ '<(PRODUCT_DIR)/MeeGo/App/Browser/libBrowserPlugin.so',  ],
        'action': [
          'ln',
          '-sf',
         '<@(_inputs)',
         '<@(_outputs)'
        ],
      },
      {
        'action_name': 'softlink2',
        'inputs': [ '<(PRODUCT_DIR)/lib.target/libBrowserWrapper.so', ],
        'outputs': [ '<(PRODUCT_DIR)/MeeGo/App/BrowserWrapper/libBrowserWrapper.so',  ],
        'action': [
          'ln',
          '-sf',
         '<@(_inputs)',
         '<@(_outputs)'
        ],
      },
      {
        'action_name': 'copy',
        'inputs': [ 'app/qmldir.browser',],
        'outputs': ['<(PRODUCT_DIR)/MeeGo/App/Browser/qmldir',],
        'action': [
          'cp',
          '-f',
          '<@(_inputs)',
          '<@(_outputs)',
        ],
      },
      {
        'action_name': 'copy2',
        'inputs': [ 'app/qmldir.wrapper',],
        'outputs': ['<(PRODUCT_DIR)/MeeGo/App/BrowserWrapper/qmldir',],
        'action': [
          'cp',
          '-f',
          '<@(_inputs)',
          '<@(_outputs)',
        ],
      },
      ],
    },

    {
      'target_name': 'BrowserWrapper',
      'type': 'shared_library',
      'product_name': 'BrowserWrapper',
      'sources': [
         'app/browser_wrapper_plugin.cc',
         'browser/browser_wrapper_qt.h',
         'browser/browser_wrapper_qt.cc',
       ],
      'dependencies': [
         '../build/linux/system.gyp:meegotouch',
      ],
      'actions':[
        {
         'action_name': 'moc_browser_wrapper_qt.h',
         'inputs': [
           'browser/browser_wrapper_qt.h',
         ],
         'outputs': [
           'browser/moc_browser_wrapper_qt.cc',
         ],
         'action': [
           'moc',
         '<(_inputs)',
         '-o',
         '<(_outputs)',
         ],
        },
        {
         'action_name': 'moc_browser_wrapper_plugin.cc',
         'inputs': [
           'app/browser_wrapper_plugin.cc',
         ],
         'outputs': [
           'app/moc_browser_wrapper_plugin.cc',
         ],
         'action': [
           'moc',
           '<(_inputs)',
           '-o',
           '<(_outputs)',
         ],
       },
      ], 
    },

    {
      'target_name': 'chrome_misc',
      'type': 'none',
      'conditions': [
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
          'actions': [
          {
            'action_name': 'manpage',
            'conditions': [
              [ 'branding == "Chrome"', {
                'variables': {
                  'name': 'Google Chrome',
                  'filename': 'google-chrome',
                  'confdir': 'google-chrome',
                },
              }, { # else branding!="Chrome"
                'variables': {
                  'name': 'Chromium',
                  'filename': 'chromium-browser',
                  'confdir': 'chromium',
                },
              }],
              ],
              'inputs': [
                'tools/build/linux/sed.sh',
              'app/resources/manpage.1.in',
              ],
              'outputs': [
                '<(PRODUCT_DIR)/chrome.1',
              ],
              'action': [
                'tools/build/linux/sed.sh',
              'app/resources/manpage.1.in',
              '<@(_outputs)',
              '-e', 's/@@NAME@@/<(name)/',
              '-e', 's/@@FILENAME@@/<(filename)/',
              '-e', 's/@@CONFDIR@@/<(confdir)/',
              ],
              'message': 'Generating manpage'
          },
          ],
          'copies': [
          {
            'destination': '<(PRODUCT_DIR)',
            'files': [
            '../third_party/xdg-utils/scripts/xdg-mime',
            '../third_party/xdg-utils/scripts/xdg-settings',
            ],
            'conditions': [
              ['branding=="Chrome"', {
                'files': ['app/theme/google_chrome/product_logo_48.png']
              }, { # else: 'branding!="Chrome"
                'files': ['app/theme/chromium/product_logo_48.png']
              }],
              ['buildqmlplugin==1', {
                'files': ['tools/build/linux/qml-launcher-wrapper']
              }, {
                'files': ['tools/build/linux/chrome-wrapper']
              }],
            ],
          },
          ],
        }],
      ]
    },
    {
      'target_name': 'chrome',
      'type': 'executable',
      'mac_bundle': 1,
      'msvs_guid': '7B219FAA-E360-43C8-B341-804A94EEFFAC',
      'variables': {
        'chrome_exe_target': 1,
        'use_system_xdg_utils%': 0,
      },
      'conditions': [
       ['meegotouch==1', {
            'sources/': [
              ['exclude', '_(chromeos|gtk)(_unittest)?\\.cc$'],
              ['exclude', '/gtk/'],
              ['exclude', '/(gtk)_[^/]*\\.cc$'],
            ],
        }],
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
          'conditions': [
            [ 'linux_use_tcmalloc==1', {
                'dependencies': [
                  '<(allocator_target)',
                ],
              },
            ],
            ['disable_nacl!=1', {
               'dependencies': [
                 'nacl',
               ],
            }],
           ],
           'dependencies': [
            '../build/linux/system.gyp:meegotouch',
            # On Linux, link the dependencies (libraries) that make up actual
            # Chromium functionality directly into the executable.
            '<@(chromium_dependencies)',
            # Needed for chrome_main.cc initialization of libraries.
            '../build/linux/system.gyp:dbus-glib',
            '../build/linux/system.gyp:gtk',
            'packed_resources',
            # Needed to use the master_preferences functions
            'installer_util',
          ],
          'sources': [
            'app/chrome_dll_resource.h',
            'app/chrome_main.cc',
            'app/chrome_main_posix.cc',
          ],
        }],
        ['OS=="mac"', {
          # 'branding' is a variable defined in common.gypi
          # (e.g. "Chromium", "Chrome")
          'conditions': [
            ['branding=="Chrome"', {
              'mac_bundle_resources': [
                'app/theme/google_chrome/app.icns',
                'app/theme/google_chrome/document.icns',
                'browser/ui/cocoa/applescript/scripting.sdef',
              ],
            }, {  # else: 'branding!="Chrome"
              'mac_bundle_resources': [
                'app/theme/chromium/app.icns',
                'app/theme/chromium/document.icns',
                'browser/ui/cocoa/applescript/scripting.sdef',
              ],
            }],
            ['mac_breakpad==1', {
              'variables': {
                # A real .dSYM is needed for dump_syms to operate on.
                'mac_real_dsym': 1,
              },
              'dependencies': [
                '../breakpad/breakpad.gyp:dump_syms',
                '../breakpad/breakpad.gyp:symupload',
              ],
              # The "Dump Symbols" post-build step is in a target_conditions
              # block so that it will follow the "Strip If Needed" step if that
              # is also being used.  There is no standard configuration where
              # both of these steps occur together, but Mark likes to use this
              # configuraiton sometimes when testing Breakpad-enabled builds
              # without the time overhead of creating real .dSYM files.  When
              # both "Dump Symbols" and "Strip If Needed" are present, "Dump
              # Symbols" must come second because "Strip If Needed" creates
              # a fake .dSYM that dump_syms needs to fake dump.  Since
              # "Strip If Needed" is added in a target_conditions block in
              # common.gypi, "Dump Symbols" needs to be in an (always true)
              # target_conditions block.
              'target_conditions': [
                ['1 == 1', {
                  'postbuilds': [
                    {
                      'postbuild_name': 'Dump Symbols',
                      'variables': {
                        'dump_product_syms_path':
                            'tools/build/mac/dump_product_syms',
                      },
                      'action': ['<(dump_product_syms_path)',
                                 '<(branding)'],
                    },
                  ],
                }],
              ],
            }],  # mac_breakpad
          ],
          'product_name': '<(mac_product_name)',
          'xcode_settings': {
            # chrome/app/app-Info.plist has:
            #   CFBundleIdentifier of CHROMIUM_BUNDLE_ID
            #   CFBundleName of CHROMIUM_SHORT_NAME
            #   CFBundleSignature of CHROMIUM_CREATOR
            # Xcode then replaces these values with the branded values we set
            # as settings on the target.
            'CHROMIUM_BUNDLE_ID': '<(mac_bundle_id)',
            'CHROMIUM_CREATOR': '<(mac_creator)',
            'CHROMIUM_SHORT_NAME': '<(branding)',
          },
          'dependencies': [
            'helper_app',
            'infoplist_strings_tool',
            'chrome_manifest_bundle',
          ],
          'mac_bundle_resources': [
            '<(PRODUCT_DIR)/<(mac_bundle_id).manifest',
          ],
          'actions': [
            {
              # Generate the InfoPlist.strings file
              'action_name': 'Generate InfoPlist.strings files',
              'variables': {
                'tool_path': '<(PRODUCT_DIR)/infoplist_strings_tool',
                # Unique dir to write to so the [lang].lproj/InfoPlist.strings
                # for the main app and the helper app don't name collide.
                'output_path': '<(INTERMEDIATE_DIR)/app_infoplist_strings',
              },
              'conditions': [
                [ 'branding == "Chrome"', {
                  'variables': {
                     'branding_name': 'google_chrome_strings',
                  },
                }, { # else branding!="Chrome"
                  'variables': {
                     'branding_name': 'chromium_strings',
                  },
                }],
              ],
              'inputs': [
                '<(tool_path)',
                '<(version_path)',
                # TODO: remove this helper when we have loops in GYP
                '>!@(<(apply_locales_cmd) \'<(grit_out_dir)/<(branding_name)_ZZLOCALE.pak\' <(locales))',
              ],
              'outputs': [
                # TODO: remove this helper when we have loops in GYP
                '>!@(<(apply_locales_cmd) -d \'<(output_path)/ZZLOCALE.lproj/InfoPlist.strings\' <(locales))',
              ],
              'action': [
                '<(tool_path)',
                '-b', '<(branding_name)',
                '-v', '<(version_path)',
                '-g', '<(grit_out_dir)',
                '-o', '<(output_path)',
                '-t', 'main',
                '<@(locales)',
              ],
              'message': 'Generating the language InfoPlist.strings files',
              'process_outputs_as_mac_bundle_resources': 1,
            },
          ],
          'copies': [
            {
              'destination': '<(PRODUCT_DIR)/<(mac_product_name).app/Contents/Versions/<(version_full)',
              'files': [
                '<(PRODUCT_DIR)/<(mac_product_name) Helper.app',
              ],
            },
          ],
          'postbuilds': [
            {
              'postbuild_name': 'Copy <(mac_product_name) Framework.framework',
              'action': [
                'tools/build/mac/copy_framework_unversioned',
                '${BUILT_PRODUCTS_DIR}/<(mac_product_name) Framework.framework',
                '${BUILT_PRODUCTS_DIR}/${CONTENTS_FOLDER_PATH}/Versions/<(version_full)',
              ],
            },
            {
              # Modify the Info.plist as needed.  The script explains why this
              # is needed.  This is also done in the helper_app and chrome_dll
              # targets.  Use -b0 to not include any Breakpad information; that
              # all goes into the framework's Info.plist.  Keystone information
              # is included if Keystone is enabled.  The application reads
              # Keystone keys from this plist and not the framework's, and
              # the ticket will reference this Info.plist to determine the tag
              # of the installed product.  Use -s1 to include Subversion
              # information.
              'postbuild_name': 'Tweak Info.plist',
              'action': ['<(tweak_info_plist_path)',
                         '-b0',
                         '-k<(mac_keystone)',
                         '-s1',
                         '<(branding)',
                         '<(mac_bundle_id)'],
            },
            {
              'postbuild_name': 'Clean up old versions',
              'action': [
                'tools/build/mac/clean_up_old_versions',
                '<(version_full)'
              ],
            },
          ],  # postbuilds
        }],
        ['OS=="linux"', {
          'conditions': [
            ['branding=="Chrome"', {
              'dependencies': [
                'linux_installer_configs',
              ],
            }],
            ['selinux==0', {
              'dependencies': [
                '../sandbox/sandbox.gyp:sandbox',
              ],
            }],
          ],
        }],
        ['OS != "mac"', {
          'conditions': [
            ['branding=="Chrome"', {
              'product_name': 'chrome'
            }, {  # else: Branding!="Chrome"
              # TODO:  change to:
              #   'product_name': 'chromium'
              # whenever we convert the rest of the infrastructure
              # (buildbots etc.) to use "gyp -Dbranding=Chrome".
              # NOTE: chrome/app/theme/chromium/BRANDING and
              # chrome/app/theme/google_chrome/BRANDING have the short names,
              # etc.; should we try to extract from there instead?
              # 'product_name': 'chrome'
              'product_name': 'chrome'
            }],
            # On Mac, this is done in chrome_dll.gypi.
            ['internal_pdf', {
              'dependencies': [
                '../pdf/pdf.gyp:pdf',
              ],
            }],
          ],
          'dependencies': [
            'chrome_misc',
            'packed_extra_resources',
            # Copy Flash Player files to PRODUCT_DIR if applicable. Let the .gyp
            # file decide what to do on a per-OS basis; on Mac, internal plugins
            # go inside the framework, so this dependency is in chrome_dll.gypi.
            '../third_party/adobe/flash/flash_player.gyp:flash_player',
          ],
        }],
        ['OS=="mac" or OS=="win"', {
          'dependencies': [
            # On Windows and Mac, make sure we've built chrome_dll, which
            # contains all of the library code with Chromium functionality.
            'chrome_dll',
          ],
        }],
        ['OS=="win"', {
          'dependencies': [
            'installer_util',
            'installer_util_strings',
            '../breakpad/breakpad.gyp:breakpad_handler',
            '../breakpad/breakpad.gyp:breakpad_sender',
            '../sandbox/sandbox.gyp:sandbox',
            'app/locales/locales.gyp:*',
            'app/policy/cloud_policy_codegen.gyp:policy',
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              'ImportLibrary': '$(OutDir)\\lib\\chrome_exe.lib',
              'ProgramDatabaseFile': '$(OutDir)\\chrome_exe.pdb',
            },
          },
        }],
      ],
    },
    
    {
      'target_name': 'BrowserPlugin',
      'type': 'shared_library',
      'variables': {
        'qml_plugin_target': 1,
      },
      'defines': [
         'BUILD_QML_PLUGIN=1',
      ],
      'product_name': 'BrowserPlugin',
      'actions': [
         {
          'action_name': 'moc_browser_object_plugin.cc',
          'inputs': [
            'app/browser_object_plugin.cc',
          ],
          'outputs': [
            'app/moc_browser_object_plugin.cc',
          ],
          'action': [
            'moc',
            '<(_inputs)',
            '-o',
            '<(_outputs)',
          ],
        },
       ],
      'conditions': [
       ['meegotouch==1', {
            'sources/': [
              ['exclude', '_(chromeos|gtk)(_unittest)?\\.cc$'],
              ['exclude', '/gtk/'],
              ['exclude', '/(gtk)_[^/]*\\.cc$'],
            ],
        }],
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
          'conditions': [
            [ 'linux_use_tcmalloc==1', {
                'dependencies': [
                  '<(allocator_target)',
                ],
              },
            ],
            ['disable_nacl!=1', {
               'dependencies': [
                 'nacl',
               ],
            }],
          ],
          'dependencies': [
            '../build/linux/system.gyp:meegotouch',
            # On Linux, link the dependencies (libraries) that make up actual
            # Chromium functionality directly into the executable.
            '<@(chromium_dependencies)',
            # Needed for chrome_main.cc initialization of libraries.
            '../build/linux/system.gyp:dbus-glib',
            '../build/linux/system.gyp:gtk',
            'packed_resources',
            # Needed to use the master_preferences functions
            'installer_util',
          ],
          'sources': [
            'app/chrome_dll_resource.h',
            'app/chrome_main.cc',
            'app/chrome_main_posix.cc',
          ],
        }],
        ['OS=="linux"', {
          'conditions': [
            ['selinux==0', {
              'dependencies': [
                '../sandbox/sandbox.gyp:sandbox',
              ],
            }],
          ],
        }],
        ['OS != "mac"', {
          'dependencies': [
            'packed_extra_resources',
            # Copy Flash Player files to PRODUCT_DIR if applicable. Let the .gyp
            # file decide what to do on a per-OS basis; on Mac, internal plugins
            # go inside the framework, so this dependency is in chrome_dll.gypi.
            '../third_party/adobe/flash/flash_player.gyp:flash_player',
          ],
        }],
      ],
    },

    {
      'target_name': 'chrome_mesa',
      'type': 'none',
      'dependencies': [
        'chrome',
        '../third_party/mesa/mesa.gyp:osmesa',
      ],
      'conditions': [
        ['OS=="mac"', {
          'copies': [{
            'destination': '<(PRODUCT_DIR)/<(mac_product_name).app/Contents/Versions/<(version_full)/<(mac_product_name) Helper.app/Contents/MacOS/',
            'files': ['<(PRODUCT_DIR)/osmesa.so'],
          }],
        }],
      ],
    },
  ],
  'conditions': [
    ['OS=="win"', {
      'targets': [
        {
          'target_name': 'chrome_nacl_win64',
          'type': 'executable',
          'product_name': 'nacl64',
          'msvs_guid': 'BB1AE956-038B-4092-96A2-951D2B418548',
          'variables': {
            'chrome_exe_target': 1,
          },
          'dependencies': [
            # On Windows make sure we've built Win64 version of chrome_dll,
            # which contains all of the library code with Chromium
            # functionality.
            'chrome_dll_nacl_win64',
            'common_constants_win64',
            'installer_util_nacl_win64',
            'app/policy/cloud_policy_codegen.gyp:policy_win64',
            '../breakpad/breakpad.gyp:breakpad_handler_win64',
            '../breakpad/breakpad.gyp:breakpad_sender_win64',
            '../base/base.gyp:base_nacl_win64',
            '../sandbox/sandbox.gyp:sandbox_win64',
          ],
          'defines': [
            '<@(nacl_win64_defines)',
          ],
          'include_dirs': [
            '<(SHARED_INTERMEDIATE_DIR)/chrome',
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              'ImportLibrary': '$(OutDir)\\lib\\nacl64_exe.lib',
              'ProgramDatabaseFile': '$(OutDir)\\nacl64_exe.pdb',
            },
          },
          'configurations': {
            'Common_Base': {
              'msvs_target_platform': 'x64',
            },
          },
        },
      ],
    }],
  ],
}
