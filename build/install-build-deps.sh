#!/bin/bash -e

# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to install everything needed to build chromium (well, ideally, anyway)
# See http://code.google.com/p/chromium/wiki/LinuxBuildInstructions
# and http://code.google.com/p/chromium/wiki/LinuxBuild64Bit

usage() {
  echo "Usage: $0 [--options]"
  echo "Options:"
  echo "--[no-]syms: enable or disable installation of debugging symbols"
  echo "--[no-]gold: enable or disable installation of gold linker"
  echo "--[no-]lib32: enable or disable installation of 32 bit libraries"
  echo "Script will prompt interactively if options not given."
  exit 1
}

while test "$1" != ""
do
  case "$1" in
  --syms)     do_inst_syms=1;;
  --no-syms)  do_inst_syms=0;;
  --gold)     do_inst_gold=1;;
  --no-gold)  do_inst_gold=0;;
  --lib32)    do_inst_lib32=1;;
  --no-lib32) do_inst_lib32=0;;
  *) usage;;
  esac
  shift
done

install_gold() {
  # Gold is optional; it's a faster replacement for ld,
  # and makes life on 2GB machines much more pleasant.

  # First make sure root can access this directory, as that's tripped
  # up some folks.
  if sudo touch xyz.$$
  then
    sudo rm xyz.$$
  else
    echo root cannot write to the current directory, not installing gold
    return
  fi

  BINUTILS=binutils-2.21
  BINUTILS_URL=http://ftp.gnu.org/gnu/binutils/$BINUTILS.tar.bz2
  BINUTILS_SHA1=ef93235588eb443e4c4a77f229a8d131bccaecc6

  test -f $BINUTILS.tar.bz2 || wget $BINUTILS_URL
  if test "`sha1sum $BINUTILS.tar.bz2|cut -d' ' -f1`" != "$BINUTILS_SHA1"
  then
    echo Bad sha1sum for $BINUTILS.tar.bz2
    exit 1
  fi

  tar -xjvf $BINUTILS.tar.bz2
  cd $BINUTILS
  patch -p1 <<EOF
diff -u -r1.103 -r1.103.2.1
--- src/gold/object.h	2010/09/08 23:54:51	1.103
+++ src/gold/object.h	2011/02/10 01:15:28	1.103.2.1
@@ -1,6 +1,6 @@
 // object.h -- support for an object file for linking in gold  -*- C++ -*-
 
-// Copyright 2006, 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
+// Copyright 2006, 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.
 // Written by Ian Lance Taylor <iant@google.com>.
 
 // This file is part of gold.
@@ -2165,15 +2165,6 @@
 		      Output_symtab_xindex*,
 		      Output_symtab_xindex*);
 
-  // Clear the local symbol information.
-  void
-  clear_local_symbols()
-  {
-    this->local_values_.clear();
-    this->local_got_offsets_.clear();
-    this->local_plt_offsets_.clear();
-  }
-
   // Record a mapping from discarded section SHNDX to the corresponding
   // kept section.
   void
diff -u -r1.60 -r1.60.2.1
--- src/gold/reloc.cc	2010/10/14 22:10:22	1.60
+++ src/gold/reloc.cc	2011/02/10 01:15:28	1.60.2.1
@@ -1,6 +1,6 @@
 // reloc.cc -- relocate input files for gold.
 
-// Copyright 2006, 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
+// Copyright 2006, 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.
 // Written by Ian Lance Taylor <iant@google.com>.
 
 // This file is part of gold.
@@ -685,9 +685,6 @@
   // Write out the local symbols.
   this->write_local_symbols(of, layout->sympool(), layout->dynpool(),
 			    layout->symtab_xindex(), layout->dynsym_xindex());
-
-  // We should no longer need the local symbol values.
-  this->clear_local_symbols();
 }
 
 // Sort a Read_multiple vector by file offset.
EOF
  ./configure --prefix=/usr/local/gold --enable-gold --enable-threads
  make maybe-all-binutils maybe-all-gold -j4
  if sudo make maybe-install-binutils maybe-install-gold
  then
    # Still need to figure out graceful way of pointing gyp to use
    # /usr/local/gold/bin/ld without requiring him to set environment
    # variables.  That will go into bootstrap-linux.sh when it's ready.
    echo "Installing gold as /usr/bin/ld."
    echo "To uninstall, do 'cd /usr/bin; sudo rm ld; sudo mv ld.orig ld'"
    test -f /usr/bin/ld && test ! -f /usr/bin/ld.orig && \
        sudo mv /usr/bin/ld /usr/bin/ld.orig
    sudo strip /usr/local/gold/bin/ld
    sudo ln -fs /usr/local/gold/bin/ld /usr/bin/ld.gold
    sudo ln -fs /usr/bin/ld.gold /usr/bin/ld
  else
    echo "make install failed, not installing gold"
  fi
}

