
set rcsid {$Id: $}

# Documentation for this script. This may be output to stderr
# if the script is invoked incorrectly. See the [process_options]
# proc below.
#
set ::USAGE_MESSAGE {
This Tcl script is used to test the various configurations required
before releasing a new version. Supported command line options (all 
optional) are:

    -makefile PATH-TO-MAKEFILE           (default "releasetest.mk")
    -platform PLATFORM                   (see below)
    -quick    BOOLEAN                    (default "0")

The default value for -makefile is "./releasetest.mk".

The script determines the default value for -platform using the
$tcl_platform(os) and $tcl_platform(machine) variables. Supported 
platforms are "Linux-x86", "Linux-x86_64" and "Darwin-i386".

If the -quick option is set to true, then the "veryquick.test" script
is run for all compilation configurations. Otherwise, sometimes "all.test"
is run, sometimes "veryquick.test".

Almost any SQLite makefile (except those generated by configure - see below)
should work. The following properties are required:

  * The makefile should support the "fulltest" target.
  * The makefile should support the variable "OPTS" as a way to pass
    options from the make command line to lemon and the C compiler.

More precisely, the following invocation must be supported:

  make -f $::MAKEFILE fulltest OPTS="-DSQLITE_SECURE_DELETE=1 -DSQLITE_DEBUG=1"

Makefiles generated by the sqlite configure program cannot be used as
they do not respect the OPTS variable.

Example Makefile contents:

  ########################################################
  TOP=/home/dan/work/sqlite/sqlite

  TCL_FLAGS=-I/home/dan/tcl/include
  LIBTCL=-L/home/dan/tcl/lib -ltcl

  BCC = gcc
  TCC = gcc -ansi -g $(CFLAGS)
  NAWK   = awk
  AR     = ar cr
  RANLIB = ranlib
  THREADLIB = -lpthread -ldl
  include $(TOP)/main.mk
  ########################################################
}

array set ::Configs {
  "Default" {
    -O2
  }
  "Unlock-Notify" {
    -O2
    -DSQLITE_ENABLE_UNLOCK_NOTIFY
    -DSQLITE_THREADSAFE
    -DOS_UNIX
    -DSQLITE_TCL_DEFAULT_FULLMUTEX=1
  }
  "Secure-Delete" {
    -O2
    -DSQLITE_SECURE_DELETE=1
    -DSQLITE_SOUNDEX=1
  }
  "Update-Delete-Limit" {
    -O2
    -DSQLITE_DEFAULT_FILE_FORMAT=4
    -DSQLITE_ENABLE_UPDATE_DELETE_LIMIT=1
  }
  "Debug-One" {
    -O2
    -DSQLITE_DEBUG=1
    -DSQLITE_MEMDEBUG=1
    -DSQLITE_MUTEX_NOOP=1
    -DSQLITE_TCL_DEFAULT_FULLMUTEX=1
    -DSQLITE_ENABLE_FTS3=1
    -DSQLITE_ENABLE_RTREE=1
    -DSQLITE_ENABLE_MEMSYS5=1
    -DSQLITE_ENABLE_MEMSYS3=1
    -DSQLITE_ENABLE_COLUMN_METADATA=1
  }
  "Device-One" {
    -O2
    -DSQLITE_DEBUG=1
    -DSQLITE_DEFAULT_AUTOVACUUM=1
    -DSQLITE_DEFAULT_CACHE_SIZE=64
    -DSQLITE_DEFAULT_PAGE_SIZE=1024
    -DSQLITE_DEFAULT_TEMP_CACHE_SIZE=32
    -DSQLITE_DISABLE_LFS=1
    -DSQLITE_ENABLE_ATOMIC_WRITE=1
    -DSQLITE_ENABLE_IOTRACE=1
    -DSQLITE_ENABLE_MEMORY_MANAGEMENT=1
    -DSQLITE_MAX_PAGE_SIZE=4096
    -DSQLITE_OMIT_LOAD_EXTENSION=1
    -DSQLITE_OMIT_PROGRESS_CALLBACK=1
    -DSQLITE_OMIT_VIRTUALTABLE=1
    -DSQLITE_TEMP_STORE=3
  }
  "Device-Two" {
    -DSQLITE_4_BYTE_ALIGNED_MALLOC=1
    -DSQLITE_DEFAULT_AUTOVACUUM=1
    -DSQLITE_DEFAULT_CACHE_SIZE=1000
    -DSQLITE_DEFAULT_LOCKING_MODE=0
    -DSQLITE_DEFAULT_PAGE_SIZE=1024
    -DSQLITE_DEFAULT_TEMP_CACHE_SIZE=1000
    -DSQLITE_DISABLE_LFS=1
    -DSQLITE_ENABLE_FTS3=1
    -DSQLITE_ENABLE_MEMORY_MANAGEMENT=1
    -DSQLITE_ENABLE_RTREE=1
    -DSQLITE_MAX_COMPOUND_SELECT=50
    -DSQLITE_MAX_PAGE_SIZE=32768
    -DSQLITE_OMIT_TRACE=1
    -DSQLITE_TEMP_STORE=3
    -DSQLITE_THREADSAFE=2
  }
  "Locking-Style" {
    -O2
    -DSQLITE_ENABLE_LOCKING_STYLE=1
  }
  "OS-X" {
    -DSQLITE_OMIT_LOAD_EXTENSION=1
    -DSQLITE_DEFAULT_MEMSTATUS=0
    -DSQLITE_THREADSAFE=2
    -DSQLITE_OS_UNIX=1
    -DSQLITE_ENABLE_LOCKING_STYLE=1
    -DUSE_PREAD=1
    -DSQLITE_ENABLE_RTREE=1
    -DSQLITE_ENABLE_FTS3=1
    -DSQLITE_ENABLE_FTS3_PARENTHESIS=1
    -DSQLITE_DEFAULT_CACHE_SIZE=1000
    -DSQLITE_MAX_LENGTH=2147483645
    -DSQLITE_MAX_VARIABLE_NUMBER=500000
    -DSQLITE_DEBUG=1 
    -DSQLITE_PREFER_PROXY_LOCKING=1
  }
  "Extra-Robustness" {
    -DSQLITE_ENABLE_OVERSIZE_CELL_CHECK=1
  }
}

