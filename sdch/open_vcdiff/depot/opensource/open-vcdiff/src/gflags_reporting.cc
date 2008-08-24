// Copyright (c) 2006, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// ---
// Author: Ray Sidney
// Revamped and reorganized by Craig Silverstein
//
// This file contains code for handling the 'reporting' flags.  These
// are flags that, when present, cause the program to report some
// information and then exit.  --help and --version are the canonical
// reporting flags, but we also have flags like --helpxml, etc.
//
// There's only one function that's meant to be called externally:
// HandleCommandLineHelpFlags().  (Well, actually, ShowUsageWithFlags(),
// ShowUsageWithFlagsRestrict(), and DescribeOneFlag() can be called
// externally too, but there's little need for it.)  These are all
// declared in the main commandlineflags.h header file.
//
// HandleCommandLineHelpFlags() will check what 'reporting' flags have
// been defined, if any -- the "help" part of the function name is a
// bit misleading -- and do the relevant reporting.  It should be
// called after all flag-values have been assigned, that is, after
// parsing the command-line.

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <string>
#include <vector>
#include "google/gflags.h"

#ifndef PATH_SEPARATOR
#define PATH_SEPARATOR  '/'
#endif

using std::string;
using std::vector;

// The 'reporting' flags.  They all call exit().
DEFINE_bool(help, false,
            "show help on all flags [tip: all flags can have two dashes]");
DEFINE_bool(helpfull, false,
            "show help on all flags -- same as -help");
DEFINE_bool(helpshort, false,
            "show help on only the main module for this program");
DEFINE_string(helpon, "",
              "show help on the modules named by this flag value");
DEFINE_string(helpmatch, "",
              "show help on modules whose name contains the specified substr");
DEFINE_bool(helppackage, false,
            "show help on all modules in the main package");
DEFINE_bool(helpxml, false,
            "produce an xml version of help");
DEFINE_bool(version, false,
            "show version and build info and exit");