if ! egrep -q \
    'Ubuntu (8\.04|8\.10|9\.04|9\.10|10\.04|10\.10|karmic|lucid|maverick)' \
    /etc/issue; then
  echo "Only Ubuntu 8.04 (hardy) through 10.10 (maverick) are currently" \
      "supported" >&2
  exit 1
fi

if ! uname -m | egrep -q "i686|x86_64"; then
  echo "Only x86 architectures are currently supported" >&2
  exit
fi

if [ "x$(id -u)" != x0 ]; then
  echo "Running as non-root user."
  echo "You might have to enter your password one or more times for 'sudo'."
  echo
fi

# Packages needed for chromeos only
chromeos_dev_list="libpulse-dev"

# Packages need for development
dev_list="bison fakeroot flex g++ gperf libapache2-mod-php5 libasound2-dev
          libbz2-dev libcairo2-dev libdbus-glib-1-dev libgconf2-dev
          libgl1-mesa-dev libglu1-mesa-dev libglib2.0-dev libgnome-keyring-dev
          libgtk2.0-dev libjpeg62-dev libnspr4-dev libnss3-dev libpam0g-dev
          libsctp-dev libsqlite3-dev libxslt1-dev libxss-dev libxtst-dev
          mesa-common-dev msttcorefonts patch perl php5-cgi pkg-config python
          python-dev rpm subversion ttf-dejavu-core ttf-kochi-gothic
          ttf-kochi-mincho wdiff libcurl4-gnutls-dev
          $chromeos_dev_list"

# Run-time libraries required by chromeos only
chromeos_lib_list="libpulse0 libbz2-1.0 libcurl4-gnutls-dev"

# Full list of required run-time libraries
lib_list="libatk1.0-0 libc6 libasound2 libcairo2 libdbus-glib-1-2 libexpat1
          libfontconfig1 libfreetype6 libglib2.0-0 libgnome-keyring0 libgtk2.0-0
          libnspr4-0d libnss3-1d libpam0g libpango1.0-0 libpcre3 libpixman-1-0
          libpng12-0 libstdc++6 libsqlite3-0 libx11-6 libxau6 libxcb1
          libxcomposite1 libxcursor1 libxdamage1 libxdmcp6 libxext6 libxfixes3
          libxi6 libxinerama1 libxrandr2 libxrender1 libxtst6 zlib1g
          $chromeos_lib_list"

# Debugging symbols for all of the run-time libraries
dbg_list="libatk1.0-dbg libc6-dbg libcairo2-dbg
          libfontconfig1-dbg libglib2.0-0-dbg libgtk2.0-0-dbg libnspr4-0d-dbg
          libnss3-1d-dbg libpango1.0-0-dbg libpcre3-dbg libpixman-1-0-dbg
          libx11-6-dbg libxau6-dbg libxcb1-dbg libxcomposite1-dbg
          libxcursor1-dbg libxdamage1-dbg libxdmcp6-dbg libxext6-dbg
          libxfixes3-dbg libxi6-dbg libxinerama1-dbg libxrandr2-dbg
          libxrender1-dbg libxtst6-dbg zlib1g-dbg"

# CUPS package changed its name from hardy to the next version. Include
# proper package here depending on the system.
if egrep -q 'Ubuntu (8\.04|8\.10)' /etc/issue; then
  dev_list="${dev_list} libcupsys2-dev"
else
  dev_list="${dev_list} libcups2-dev"
fi

# apache2.2-bin package was introduced in karmic.
if egrep -q 'Ubuntu (8\.04|8\.10|9\.04)' /etc/issue; then
  dev_list="${dev_list} apache2"
else
  dev_list="${dev_list} apache2.2-bin"
fi