array set ::Platforms {
  Linux-x86_64 {
    "Secure-Delete"           test
    "Unlock-Notify"           "QUICKTEST_INCLUDE=notify2.test test"
    "Update-Delete-Limit"     test
    "Debug-One"               test
    "Extra-Robustness"        test
    "Device-Two"              test
    "Default"                 "threadtest test"
    "Device-One"              fulltest
  }
  Linux-i686 {
    "Unlock-Notify"           "QUICKTEST_INCLUDE=notify2.test test"
    "Device-One"              test
    "Device-Two"              test
    "Default"                 "threadtest fulltest"
  }
  Darwin-i386 {
    "Locking-Style"           test
    "OS-X"                    "threadtest fulltest"
  }
}

# End of configuration section.
#########################################################################
#########################################################################

foreach {key value} [array get ::Platforms] {
  foreach {v t} $value {
    if {0==[info exists ::Configs($v)]} {
      puts stderr "No such configuration: \"$v\""
      exit -1
    }
  }
}

proc run_test_suite {name testtarget config} {
  
  # Tcl variable $opts is used to build up the value used to set the 
  # OPTS Makefile variable. Variable $cflags holds the value for
  # CFLAGS. The makefile will pass OPTS to both gcc and lemon, but
  # CFLAGS is only passed to gcc.
  #
  set cflags ""
  set opts ""
  foreach arg $config {
    if {[string match -D* $arg]} {
      lappend opts $arg
    } else {
      lappend cflags $arg
    }
  }

  set cflags [join $cflags " "]
  set opts   [join $opts " "]
  append opts " -DSQLITE_NO_SYNC=1 -DHAVE_USLEEP"

  # Set the sub-directory to use.
  #
  set dir [string tolower [string map {- _ " " _} $name]]

  if {$::tcl_platform(platform)=="windows"} {
    append opts " -DSQLITE_OS_WIN=1"
  } elseif {$::tcl_platform(platform)=="os2"} {
    append opts " -DSQLITE_OS_OS2=1"
  } else {
    append opts " -DSQLITE_OS_UNIX=1"
  }

  # Run the test.
  #
  set makefile [file normalize $::MAKEFILE]
  file mkdir $dir
  puts -nonewline "Testing configuration \"$name\" (logfile=$dir/test.log)..."
  flush stdout

  set makecmd [concat                                  \
    [list exec make -C $dir -f $makefile clean]        \
    $testtarget                                        \
    [list CFLAGS=$cflags OPTS=$opts >& $dir/test.log]  \
  ]

  set tm1 [clock seconds] 
  set rc [catch $makecmd]
  set tm2 [clock seconds]

  set minutes [expr {($tm2-$tm1)/60}]
  set seconds [expr {($tm2-$tm1)%60}]
  puts -nonewline [format " (%d:%.2d) " $minutes $seconds]
  if {$rc} {
    puts "FAILED."
  } else {
    puts "Ok."
  }
}


# This proc processes the command line options passed to this script.
# Currently the only option supported is "-makefile", default
# "releasetest.mk". Set the ::MAKEFILE variable to the value of this
# option.
#
proc process_options {argv} {
  set ::MAKEFILE releasetest.mk                       ;# Default value
  set ::QUICK    0                                    ;# Default value
  set platform $::tcl_platform(os)-$::tcl_platform(machine)

  for {set i 0} {$i < [llength $argv]} {incr i} {
    switch -- [lindex $argv $i] {
      -makefile {
        incr i
        set ::MAKEFILE [lindex $argv $i]
      }

      -platform {
        incr i
        set platform [lindex $argv $i]
      }

      -quick {
        incr i
        set ::QUICK [lindex $argv $i]
      }
  
      default {
        puts stderr ""
        puts stderr [string trim $::USAGE_MESSAGE]
        exit -1
      }
    }
  }

  set ::MAKEFILE [file normalize $::MAKEFILE]

  if {0==[info exists ::Platforms($platform)]} {
    puts "Unknown platform: $platform"
    puts -nonewline "Set the -platform option to "
    set print [list]
    foreach p [array names ::Platforms] {
      lappend print "\"$p\""
    }
    lset print end "or [lindex $print end]"
    puts "[join $print {, }]."
    exit
  }

  set ::CONFIGLIST $::Platforms($platform)
  puts "Running the following configurations for $platform:"
  puts "    [string trim $::CONFIGLIST]"
}

# Main routine.
#
proc main {argv} {

  # Process any command line options.
  process_options $argv

  foreach {zConfig target} $::CONFIGLIST {
    if {$::QUICK} {set target test}
    set config_options $::Configs($zConfig)

    run_test_suite $zConfig $target $config_options

    # If the configuration included the SQLITE_DEBUG option, then remove
    # it and run veryquick.test. If it did not include the SQLITE_DEBUG option
    # add it and run veryquick.test.
    set debug_idx [lsearch -glob $config_options -DSQLITE_DEBUG*]
    if {$debug_idx < 0} {
      run_test_suite "${zConfig}_debug" test [
        concat $config_options -DSQLITE_DEBUG=1
      ]
    } else {
      run_test_suite "${zConfig}_ndebug" test [
        lreplace $config_options $debug_idx $debug_idx
      ]
    }

  }
}

main $argv
