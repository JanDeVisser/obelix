#!/usr/bin/python

#  Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
#
#  SPDX-License-Identifier: GPL-3.0-or-later

import re

r = re.compile("(0x[0-9a-f]{2},?){8}")
zeros = "0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00"
dither = "0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55"

filemap = {
    32: "space",
    33: "bang",
    34: "dquote",
    35: "hash",
    36: "dollar",
    37: "percent",
    38: "amp",
    39: "squote",
    40: "lparen",
    41: "rparen",
    42: "star",
    43: "plus",
    44: "comma",
    45: "dash",
    46: "dot",
    47: "slash",
    58: "colon",
    59: "semicolon",
    60: "lt",
    61: "eq",
    62: "gt",
    63: "qmark",
    64: "at",
    91: "lbracket",
    92: "bslash",
    93: "rbracket",
    94: "hat",
    95: "uscore",
    96: "bquote",
    123: "lbrace",
    124: "pipe",
    125: "rbrace",
    126: "tilde"
}


def readchar(ch):
    try:
        with open((filemap[ch] if ix in filemap else chr(ch)) + ".xbm") as fh:
            for line in fh.readlines():
                m = r.search(line)
                if m:
                    return m.group(0)
            return dither
    except:
        return zeros


print("static const uchar bitmaps[128][8] = {")

for ix in range(0, 128):
    bytes_ = readchar(ix)

    print("  {{       /* 0x{0:02x} {0:3d} */".format(ix))
    for b in bytes_.split(","):
        bval = int(b, 16)
        bitmap = ""
        bit = 128
        while bit != 0:
            bitmap = ("#" if bval & bit else " ") + bitmap
            bit = bit >> 1
        print("    {0}, /* {1} */".format(b, bitmap))
    print("  },")
print("};");