# Waits for the user to press 'Y' or 'N'. Either uppercase of lowercase is
# accepted. Returns 0 for 'Y' and 1 for 'N'. If an optional parameter has
# been provided to yes_no(), the function also accepts RETURN as a user input.
# The parameter specifies the exit code that should be returned in that case.
# The function will echo the user's selection followed by a newline character.
# Users can abort the function by pressing CTRL-C. This will call "exit 1".
yes_no() {
  local c
  while :; do
    c="$(trap 'stty echo -iuclc icanon 2>/dev/null' EXIT INT TERM QUIT
         stty -echo iuclc -icanon 2>/dev/null
         dd count=1 bs=1 2>/dev/null | od -An -tx1)"
    case "$c" in
      " 0a") if [ -n "$1" ]; then
               [ $1 -eq 0 ] && echo "Y" || echo "N"
               return $1
             fi
             ;;
      " 79") echo "Y"
             return 0
             ;;
      " 6e") echo "N"
             return 1
             ;;
      "")    echo "Aborted" >&2
             exit 1
             ;;
      *)     # The user pressed an unrecognized key. As we are not echoing
             # any incorrect user input, alert the user by ringing the bell.
             (tput bel) 2>/dev/null
             ;;
    esac
  done
}

if test "$do_inst_syms" = ""
then
  echo "This script installs all tools and libraries needed to build Chromium."
  echo ""
  echo "For most of the libraries, it can also install debugging symbols, which"
  echo "will allow you to debug code in the system libraries. Most developers"
  echo "won't need these symbols."
  echo -n "Do you want me to install them for you (y/N) "
  if yes_no 1; then
    do_inst_syms=1
  fi
fi
if test "$do_inst_syms" = "1"; then
  echo "Installing debugging symbols."
else
  echo "Skipping installation of debugging symbols."
  dbg_list=
fi

sudo apt-get update

# We initially run "apt-get" with the --reinstall option and parse its output.
# This way, we can find all the packages that need to be newly installed
# without accidentally promoting any packages from "auto" to "manual".
# We then re-run "apt-get" with just the list of missing packages.
echo "Finding missing packages..."
packages="${dev_list} ${lib_list} ${dbg_list}"
# Intentially leaving $packages unquoted so it's more readable.
echo "Packages required: " $packages
echo
new_list_cmd="sudo apt-get install --reinstall $(echo $packages)"
if new_list="$(yes n | LANG=C $new_list_cmd)"; then
  # We probably never hit this following line.
  echo "No missing packages, and the packages are up-to-date."
elif [ $? -eq 1 ]; then
  # We expect apt-get to have exit status of 1.
  # This indicates that we canceled the install with "yes n|".
  new_list=$(echo "$new_list" |
    sed -e '1,/The following NEW packages will be installed:/d;s/^  //;t;d')
  new_list=$(echo "$new_list" | sed 's/ *$//')
  if [ -z "$new_list" ] ; then
    echo "No missing packages, and the packages are up-to-date."
  else
    echo "Installing missing packages: $new_list."
    sudo apt-get install ${new_list}
  fi
  echo
else
  # An apt-get exit status of 100 indicates that a real error has occurred.

  # I am intentionally leaving out the '"'s around new_list_cmd,
  # as this makes it easier to cut and paste the output
  echo "The following command failed: " ${new_list_cmd}
  echo
  echo "It produces the following output:"
  yes n | $new_list_cmd || true
  echo
  echo "You will have to install the above packages yourself."
  echo
  exit 100
fi

# Some operating systems already ship gold (on recent Debian and
# Ubuntu you can do "apt-get install binutils-gold" to get it), but
# older releases didn't.  Additionally, gold 2.20 (included in Ubuntu
# Lucid) makes binaries that just segfault, and 2.20.1 does not support
# --map-whole-files.
# So install from source if we don't have a good version.

case `ld --version` in
*gold*2.2[1-9].*) ;;
* )
  if test "$do_inst_gold" = ""
  then
    echo "Gold is a new linker that links Chrome 5x faster than ld."
    echo "Don't use it if you need to link other apps (e.g. valgrind, wine)"
    echo -n "REPLACE SYSTEM LINKER ld with gold and back up ld? (y/N) "
    if yes_no 1; then
      do_inst_gold=1
    fi
  fi
  if test "$do_inst_gold" = "1"
  then
    # If the system provides a good version of gold, just install it.
    if apt-cache show binutils-gold | grep -Eq 'Version: 2.2[1-9].*'; then
      echo "Installing binutils-gold. Backing up ld as ld.single."
      sudo apt-get install binutils-gold
    else
      # FIXME: avoid installing as /usr/bin/ld
      echo "Building binutils. Backing up ld as ld.orig."
      install_gold || exit 99
    fi
  else
    echo "Not installing gold."
  fi
