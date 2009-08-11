#!/bin/sh
# Script to build valgrind for use with chromium

SVNDATE='{2009-07-15}'
SHORTSVNDATE=20090715
TSAN_SVN_REV='1096'

THISDIR=`dirname $0`
THISDIR=`cd $THISDIR && /bin/pwd`

case x"$1" in
x|x/*) ;;
*)
  echo "Usage: sh build-valgrind-for-chromium.sh [prefix]"
  echo "Prefix is optional, but if present, must be the absolute path to where"
  echo "you want to install valgrind's bin, include, and lib directories."
  echo "Prefix defaults to /usr/local/valgrind-$SHORTSVNDATE, where"
  echo "$SHORTSVNDATE is the date used when retrieving valgrind from svn."
  echo "Will use sudo to do the install if you don't own the parent of prefix."
  exit 1
  ;;
esac

set -x
set -e

if ld --version | grep gold
then
  # build/install-build-deps leaves original ld around, try using that
  if test -x /usr/bin/ld.orig
  then
    echo "Using /usr/bin/ld.orig instead of gold to link valgrind"
    test -d $THISDIR/override_ld && rm -rf $THISDIR/override_ld
    mkdir $THISDIR/override_ld
    ln -s /usr/bin/ld.orig $THISDIR/override_ld/ld
    PATH="$THISDIR/override_ld:$PATH"
  else
    echo "Cannot build valgrind with gold.  Please switch to normal /usr/bin/ld, rerun this script, then switch back to gold."
    exit 1
  fi
fi

# Desired parent directory for valgrind's bin, include, etc.
PREFIX="${1:-/usr/local/valgrind-$SHORTSVNDATE}"
parent_of_prefix="`dirname $PREFIX`"
if test ! -d "$parent_of_prefix"
then
  echo "Directory $parent_of_prefix does not exist"
  exit 1
fi

# Check out latest version that following patches known to apply against
rm -rf valgrind-$SHORTSVNDATE
svn co -r $SVNDATE svn://svn.valgrind.org/valgrind/trunk valgrind-$SHORTSVNDATE

cd valgrind-$SHORTSVNDATE

# Make sure svn gets the right version of the external VEX repo, too
svn update -r $SVNDATE VEX/

# Work around bug https://bugs.kde.org/show_bug.cgi?id=162848
# "fork() not handled properly"
patch -p0 < "$THISDIR"/fork.patch

# Work around bug https://bugs.kde.org/show_bug.cgi?id=186796
# "long suppressions truncated"
patch -p0 < "$THISDIR"/longlines.patch

# Work around bug http://bugs.kde.org/186790
# "Suppression counts do not include leak suppressions"
patch -p0 < "$THISDIR"/leak.patch

# Add feature bug https://bugs.kde.org/show_bug.cgi?id=201170
# "Want --show-possible option so I can ignore the bazillion possible leaks..."
patch -p0 < "$THISDIR"/possible.patch

if [ "$INSTALL_TSAN" = "yes" ]
then
  # Add ThreadSanitier to the installation.
  # ThreadSanitizer is an experimental dynamic data race detector.
  # See http://code.google.com/p/data-race-test/wiki/ThreadSanitizer
  svn checkout -r $TSAN_SVN_REV http://data-race-test.googlecode.com/svn/trunk/tsan tsan
  mkdir tsan/{docs,tests}
  touch tsan/{docs,tests}/Makefile.am
  patch -p 0 < tsan/valgrind.patch
  patch -p 0 -d VEX < tsan/vex.patch
fi

sh autogen.sh
./configure --prefix="$PREFIX"
make -j4

if ./vg-in-place true
then
  echo built valgrind passes smoke test, good
else
  echo built valgrind fails smoke test
  exit 1
fi

test -d $THISDIR/override_ld && rm -rf $THISDIR/override_ld

# Don't use sudo if we own the destination
if test -w "$parent_of_prefix"
then
   make install
else
   sudo make install
fi
