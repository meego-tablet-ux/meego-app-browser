/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is the Solaris software cryptographic token.
 *
 * The Initial Developer of the Original Code is
 * Sun Microsystems, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Sun Microsystems, Inc.
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

#ifndef MPI_AMD64
#error This file only works on AMD64 platforms.
#endif

#include <mpi-priv.h>

/*
 * MPI glue
 *
 */

/* Presently, this is only used by the Montgomery arithmetic code. */
/* c += a * b */
void MPI_ASM_DECL s_mpv_mul_d_add_prop(const mp_digit *a, mp_size a_len,
                                       mp_digit b, mp_digit *c)
{
  mp_digit w;
  mp_digit d;

  d = s_mpv_mul_add_vec64(c, a, a_len, b);
  c += a_len;
  while (d) {
    w = c[0] + d;
    d = (w < c[0] || w < d);
    *c++ = w;
  }
}