esac

# Install 32bit backwards compatibility support for 64bit systems
if [ "$(uname -m)" = "x86_64" ]; then
  if test "$do_inst_lib32" = ""
  then
    echo "Installing 32bit libraries not already provided by the system"
    echo
    echo "This is only needed to build a 32-bit Chrome on your 64-bit system."
    echo
    echo "While we only need to install a relatively small number of library"
    echo "files, we temporarily need to download a lot of large *.deb packages"
    echo "that contain these files. We will create new *.deb packages that"
    echo "include just the 32bit libraries. These files will then be found on"
    echo "your system in places like /lib32, /usr/lib32, /usr/lib/debug/lib32,"
    echo "/usr/lib/debug/usr/lib32. If you ever need to uninstall these files,"
    echo "look for packages named *-ia32.deb."
    echo "Do you want me to download all packages needed to build new 32bit"
    echo -n "package files (Y/n) "
    if yes_no 0; then
      do_inst_lib32=1
    fi
  fi
  if test "$do_inst_lib32" != "1"
  then
    echo "Exiting without installing any 32bit libraries."
    exit 0
  fi

  # Standard 32bit compatibility libraries
  echo "First, installing the limited existing 32-bit support..."
  cmp_list="ia32-libs lib32asound2-dev lib32readline5-dev lib32stdc++6 lib32z1
            lib32z1-dev libc6-dev-i386 libc6-i386 g++-multilib"
  sudo apt-get install $cmp_list

  tmp=/tmp/install-32bit.$$
  trap 'rm -rf "${tmp}"' EXIT INT TERM QUIT
  mkdir -p "${tmp}/apt/lists/partial" "${tmp}/cache" "${tmp}/partial"
  touch "${tmp}/status"

  [ -r /etc/apt/apt.conf ] && cp /etc/apt/apt.conf "${tmp}/apt/"
  cat >>"${tmp}/apt/apt.conf" <<EOF
        Apt::Architecture "i386";
        Dir::Cache "${tmp}/cache";
        Dir::Cache::Archives "${tmp}/";
        Dir::State::Lists "${tmp}/apt/lists/";
        Dir::State::status "${tmp}/status";
