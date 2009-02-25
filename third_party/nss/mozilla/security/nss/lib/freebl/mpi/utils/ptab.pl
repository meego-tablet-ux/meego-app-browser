#!/usr/linguist/bin/perl

# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the MPI Arbitrary Precision Integer Arithmetic library.
#
# The Initial Developer of the Original Code is
# Michael J. Fromberger <sting@linguist.dartmouth.edu>.
# Portions created by the Initial Developer are Copyright (C) 1998
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

# $Id: ptab.pl,v 1.2 2005/02/02 22:28:23 gerv%gerv.net Exp $
#

while(<>) {
    chomp;
    push(@primes, $_);
}

printf("mp_size   prime_tab_size = %d;\n", ($#primes + 1));
print "mp_digit  prime_tab[] = {\n";

print "\t";
$last = pop(@primes);
foreach $prime (sort {$a<=>$b} @primes) {
    printf("0x%04X, ", $prime);
    $brk = ($brk + 1) % 8;
    print "\n\t" if(!$brk);
}
printf("0x%04X", $last);
print "\n" if($brk);
print "};\n\n";

exit 0;
