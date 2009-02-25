/*
 *  mksp.c
 *
 *  Generate SP tables for DES-150 library
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the DES-150 library.
 *
 * The Initial Developer of the Original Code is
 * Nelson B. Bolyard, nelsonb@iname.com.
 * Portions created by the Initial Developer are Copyright (C) 1990
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <stdio.h>

/*
 * sboxes - the tables for the s-box functions
 *        from FIPS 46, pages 15-16.
 */
unsigned char S[8][64] = {
/* Func S1 = */ {
	14,  0,  4, 15, 13,  7,  1,  4,  2, 14, 15,  2, 11, 13,  8,  1,
	 3, 10, 10,  6,  6, 12, 12, 11,  5,  9,  9,  5,  0,  3,  7,  8,
	 4, 15,  1, 12, 14,  8,  8,  2, 13,  4,  6,  9,  2,  1, 11,  7,
	15,  5, 12, 11,  9,  3,  7, 14,  3, 10, 10,  0,  5,  6,  0, 13
    },
/* Func S2 = */ {
	15,  3,  1, 13,  8,  4, 14,  7,  6, 15, 11,  2,  3,  8,  4, 14,
	 9, 12,  7,  0,  2,  1, 13, 10, 12,  6,  0,  9,  5, 11, 10,  5,
	 0, 13, 14,  8,  7, 10, 11,  1, 10,  3,  4, 15, 13,  4,  1,  2,
	 5, 11,  8,  6, 12,  7,  6, 12,  9,  0,  3,  5,  2, 14, 15,  9
    },
/* Func S3 = */ {
	10, 13,  0,  7,  9,  0, 14,  9,  6,  3,  3,  4, 15,  6,  5, 10,
	 1,  2, 13,  8, 12,  5,  7, 14, 11, 12,  4, 11,  2, 15,  8,  1,
	13,  1,  6, 10,  4, 13,  9,  0,  8,  6, 15,  9,  3,  8,  0,  7,
	11,  4,  1, 15,  2, 14, 12,  3,  5, 11, 10,  5, 14,  2,  7, 12
    },
/* Func S4 = */ {
	 7, 13, 13,  8, 14, 11,  3,  5,  0,  6,  6, 15,  9,  0, 10,  3,
	 1,  4,  2,  7,  8,  2,  5, 12, 11,  1, 12, 10,  4, 14, 15,  9,
	10,  3,  6, 15,  9,  0,  0,  6, 12, 10, 11,  1,  7, 13, 13,  8,
	15,  9,  1,  4,  3,  5, 14, 11,  5, 12,  2,  7,  8,  2,  4, 14
    },
/* Func S5 = */ {
	 2, 14, 12, 11,  4,  2,  1, 12,  7,  4, 10,  7, 11, 13,  6,  1,
	 8,  5,  5,  0,  3, 15, 15, 10, 13,  3,  0,  9, 14,  8,  9,  6,
	 4, 11,  2,  8,  1, 12, 11,  7, 10,  1, 13, 14,  7,  2,  8, 13,
	15,  6,  9, 15, 12,  0,  5,  9,  6, 10,  3,  4,  0,  5, 14,  3
    },
/* Func S6 = */ {
	12, 10,  1, 15, 10,  4, 15,  2,  9,  7,  2, 12,  6,  9,  8,  5,
	 0,  6, 13,  1,  3, 13,  4, 14, 14,  0,  7, 11,  5,  3, 11,  8,
	 9,  4, 14,  3, 15,  2,  5, 12,  2,  9,  8,  5, 12, 15,  3, 10,
	 7, 11,  0, 14,  4,  1, 10,  7,  1,  6, 13,  0, 11,  8,  6, 13
    },
/* Func S7 = */ {
	 4, 13, 11,  0,  2, 11, 14,  7, 15,  4,  0,  9,  8,  1, 13, 10,
	 3, 14, 12,  3,  9,  5,  7, 12,  5,  2, 10, 15,  6,  8,  1,  6,
	 1,  6,  4, 11, 11, 13, 13,  8, 12,  1,  3,  4,  7, 10, 14,  7,
	10,  9, 15,  5,  6,  0,  8, 15,  0, 14,  5,  2,  9,  3,  2, 12
    },
/* Func S8 = */ {
	13,  1,  2, 15,  8, 13,  4,  8,  6, 10, 15,  3, 11,  7,  1,  4,
	10, 12,  9,  5,  3,  6, 14, 11,  5,  0,  0, 14, 12,  9,  7,  2,
	 7,  2, 11,  1,  4, 14,  1,  7,  9,  4, 12, 10, 14,  8,  2, 13,
	 0, 15,  6, 12, 10,  9, 13,  0, 15,  3,  3,  5,  5,  6,  8, 11
    }
};

/*
 * Permutation function for results from s-boxes
 *   from FIPS 46 pages 12 and 16.
 * P = 
 */
unsigned char P[32] = {
	16,   7,  20,  21,  29,  12,  28,  17,
	 1,  15,  23,  26,   5,  18,  31,  10,
	 2,   8,  24,  14,  32,  27,   3,   9,
	19,  13,  30,   6,  22,  11,   4,  25
};

unsigned int Pinv[32];
unsigned int SP[8][64];

void
makePinv(void)
{
    int i;
    unsigned int Pi = 0x80000000;
    for (i = 0; i < 32; ++i) {
    	int j = 32 - P[i];
	Pinv[j] = Pi;
	Pi >>= 1;
    }
}

void
makeSP(void)
{
    int box;
    for (box = 0; box < 8; ++box) {
	int item;
	printf("/* box S%d */ {\n", box + 1);
    	for (item = 0; item < 64; ++item ) {
	    unsigned int s = S[box][item];
	    unsigned int val = 0;
	    unsigned int bitnum = (7-box) * 4;
	    for (; s; s >>= 1, ++bitnum) {
		if (s & 1) {
		    val |= Pinv[bitnum];
		}
	    }
	    val = (val << 3) | (val >> 29);
	    SP[box][item] = val;
	}
	for (item = 0; item < 64; item += 4) {
	    printf("\t0x%08x, 0x%08x, 0x%08x, 0x%08x,\n",
	    SP[box][item], SP[box][item+1], SP[box][item+2], SP[box][item+3]);
	}
	printf("    },\n");
    }
}

int
main()
{
    makePinv();
    makeSP();
    return 0;
}