EOF

  # Download 32bit packages
  echo "Computing list of available 32bit packages..."
  sudo apt-get -c="${tmp}/apt/apt.conf" update

  echo "Downloading available 32bit packages..."
  sudo apt-get -c="${tmp}/apt/apt.conf" \
          --yes --download-only --force-yes --reinstall install \
          ${lib_list} ${dbg_list}

  # Open packages, remove everything that is not a library, move the
  # library to a lib32 directory and package everything as a *.deb file.
  echo "Repackaging and installing 32bit packages for use on 64bit systems..."
  for i in ${lib_list} ${dbg_list}; do
    orig="$(echo "${tmp}/${i}"_*_i386.deb)"
    compat="$(echo "${orig}" |
              sed -e 's,\(_[^_/]*_\)i386\(.deb\),-ia32\1amd64\2,')"
    rm -rf "${tmp}/staging"
    msg="$(fakeroot -u sh -exc '
      # Unpack 32bit Debian archive
      umask 022
      mkdir -p "'"${tmp}"'/staging/dpkg/DEBIAN"
      cd "'"${tmp}"'/staging"
      ar x "'${orig}'"
      tar zCfx dpkg data.tar.gz
      tar zCfx dpkg/DEBIAN control.tar.gz

      # Create a posix extended regular expression fragment that will
      # recognize the includes which have changed. Should be rare,
      # will almost always be empty.
      includes=`sed -n -e "s/^[0-9a-z]*  //g" \
                       -e "\,usr/include/,p" dpkg/DEBIAN/md5sums |
                  xargs -n 1 -I FILE /bin/sh -c \
                    "cmp -s dpkg/FILE /FILE || echo FILE" |
                  tr "\n" "|" |
                  sed -e "s,|$,,"`

      # If empty, set it to not match anything.
      test -z "$includes" && includes="^//"

      # Turn the conflicts into an extended RE for removal from the
      # Provides line.
      conflicts=`sed -n -e "/Conflicts/s/Conflicts: *//;T;s/, */|/g;p" \
                   dpkg/DEBIAN/control`

      # Rename package, change architecture, remove conflicts and dependencies
      sed -r -i                              \
          -e "/Package/s/$/-ia32/"           \
          -e "/Architecture/s/:.*$/: amd64/" \
          -e "/Depends/s/:.*/: ia32-libs/"   \
          -e "/Provides/s/($conflicts)(, *)?//g;T1;s/, *$//;:1"   \
          -e "/Recommends/d"                 \
          -e "/Conflicts/d"                  \
        dpkg/DEBIAN/control

      # Only keep files that live in "lib" directories or the includes
      # that have changed.
      sed -r -i                                                               \
          -e "/\/lib64\//d" -e "/\/.?bin\//d"                                 \
          -e "\,$includes,s,[ /]include/,&32/,g;s,include/32/,include32/,g"   \
          -e "s, lib/, lib32/,g"                                              \
          -e "s,/lib/,/lib32/,g"                                              \
          -e "t;d"                                                            \
          -e "\,^/usr/lib32/debug\(.*/lib32\),s,^/usr/lib32/debug,/usr/lib/debug," \
        dpkg/DEBIAN/md5sums

      # Re-run ldconfig after installation/removal
      { echo "#!/bin/sh"; echo "[ \"x\$1\" = xconfigure ]&&ldconfig||:"; } \
        >dpkg/DEBIAN/postinst
      { echo "#!/bin/sh"; echo "[ \"x\$1\" = xremove ]&&ldconfig||:"; } \
        >dpkg/DEBIAN/postrm
      chmod 755 dpkg/DEBIAN/postinst dpkg/DEBIAN/postrm

      # Remove any other control files
      find dpkg/DEBIAN -mindepth 1 "(" -name control -o -name md5sums -o \
                       -name postinst -o -name postrm ")" -o -print |
        xargs -r rm -rf

      # Remove any files/dirs that live outside of "lib" directories,
      # or are not in our list of changed includes.
      find dpkg -mindepth 1 -regextype posix-extended \
          "(" -name DEBIAN -o -name lib -o -regex "dpkg/($includes)" ")" \
          -prune -o -print | tac |
        xargs -r -n 1 sh -c "rm \$0 2>/dev/null || rmdir \$0 2>/dev/null || : "
      find dpkg -name lib64 -o -name bin -o -name "?bin" |
        tac | xargs -r rm -rf

      # Remove any symbolic links that were broken by the above steps.
      find -L dpkg -type l -print | tac | xargs -r rm -rf

      # Rename lib to lib32, but keep debug symbols in /usr/lib/debug/usr/lib32
      # That is where gdb looks for them.
      find dpkg -type d -o -path "*/lib/*" -print |
        xargs -r -n 1 sh -c "
          i=\$(echo \"\${0}\" |
               sed -e s,/lib/,/lib32/,g \
               -e s,/usr/lib32/debug\\\\\(.*/lib32\\\\\),/usr/lib/debug\\\\1,);
          mkdir -p \"\${i%/*}\";
          mv \"\${0}\" \"\${i}\""

      # Rename include to include32.
      [ -d "dpkg/usr/include" ] && mv "dpkg/usr/include" "dpkg/usr/include32"

      # Prune any empty directories
      find dpkg -type d | tac | xargs -r -n 1 rmdir 2>/dev/null || :

      # Create our own Debian package
      cd ..
      dpkg --build staging/dpkg .' 2>&1)"
    compat="$(eval echo $(echo "${compat}" |
                          sed -e 's,_[^_/]*_amd64.deb,_*_amd64.deb,'))"
    [ -r "${compat}" ] || {
      echo "${msg}" >&2
      echo "Failed to build new Debian archive!" >&2
      exit 1
    }

    msg="$(sudo dpkg -i "${compat}" 2>&1)" && {
        echo "Installed ${compat##*/}"
      } || {
        # echo "${msg}" >&2
        echo "Skipped ${compat##*/}"
      }
  done

  # Add symbolic links for developing 32bit code
  echo "Adding missing symbolic links, enabling 32bit code development..."
  for i in $(find /lib32 /usr/lib32 -maxdepth 1 -name \*.so.\* |
             sed -e 's/[.]so[.][0-9].*/.so/' |
             sort -u); do
    [ "x${i##*/}" = "xld-linux.so" ] && continue
    [ -r "$i" ] && continue
    j="$(ls "$i."* | sed -e 's/.*[.]so[.]\([^.]*\)$/\1/;t;d' |
         sort -n | tail -n 1)"
    [ -r "$i.$j" ] || continue
    sudo ln -s "${i##*/}.$j" "$i"
  done
fi
