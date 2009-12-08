// Copyright (c) 2009 The Chromium Authors. All rights reserved.  Use of this
// source code is governed by a BSD-style license that can be found in the
// LICENSE file.

#include "net/ftp/ftp_directory_listing_parser_ls.h"

#include <vector>

#include "base/string_util.h"
#include "net/ftp/ftp_util.h"

namespace {

bool LooksLikeUnixPermission(const string16& text) {
  if (text.length() != 3)
    return false;

  // Meaning of the flags:
  // r - file is readable
  // w - file is writable
  // x - file is executable
  // s or S - setuid/setgid bit set
  // t or T - "sticky" bit set
  return ((text[0] == 'r' || text[0] == '-') &&
          (text[1] == 'w' || text[1] == '-') &&
          (text[2] == 'x' || text[2] == 's' || text[2] == 'S' ||
           text[2] == 't' || text[2] == 'T' || text[2] == '-'));
}

bool LooksLikeUnixPermissionsListing(const string16& text) {
  if (text.length() != 10)
    return false;

  if (text[0] != 'b' && text[0] != 'c' && text[0] != 'd' &&
      text[0] != 'l' && text[0] != 'p' && text[0] != 's' &&
      text[0] != '-')
    return false;

  return (LooksLikeUnixPermission(text.substr(1, 3)) &&
          LooksLikeUnixPermission(text.substr(4, 3)) &&
          LooksLikeUnixPermission(text.substr(7, 3)));
}

string16 GetStringPartAfterColumns(const string16& text, int columns) {
  DCHECK_LE(1, columns);
  int columns_so_far = 0;
  size_t last = 0;
  for (size_t i = 1; i < text.length(); ++i) {
    if (!isspace(text[i - 1]) && isspace(text[i])) {
      last = i;
      if (++columns_so_far == columns)
        break;
    }
  }
  string16 result(text.substr(last));
  TrimWhitespace(result, TRIM_ALL, &result);
  return result;
}

bool DetectColumnOffset(const std::vector<string16>& columns, int* offset) {
  base::Time time;

  if (columns.size() >= 8 &&
      net::FtpUtil::LsDateListingToTime(columns[5], columns[6], columns[7],
                                        &time)) {
    // Standard listing, exactly like ls -l.
    *offset = 1;
    return true;
  }

  if (columns.size() >= 7 &&
      net::FtpUtil::LsDateListingToTime(columns[4], columns[5], columns[6],
                                        &time)) {
    // wu-ftpd listing, no "number of links" column.
    *offset = 0;
    return true;
  }

  // Unrecognized listing style.
  return false;
}

}  // namespace

namespace net {

FtpDirectoryListingParserLs::FtpDirectoryListingParserLs()
    : received_nonempty_line_(false),
      received_total_line_(false),
      column_offset_(-1) {
}

bool FtpDirectoryListingParserLs::ConsumeLine(const string16& line) {
  std::vector<string16> columns;
  SplitString(CollapseWhitespace(line, false), ' ', &columns);

  if (line.empty() && !received_nonempty_line_) {
    // Allow empty lines only at the beginning of the listing. For example VMS
    // systems in Unix emulation mode add an empty line before the first listing
    // entry.
    return true;
  }
  // Some FTP servers put a "total n" line at the beginning of the listing
  // (n is an integer). Allow such a line, but only once, and only if it's
  // the first non-empty line. Do not match the word exactly, because it may be
  // in different languages (at least English and German have been seen in the
  // field).
  if (columns.size() == 2 && !received_total_line_) {
    received_total_line_ = true;

    int total_number;
    if (!StringToInt(columns[1], &total_number))
      return false;
    if (total_number < 0)
      return false;

    return true;
  }
  if (!received_nonempty_line_ && !DetectColumnOffset(columns, &column_offset_))
    return false;
  received_nonempty_line_ = true;

  // We may receive file names containing spaces, which can make the number of
  // columns arbitrarily large. We will handle that later. For now just make
  // sure we have all the columns that should normally be there.
  if (columns.size() < 8U + column_offset_)
    return false;

  if (!LooksLikeUnixPermissionsListing(columns[0]))
    return false;

  FtpDirectoryListingEntry entry;
  if (columns[0][0] == 'l') {
    entry.type = FtpDirectoryListingEntry::SYMLINK;
  } else if (columns[0][0] == 'd') {
    entry.type = FtpDirectoryListingEntry::DIRECTORY;
  } else {
    entry.type = FtpDirectoryListingEntry::FILE;
  }

  if (!StringToInt64(columns[3 + column_offset_], &entry.size))
    return false;
  if (entry.size < 0)
    return false;
  if (entry.type != FtpDirectoryListingEntry::FILE)
    entry.size = -1;

  if (!FtpUtil::LsDateListingToTime(columns[4 + column_offset_],
                                    columns[5 + column_offset_],
                                    columns[6 + column_offset_],
                                    &entry.last_modified)) {
    return false;
  }

  entry.name = GetStringPartAfterColumns(line, 7 + column_offset_);
  if (entry.type == FtpDirectoryListingEntry::SYMLINK) {
    string16::size_type pos = entry.name.rfind(ASCIIToUTF16(" -> "));
    if (pos == string16::npos)
      return false;
    entry.name = entry.name.substr(0, pos);
  }

  entries_.push(entry);
  return true;
}

bool FtpDirectoryListingParserLs::OnEndOfInput() {
  return true;
}

bool FtpDirectoryListingParserLs::EntryAvailable() const {
  return !entries_.empty();
}

FtpDirectoryListingEntry FtpDirectoryListingParserLs::PopEntry() {
  FtpDirectoryListingEntry entry = entries_.front();
  entries_.pop();
  return entry;
}

}  // namespace net