namespace google {

// --------------------------------------------------------------------
// DescribeOneFlag()
// DescribeOneFlagInXML()
//    Routines that pretty-print info about a flag.  These use
//    a CommandLineFlagInfo, which is the way the commandlineflags
//    API exposes static info about a flag.
// --------------------------------------------------------------------

static const int kLineLength = 80;

static void AddString(const string& s,
                      string* final_string, int* chars_in_line) {
  const int slen = static_cast<int>(s.length());
  if (*chars_in_line + 1 + slen >= kLineLength) {  // < 80 chars/line
    *final_string += "\n      ";
    *chars_in_line = 6;
  } else {
    *final_string += " ";
    *chars_in_line += 1;
  }
  *final_string += s;
  *chars_in_line += slen;
}

// Create a descriptive string for a flag.
// Goes to some trouble to make pretty line breaks.
string DescribeOneFlag(const CommandLineFlagInfo& flag) {
  string main_part = (string("    -") + flag.name +
                      " (" + flag.description + ')');
  const char* c_string = main_part.c_str();
  int chars_left = static_cast<int>(main_part.length());
  string final_string = "";
  int chars_in_line = 0;  // how many chars in current line so far?
  while (1) {
    assert(chars_left == strlen(c_string));  // Unless there's a \0 in there?
    const char* newline = strchr(c_string, '\n');
    if (newline == NULL && chars_in_line+chars_left < kLineLength) {
      // The whole remainder of the string fits on this line
      final_string += c_string;
      chars_in_line += chars_left;
      break;
    }
    if (newline != NULL && newline - c_string < kLineLength - chars_in_line) {
      int n = static_cast<int>(newline - c_string);
      final_string.append(c_string, n);
      chars_left -= n + 1;
      c_string += n + 1;
    } else {
      // Find the last whitespace on this 80-char line
      int whitespace = kLineLength-chars_in_line-1;  // < 80 chars/line
      while ( whitespace > 0 && !isspace(c_string[whitespace]) ) {
        --whitespace;
      }
      if (whitespace <= 0) {
        // Couldn't find any whitespace to make a line break.  Just dump the
        // rest out!
        final_string += c_string;
        chars_in_line = kLineLength;  // next part gets its own line for sure!
        break;
      }
      final_string += string(c_string, whitespace);
      chars_in_line += whitespace;
      while (isspace(c_string[whitespace]))  ++whitespace;
      c_string += whitespace;
      chars_left -= whitespace;
    }
    if (*c_string == '\0')
      break;
    final_string += "\n      ";
    chars_in_line = 6;
  }

  // Append data type
  AddString(string("type: ") + flag.type, &final_string, &chars_in_line);
  // Append the effective default value (i.e., the value that the flag
  // will have after the command line is parsed if the flag is not
  // specified on the command line), which may be different from the
  // stored default value. This would happen if the value of the flag
  // was modified before the command line was parsed. (Unless the
  // value was modified using SetCommandLineOptionWithMode() with mode
  // SET_FLAGS_DEFAULT.)
  // Note that we are assuming this code is being executed because a help
  // request was just parsed from the command line, in which case the
  // printed value is indeed the effective default, as long as no value
  // for the flag was parsed from the command line before "--help".
  if (strcmp(flag.type.c_str(), "string") == 0) {  // add quotes for strings
    AddString(string("default: \"") + flag.current_value + string("\""),
              &final_string, &chars_in_line);
  } else {
    AddString(string("default: ") + flag.current_value,
              &final_string, &chars_in_line);
  }

  final_string += '\n';
  return final_string;
}

// Simple routine to xml-escape a string: escape & and < only.
static string XMLText(const string& txt) {
  string ans = txt;
  for (string::size_type pos = 0; (pos = ans.find("&", pos)) != string::npos; )
    ans.replace(pos++, 1, "&amp;");
  for (string::size_type pos = 0; (pos = ans.find("<", pos)) != string::npos; )
    ans.replace(pos++, 1, "&lt;");
  return ans;
}

static string DescribeOneFlagInXML(const CommandLineFlagInfo& flag) {
  // The file and flagname could have been attributes, but default
  // and meaning need to avoid attribute normalization.  This way it
  // can be parsed by simple programs, in addition to xml parsers.
  return (string("<flag>") +
          "<file>" + XMLText(flag.filename) + "</file>" +
          "<name>" + XMLText(flag.name) + "</name>" +
          "<meaning>" + XMLText(flag.description) + "</meaning>" +
          "<default>" + XMLText(flag.default_value) + "</default>" +
          "<type>" + XMLText(flag.type) + "</type>" +
          string("</flag>"));
}

// --------------------------------------------------------------------
// ShowUsageWithFlags()
// ShowUsageWithFlagsRestrict()
// ShowXMLOfFlags()
//    These routines variously expose the registry's list of flag
//    values.  ShowUsage*() prints the flag-value information
//    to stdout in a user-readable format (that's what --help uses).
//    The Restrict() version limits what flags are shown.
//    ShowXMLOfFlags() prints the flag-value information to stdout
//    in a machine-readable format.  In all cases, the flags are
//    sorted: first by filename they are defined in, then by flagname.
// --------------------------------------------------------------------

static const char* Basename(const char* filename) {
  const char* sep = strrchr(filename, PATH_SEPARATOR);
  return sep ? sep + 1 : filename;
}

static string Dirname(const string& filename) {
  string::size_type sep = filename.rfind(PATH_SEPARATOR);
  return filename.substr(0, (sep == string::npos) ? 0 : sep);
}

// Test whether a filename contains at least one of the substrings.
static bool FileMatchesSubstring(const string& filename,
                                 const vector<string>& substrings) {
  for (vector<string>::const_iterator target = substrings.begin();
       target != substrings.end();
       ++target) {
    if (strstr(filename.c_str(), target->c_str()) != NULL) {
      return true;
    }
  }
  return false;
}

// Show help for every filename which matches any of the target substrings.
// If substrings is empty, shows help for every file. If a flag's help message
// has been stripped (e.g. by adding '#define STRIP_FLAG_HELP 1' before
// including google/gflags.h), then this flag will not be displayed by
// '--help' and its variants.
static void ShowUsageWithFlagsMatching(const char *argv0,
                                       const vector<string> &substrings) {
  fprintf(stdout, "%s: %s\n", Basename(argv0), ProgramUsage());

  vector<CommandLineFlagInfo> flags;
  GetAllFlags(&flags);           // flags are sorted by filename, then flagname

  string last_filename = "";     // so we know when we're at a new file
  bool first_directory = true;   // controls blank lines between dirs
  bool found_match = false;      // stays false iff no dir matches restrict
  for (vector<CommandLineFlagInfo>::const_iterator flag = flags.begin();
       flag != flags.end();
       ++flag) {
    if (substrings.empty() ||
        FileMatchesSubstring(flag->filename, substrings)) {
      // If the flag has been stripped, pretend that it doesn't exist.
      if (flag->description == kStrippedFlagHelp) continue;
      found_match = true;     // this flag passed the match!
      if (flag->filename != last_filename) {                      // new file
        if (Dirname(flag->filename) != Dirname(last_filename)) {  // new dir!
          if (!first_directory)
            fprintf(stdout, "\n\n");   // put blank lines between directories
          first_directory = false;
        }
        fprintf(stdout, "\n  Flags from %s:\n", flag->filename.c_str());
        last_filename = flag->filename;
      }
      // Now print this flag
      fprintf(stdout, "%s", DescribeOneFlag(*flag).c_str());
    }
  }
  if (!found_match && !substrings.empty()) {
    fprintf(stdout, "\n  No modules matched: use -help\n");
  }
}

void ShowUsageWithFlagsRestrict(const char *argv0, const char *restrict) {
  vector<string> substrings;
  if (restrict != NULL && *restrict != '\0') {
    substrings.push_back(restrict);
  }
  ShowUsageWithFlagsMatching(argv0, substrings);
}

void ShowUsageWithFlags(const char *argv0) {
  ShowUsageWithFlagsRestrict(argv0, "");
}

// Convert the help, program, and usage to xml.
static void ShowXMLOfFlags(const char *prog_name) {
  vector<CommandLineFlagInfo> flags;
  GetAllFlags(&flags);   // flags are sorted: by filename, then flagname

  // XML.  There is no corresponding schema yet
  fprintf(stdout, "<?xml version=\"1.0\"?>\n");
  // The document
  fprintf(stdout, "<AllFlags>\n");
  // the program name and usage
  fprintf(stdout, "<program>%s</program>\n",
          XMLText(Basename(prog_name)).c_str());
  fprintf(stdout, "<usage>%s</usage>\n",
          XMLText(ProgramUsage()).c_str());
  // All the flags
  for (vector<CommandLineFlagInfo>::const_iterator flag = flags.begin();
       flag != flags.end();
       ++flag) {
    if (flag->description != kStrippedFlagHelp)
      fprintf(stdout, "%s\n", DescribeOneFlagInXML(*flag).c_str());
  }
  // The end of the document
  fprintf(stdout, "</AllFlags>\n");
}

// --------------------------------------------------------------------
// ShowVersion()
//    Called upon --version.  Prints build-related info.
// --------------------------------------------------------------------

static void ShowVersion() {
  fprintf(stdout, "%s\n", ProgramInvocationShortName());
  // TODO: add other stuff, like a timestamp, who built it, what
  //       target they built, etc.

# if !defined(NDEBUG)
  fprintf(stdout, "Debug build (NDEBUG not #defined)\n");
# endif
}

// --------------------------------------------------------------------
// HandleCommandLineHelpFlags()
//    Checks all the 'reporting' commandline flags to see if any
//    have been set.  If so, handles them appropriately.  Note
//    that all of them, by definition, cause the program to exit
//    if they trigger.
// --------------------------------------------------------------------

void HandleCommandLineHelpFlags() {
  const char* progname = ProgramInvocationShortName();
  extern void (*commandlineflags_exitfunc)(int);   // in gflags.cc

  if (FLAGS_helpshort) {
    // show only flags related to this binary:
    // E.g. for fileutil.cc, want flags containing   ... "/fileutil." cc
    vector<string> substrings;
    substrings.push_back(string("/") + progname + ".");
    substrings.push_back(string("/") + progname + "-main.");
    substrings.push_back(string("/") + progname + "_main.");
    ShowUsageWithFlagsMatching(progname, substrings);
    commandlineflags_exitfunc(1);   // almost certainly exit()

  } else if (FLAGS_help || FLAGS_helpfull) {
    // show all options
    ShowUsageWithFlagsRestrict(progname, "");   // empty restrict
    commandlineflags_exitfunc(1);

  } else if (!FLAGS_helpon.empty()) {
    string restrict = "/" + FLAGS_helpon + ".";
    ShowUsageWithFlagsRestrict(progname, restrict.c_str());
    commandlineflags_exitfunc(1);

  } else if (!FLAGS_helpmatch.empty()) {
    ShowUsageWithFlagsRestrict(progname, FLAGS_helpmatch.c_str());
    commandlineflags_exitfunc(1);

  } else if (FLAGS_helppackage) {
    // Shows help for all files in the same directory as main().  We
    // don't want to resort to looking at dirname(progname), because
    // the user can pick progname, and it may not relate to the file
    // where main() resides.  So instead, we search the flags for a
    // filename like "/progname.cc", and take the dirname of that.
    vector<CommandLineFlagInfo> flags;
    GetAllFlags(&flags);
    vector<string> substrings;
    substrings.push_back(string("/") + progname + ".");
    substrings.push_back(string("/") + progname + "-main.");
    substrings.push_back(string("/") + progname + "_main.");
    string last_package = "";
    for (vector<CommandLineFlagInfo>::const_iterator flag = flags.begin();
         flag != flags.end();
         ++flag) {
      if (!FileMatchesSubstring(flag->filename, substrings))
        continue;
      const string package = Dirname(flag->filename) + "/";
      if (package != last_package) {
        ShowUsageWithFlagsRestrict(progname, package.c_str());
        if (last_package != "") {      // means this isn't our first pkg
          fprintf(stderr, "WARNING: Multiple packages contain a file=%s\n",
                  progname);
        }
        last_package = package;
      }
    }
    if (last_package == "") {   // never found a package to print
      fprintf(stderr, "WARNING: Unable to find a package for file=%s\n",
              progname);
    }
    commandlineflags_exitfunc(1);

  } else if (FLAGS_helpxml) {
    ShowXMLOfFlags(progname);
    commandlineflags_exitfunc(1);

  } else if (FLAGS_version) {
    ShowVersion();
    // Unlike help, we may be asking for version in a script, so return 0
    commandlineflags_exitfunc(0);
  }
}

}  // namespace google
