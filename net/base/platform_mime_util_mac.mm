// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
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

#include <CoreServices/CoreServices.h>
#include <string>

#include "base/scoped_cftyperef.h"
#include "base/sys_string_conversions.h"
#include "net/base/platform_mime_util.h"

namespace net {

bool PlatformMimeUtil::GetPlatformMimeTypeFromExtension(
    const std::wstring& ext, std::string* result) const {
  std::wstring ext_nodot = ext;
  if (ext_nodot.length() >= 1 && ext_nodot[0] == L'.')
    ext_nodot.erase(ext_nodot.begin());
  scoped_cftyperef<CFStringRef> ext_ref(base::SysWideToCFStringRef(ext_nodot));
  if (!ext_ref)
    return false;
  scoped_cftyperef<CFStringRef> uti(
      UTTypeCreatePreferredIdentifierForTag(kUTTagClassFilenameExtension,
                                            ext_ref,
                                            NULL));
  if (!uti)
    return false;
  scoped_cftyperef<CFStringRef> mime_ref(
      UTTypeCopyPreferredTagWithClass(uti, kUTTagClassMIMEType));
  if (!mime_ref)
    return false;
  
  *result = base::SysCFStringRefToUTF8(mime_ref);
  return true;
}

bool PlatformMimeUtil::GetPreferredExtensionForMimeType(
    const std::string& mime_type, std::wstring* ext) const {
  scoped_cftyperef<CFStringRef> mime_ref(base::SysUTF8ToCFStringRef(mime_type));
  if (!mime_ref)
    return false;
  scoped_cftyperef<CFStringRef> uti(
      UTTypeCreatePreferredIdentifierForTag(kUTTagClassMIMEType,
                                            mime_ref,
                                            NULL));
  if (!uti)
    return false;
  scoped_cftyperef<CFStringRef> ext_ref(
      UTTypeCopyPreferredTagWithClass(uti, kUTTagClassFilenameExtension));
  if (!ext_ref)
    return false;
  
  ext_ref.reset(CFStringCreateWithFormat(kCFAllocatorDefault,
                                         NULL,
                                         CFSTR(".%@"),
                                         ext_ref.get()));
  
  *ext = base::SysCFStringRefToWide(ext_ref);
  return true;
}

}  // namespace net
