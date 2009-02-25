#! /bin/sh
# 
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
# The Original Code is the PKIX-C library.
#
# The Initial Developer of the Original Code is
# Sun Microsystems, Inc.
# Portions created by the Initial Developer are
# Copyright 2004-2007 Sun Microsystems, Inc.  All Rights Reserved.
#
# Contributor(s):
#   Sun Microsystems, Inc.
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
#
# runTests.sh
#

totalErrors=0
pkixErrors=0
pkixplErrors=0
checkMemArg=""
arenasArg=""
quietArg=""
memText=""

############################## libpkix_init ###############################
# local shell function to initialize this script
########################################################################
libpkix_init()
{
  SCRIPTNAME="libpkixs.sh"
  if [ -z "${CLEANUP}" ] ; then     # if nobody else is responsible for
      CLEANUP="${SCRIPTNAME}"       # cleaning this script will do it
  fi

  LIBPKIX_CURDIR=`pwd`
  if [ -z "${INIT_SOURCED}" -o "${INIT_SOURCED}" != "TRUE" ] ; then
      cd ../common
      . ./init.sh
  fi
  cd ${LIBPKIX_CURDIR}

  # test at libpkix is written in ksh and hence cannot be sourced "."
  # by this sh script. While we want to provide each libpkix test script the
  # ability to be executed alone, we will need to use common/init.sh
  # to set up bin etc. Since variable values can not be passed to sub-directory
  # script for checking ($INIT_SOURCED), log is recreated and old data lost.
  # The cludge way provided here is not ideal, but works (for now) :
  # We save the log up to this point then concatenate it with libpkix log
  # as the final one.
  LOGFILE_ALL=${LOGFILE}
  if [ ! -z ${LOGFILE_ALL} ] ; then
       mv ${LOGFILE_ALL} ${LOGFILE_ALL}.tmp
       touch ${LOGFILE_ALL}
  fi

  SCRIPTNAME="libpkixs.sh"
  LIBPKIX_LOG=${HOSTDIR}/libpkix.log    #we don't want all the errormessages 
         # in the output.log, otherwise we can't tell what's a real error


}

############################## libpkix_cleanup ############################
# local shell function to finish this script (no exit since it might be
# sourced)
########################################################################
libpkix_cleanup()
{
  if [ ! -z ${LOGFILE_ALL} ] ; then
       rm ${LOGFILE_ALL}
       cat ${LOGFILE_ALL}.tmp ${LIBPKIX_LOG} > ${LOGFILE_ALL}
       rm ${LOGFILE_ALL}.tmp
  fi

  html "</TABLE><BR>" 
  cd ${QADIR}
  . common/cleanup.sh
}

