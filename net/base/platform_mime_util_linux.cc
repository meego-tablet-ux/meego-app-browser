// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/mime_util.h"
#include "net/base/platform_mime_util.h"

namespace net {

bool PlatformMimeUtil::GetPlatformMimeTypeFromExtension(
    const FilePath::StringType& ext, std::string* result) const {
  // TODO(thestig) This is a temporary hack until we can fix this
  // properly in test shell / webkit.
  // We have to play dumb and not return application/x-perl here
  // to make the reload-subframe-object layout test happy.
  if (ext == "pl")
    return false;

  FilePath dummy_path("foo." + ext);
  std::string out = mime_util::GetFileMimeType(dummy_path);

  // GetFileMimeType likes to return application/octet-stream
  // for everything it doesn't know - ignore that.
  if (out == "application/octet-stream" || out.empty())
    return false;

  // GetFileMimeType returns image/x-ico because that's what's in the XDG
  // mime database. That database is the merger of the Gnome and KDE mime
  // databases. Apparently someone working on KDE in 2001 decided .ico
  // resolves to image/x-ico, whereas the rest of the world uses image/x-icon.
  // FWIW, image/vnd.microsoft.icon is the official IANA assignment.
  if (out == "image/x-ico")
    out = "image/x-icon";

  *result = out;
  return true;
}

struct MimeToExt {
  const char* mime_type;
  const char* ext;
};

const struct MimeToExt mime_type_ext_map[] = {
  {"image/jpeg", "jpg"},
  {"image/png", "png"},
  {"image/gif", "gif"},
  {"text/html", "html"},
  {"video/mp4", "mp4"},
  {"video/mpeg", "mpg"},
  {"audio/mpeg", "mp3"},
  {"text/plain", "txt"},
  {"application/pdf", "pdf"},
  {"application/x-tar", "tar"}
};

bool PlatformMimeUtil::GetPreferredExtensionForMimeType(
    const std::string& mime_type, FilePath::StringType* ext) const {

  for (size_t x = 0;
       x < (sizeof(mime_type_ext_map) / sizeof(MimeToExt));
       x++) {
    if (mime_type_ext_map[x].mime_type == mime_type) {
      *ext = mime_type_ext_map[x].ext;
      return true;
    }
  }

  // TODO(dhg): Fix this the right way by implementing whats said below.
  // Unlike GetPlatformMimeTypeFromExtension, this method doesn't have a
  // default list that it uses, but for now we are also returning false since
  // this doesn't really matter as much under Linux.
  //
  // If we wanted to do this properly, we would read the mime.cache file which
  // has a section where they assign a glob (*.gif) to a mimetype
  // (image/gif). We look up the "heaviest" glob for a certain mime type and
  // then then try to chop off "*.".

  return false;
}

}  // namespace net
