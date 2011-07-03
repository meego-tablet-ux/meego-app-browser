#!/bin/bash

builddir="../../../out"
link="yes"

while getopts ":i" opt; do
  case $opt in
    i)
      link="no"
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      echo "Usage: $0 [-i]"
      echo "       -i install instead of link"
      exit 1
      ;;
  esac
done

install_browser () {
  browser=$1
  buildtype=$2
  installdir=$builddir/$buildtype
  if [ ! -d $installdir ]; then
    echo "$installdir does not exsit, run 'make chrome BUILDTYPE=$buildtype' first"
    return 1
  fi

  if [ -d $installdir/$browser ]; then
    rm -rf $installdir/$browser
  fi 

  if [ "x$link" = "xno" ]; then
    cp -r $browser $installdir/
    find ./common -name "*.qml" -exec cp '{}' $installdir/$browser \;
    cd $installdir
    chmod +r -R $browser
    tar zcvf $browser.tar.gz $browser
    cd -
    echo "Done: copy files for $browser $buildtype"
  else
    oldpwd=`pwd`
    relativepath="../../../chrome/browser/qt"
    mkdir $installdir/$browser && cd $installdir/$browser
    find $relativepath/$browser -name "*.qml" -exec ln -s '{}' . \;
    find $relativepath/common -name "*.qml" -exec ln -s '{}' . \;
    echo "Done: link files for $browser $buildtype"
  fi
}

install_browser meego-app-browser "Debug"
install_browser meego-app-browser "Release"
