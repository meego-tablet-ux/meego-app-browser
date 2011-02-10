// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_FILEAPI_FILE_SYSTEM_PATH_MANAGER_H_
#define WEBKIT_FILEAPI_FILE_SYSTEM_PATH_MANAGER_H_

#include "base/callback.h"
#include "base/file_path.h"
#include "base/scoped_ptr.h"
#include "webkit/fileapi/file_system_types.h"

class GURL;

namespace base {
class MessageLoopProxy;
}

namespace fileapi {

// An interface to construct or crack sandboxed filesystem paths.
// Currently each sandboxed filesystem path looks like:
//
//   <profile_dir>/FileSystem/<origin_identifier>/<type>/chrome-<unique>/...
//
// where <type> is either one of "Temporary" or "Persistent".
class FileSystemPathManager {
 public:
  FileSystemPathManager(scoped_refptr<base::MessageLoopProxy> file_message_loop,
                        const FilePath& profile_path,
                        bool is_incognito,
                        bool allow_file_access_from_files);
  ~FileSystemPathManager();

  // Callback for GetFileSystemRootPath.
  // If the request is accepted and the root filesystem for the origin exists
  // the callback is called with success=true and valid root_path and name.
  // If the request is accepted, |create| is specified for
  // GetFileSystemRootPath, and the root directory does not exist, it creates
  // a new one and calls back with success=true if the creation has succeeded.
  typedef Callback3<bool /* success */,
                    const FilePath& /* root_path */,
                    const std::string& /* name */>::Type GetRootPathCallback;

  // Retrieves the root path for the given |origin_url| and |type|, and
  // calls the given |callback| with the root path and name.
  // If |create| is true this also creates the directory if it doesn't exist.
  void GetFileSystemRootPath(const GURL& origin_url,
                             fileapi::FileSystemType type,
                             bool create,
                             GetRootPathCallback* callback);

  // Cracks the given |path|, retrieves the information embedded in the path
  // and populates |origin_url| and |type|.  Also it populates |virtual_path|
  // that is a sandboxed path in the file system, i.e. the relative path to
  // the file system's root path for the given origin and type.
  // It returns false if the path does not conform to the expected
  // filesystem path format.
  bool CrackFileSystemPath(const FilePath& path,
                           GURL* origin_url,
                           FileSystemType* type,
                           FilePath* virtual_path) const;

  // Checks if a given |name| contains any restricted names/chars in it.
  bool IsRestrictedFileName(const FilePath& filename) const;

  // Returns true if the given |url|'s scheme is allowed to access
  // filesystem.
  bool IsAllowedScheme(const GURL& url) const;

  // The FileSystem directory name.
  static const FilePath::CharType kFileSystemDirectory[];

  static const char kPersistentName[];
  static const char kTemporaryName[];

  const FilePath& base_path() const {
    return base_path_;
  }

  // Returns the filesystem name string for the given |origin_url| and |type|.
  static std::string GetFileSystemName(const GURL& url,
                                       fileapi::FileSystemType type);

  // Returns the storage identifier string for the given |url|.
  static std::string GetStorageIdentifierFromURL(const GURL& url);

  // Gets a base directory path of the sandboxed filesystem that is
  // specified by |origin_url| and |type|.
  // |base_path| must be pointing the FileSystem's data directory
  // under the profile directory, i.e. <profile_dir>/kFileSystemDirectory.
  // Returns an empty path if any of the given parameters are invalid.
  // Returned directory path does not contain 'unique' part, therefore
  // it is not an actural root path for the filesystem.
  static FilePath GetFileSystemBaseDirectoryForOriginAndType(
      const FilePath& base_path,
      const GURL& origin_url,
      fileapi::FileSystemType type);

 private:
  class GetFileSystemRootPathTask;

  scoped_refptr<base::MessageLoopProxy> file_message_loop_;

  const FilePath base_path_;
  const bool is_incognito_;
  const bool allow_file_access_from_files_;

  DISALLOW_COPY_AND_ASSIGN(FileSystemPathManager);
};

}  // namespace fileapi

#endif  // WEBKIT_FILEAPI_FILE_SYSTEM_PATH_MANAGER_H_