############################## libpkix_UT_main ############################
# local shell function to run libpkix unit tests
########################################################################
ParseArgs ()
{
    while [ $# -gt 0 ]; do
        if [ $1 == "-checkmem" ]; then
            checkMemArg=$1
            memText="   (Memory Checking Enabled)"
        elif [ $1 == "-quiet" ]; then
            quietArg=$1
        elif [ $1 == "-arenas" ]; then
            arenasArg=$1
        fi
        shift
    done
}

libpkix_UT_main()
{

html_head "LIBPKIX Unit Tests"

ParseArgs 

echo "*******************************************************************************"
echo "START OF ALL TESTS${memText}"
echo "*******************************************************************************"
echo ""

echo "RUNNING tests in pkix_pl_test";
html_msg 0 0 "Running tests in pkix_pl_test:"
cd pkix_pl_tests;
runPLTests.sh ${arenasArg} ${checkMemArg} ${quietArg}
pkixplErrors=$?
html_msg $? 0 "Results of tests in pkix_pl_test"

echo "RUNNING tests in pkix_test";
html_msg 0 0 "Running tests in pkix_test:"
cd ../pkix_tests;
runTests.sh ${arenasArg} ${checkMemArg} ${quietArg}
pkixErrors=$?
html_msg $? 0 "Results of tests in pkix_test"

echo "RUNNING performance tests in sample_apps";
html_msg 0 0 "Running performance tests in sample_apps:"
cd ../sample_apps;
runPerf.sh ${arenasArg} ${checkMemArg} ${quietArg}
pkixPerfErrors=$?
html_msg $? 0 "Results of performance tests in sample_apps"

totalErrors=`expr ${pkixErrors} + ${pkixplErrors} + ${pkixPerfErrors}`

if [ ${totalErrors} -eq 0 ]; then
    echo ""
    echo "************************************************************"
    echo "END OF ALL TESTS: ALL TESTS COMPLETED SUCCESSFULLY"
    echo "************************************************************"
    html_msg ${totalErrors} 0 "ALL LIBPKIX TESTS COMPLETED SUCCESSFULLY"

    return 0
fi

if [ ${totalErrors} -eq 1 ]; then
    plural=""
else
    plural="S"
fi

if [ ${totalErrors} -ne 0 ]; then
    echo ""
    echo "************************************************************"
    echo "END OF ALL TESTS: ${totalErrors} TEST${plural} FAILED"
    echo "************************************************************"
    html_msg 1 0 "${totalErrors} LIBPKIX TEST${plural} FAILED"
return 1
fi
}

############################## libpkix_setup_db ############################
# Sets up directory and db for pkix functional test.
########################################################################
libpkix_setup_db()
{
  fnCaller=$1

  DB_DIR="${HOSTDIR}/libpkix_functional"
  CERT_DIR="${QADIR}/libpkix/certs"
  
  mkdir -p $DB_DIR
  if [ ! -f ${R_PWFILE} ]; then
      echo nss > ${R_PWFILE}
  fi
  echo "Initializing Cert DB in $FN_DB_DIR"
  certutil -N -d "${DB_DIR}" -f "${R_PWFILE}" 2>&1
  RET=$?
  if [ "$RET" -ne 0 ]; then
      return $RET
  fi

  echo "Loading certs into DB at $DB_DIR"
  output=$TMP/libpkix_setup.tmp
  while read certName trusts; do
      certutil -d $DB_DIR -A -n $certName -t $trusts -i $CERT_DIR/$certName.cert > $output 2>&1
      if [ $? -ne 0 ]; then
          echo "WARNING: unable to add a certificate($certName) into database"
          echo "certutil output:"
          cat $output
      fi
      rm -f $output
  done < $QADIR/libpkix/cert_trust.map
  
}

############################## libpkix_leak_test ############################
# Runs pkix object and memory leak test
########################################################################
libpkix_leak_test()
{

    if [ -n "${MEMLEAK_DBG}" ]; then
        LOGNAME="libpkix_memleak"
        LOGFILE=${LOGDIR}/${LOGNAME}.log
        tmpLogFile=$LOGFILE.tmp

        html_head "Memory leak checking - libpkix"
    else
        tmpLogFile=$DB_DIR/libpkix_memoryleak.log

        html_head "LIBPKIX Object Leak Tests"
    fi 
    while read status leafCert explPolicy others; do
        # continue with empty and commented lines. 
        [ -z "$status" -o "$status" = "#" ] && continue

        # can only run positive tests. Positive validation
        # status is the exit condition for the code in the library.
        [ $status -ne 0 ] && continue;
        extraOpt=""
        if [ "$explPolicy" -a "$explPolicy" != "undef" ]; then
            extraOpt="-p -o $explPolicy"
        fi
        cmd="vfychain -d $DB_DIR $extraOpt $CERT_DIR/$leafCert.cert"
        if [ -n "$MEMLEAK_DBG" ]; then
            cmd="$RUN_COMMAND_DBG $cmd"
        fi
        echo $cmd
        $cmd > $tmpLogFile
        if [ -z "$MEMLEAK_DBG" ]; then
            cat $tmpLogFile
            grep "Memory Leak:" $tmpLogFile
            html_msg $? 1 "Object leak tests with $leafCert certificate"
        else
            cat $tmpLogFile >> $LOGFILE
        fi
    done < $QADIR/libpkix/vfychain_test.lst

    if [ -n "${MEMLEAK_DBG}" ]; then
        log_parse
        ret=$?
        html_msg ${ret} 0 "${LOGNAME}" \
            "produced a returncode of $ret, expected is 0"
        html "</TABLE><BR>"
    fi
}


################## main #################################################

libpkix_init 
libpkix_setup_db
if [ "$PKIX_OBJECT_LEAK_TEST" ]; then
    libpkix_leak_test
    if [ -n "${MEMLEAK_DBG}" ]; then
        libpkix_cleanup
        return 1
    fi
fi

# place other tests here

# Run libpkix unit tests at the end
if [ "$BUILD_LIBPKIX_TEST" ]; then
    libpkix_UT_main  | tee ${LIBPKIX_LOG}
fi
libpkix_cleanup
