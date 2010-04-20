#!/usr/bin/env python

'''
/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
'''


import sys

from u_format_parse import *


def layout_map(layout):
    return 'UTIL_FORMAT_LAYOUT_' + str(layout).upper()


def colorspace_map(colorspace):
    return 'UTIL_FORMAT_COLORSPACE_' + str(colorspace).upper()


colorspace_channels_map = {
    'rgb': 'rgba',
    'rgba': 'rgba',
    'zs': 'zs',
    'yuv': ['y1', 'y2', 'u', 'v'],
    'dxt': []
}


kind_map = {
    VOID:     "UTIL_FORMAT_TYPE_VOID",
    UNSIGNED: "UTIL_FORMAT_TYPE_UNSIGNED",
    SIGNED:   "UTIL_FORMAT_TYPE_SIGNED",
    FIXED:    "UTIL_FORMAT_TYPE_FIXED",
    FLOAT:    "UTIL_FORMAT_TYPE_FLOAT",
}


def bool_map(value):
    if value:
        return "TRUE"
    else:
        return "FALSE"


swizzle_map = {
    SWIZZLE_X:    "UTIL_FORMAT_SWIZZLE_X",
    SWIZZLE_Y:    "UTIL_FORMAT_SWIZZLE_Y",
    SWIZZLE_Z:    "UTIL_FORMAT_SWIZZLE_Z",
    SWIZZLE_W:    "UTIL_FORMAT_SWIZZLE_W",
    SWIZZLE_0:    "UTIL_FORMAT_SWIZZLE_0",
    SWIZZLE_1:    "UTIL_FORMAT_SWIZZLE_1",
    SWIZZLE_NONE: "UTIL_FORMAT_SWIZZLE_NONE",
}


def write_format_table(formats):
    print '/* This file is autogenerated by u_format_table.py from u_format.csv. Do not edit directly. */'
    print
    # This will print the copyright message on the top of this file
    print __doc__.strip()
    print
    print '#include "u_format.h"'
    print
    print 'const struct util_format_description'
    print 'util_format_description_table[] = '
    print "{"
    for format in formats:
        print "   {"
        print "      %s," % (format.name,)
        print "      \"%s\"," % (format.name,)
        print "      {%u, %u, %u}, /* block */" % (format.block_width, format.block_height, format.block_size())
        print "      %s," % (layout_map(format.layout),)
        print "      {"
        for i in range(4):
            type = format.in_types[i]
            if i < 3:
                sep = ","
            else:
                sep = ""
            print "         {%s, %s, %u}%s /* %s */" % (kind_map[type.kind], bool_map(type.norm), type.size, sep, "xyzw"[i])
        print "      },"
        print "      {"
        for i in range(4):
            swizzle = format.out_swizzle[i]
            if i < 3:
                sep = ","
            else:
                sep = ""
            try:
                comment = layout_channels_map[format.layout][i]
            except:
                comment = 'ignored'
            print "         %s%s /* %s */" % (swizzle_map[swizzle], sep, comment)
        print "      },"
        print "      %s," % (colorspace_map(format.colorspace),)
        print "   },"
    print "   {"
    print "      PIPE_FORMAT_NONE,"
    print "      \"PIPE_FORMAT_NONE\","
    print "      {0, 0, 0},"
    print "      0,"
    print "      {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},"
    print "      {0, 0, 0, 0},"
    print "      0"
    print "   },"
    print "};"


def main():

    formats = []
    for arg in sys.argv[1:]:
        formats.extend(parse(arg))
    write_format_table(formats)


if __name__ == '__main__':
    main()
