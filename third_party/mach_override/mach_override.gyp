# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
  ],
  'conditions': [
    ['OS=="mac"', {
      'targets' : [
        {
          'target_name' : 'mach_override',
          'type': '<(library)',
          'sources': [
            'mach_override.c',
            'mach_override.h',
          ],
        },
      ],
    }],
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
