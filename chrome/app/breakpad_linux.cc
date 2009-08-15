// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/breakpad_linux.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <string>

#include "base/command_line.h"
#include "base/eintr_wrapper.h"
#include "base/file_version_info_linux.h"
#include "base/format_macros.h"
#include "base/global_descriptors_posix.h"
#include "base/json_writer.h"
#include "base/linux_util.h"
#include "base/path_service.h"
#include "base/rand_util.h"
#include "base/scoped_fd.h"
#include "base/string_util.h"
#include "base/values.h"
#include "breakpad/linux/directory_reader.h"
#include "breakpad/linux/exception_handler.h"
#include "breakpad/linux/linux_libc_support.h"
#include "breakpad/linux/linux_syscall_support.h"
#include "breakpad/linux/memory.h"
#include "chrome/common/chrome_descriptors.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/installer/util/google_update_settings.h"

static const char kUploadURL[] =
    "https://clients2.google.com/cr/report";

// Writes the value |v| as 16 hex characters to the memory pointed at by
// |output|.
static void write_uint64_hex(char* output, uint64_t v) {
  static const char hextable[] = "0123456789abcdef";

  for (int i = 15; i >= 0; --i) {
    output[i] = hextable[v & 15];
    v >>= 4;
  }
}

pid_t UploadCrashDump(const BreakpadInfo& info) {
  // WARNING: this code runs in a compromised context. It may not call into
  // libc nor allocate memory normally.

  const int dumpfd = sys_open(info.filename, O_RDONLY, 0);
  if (dumpfd < 0) {
    static const char msg[] = "Cannot upload crash dump: failed to open\n";
    sys_write(2, msg, sizeof(msg));
    return -1;
  }
  struct kernel_stat st;
  if (sys_fstat(dumpfd, &st) != 0) {
    static const char msg[] = "Cannot upload crash dump: stat failed\n";
    sys_write(2, msg, sizeof(msg));
    sys_close(dumpfd);
    return -1;
  }

  google_breakpad::PageAllocator allocator;

  uint8_t* dump_data = reinterpret_cast<uint8_t*>(allocator.Alloc(st.st_size));
  if (!dump_data) {
    static const char msg[] = "Cannot upload crash dump: cannot alloc\n";
    sys_write(2, msg, sizeof(msg));
    sys_close(dumpfd);
    return -1;
  }

  sys_read(dumpfd, dump_data, st.st_size);
  sys_close(dumpfd);

  // We need to build a MIME block for uploading to the server. Since we are
  // going to fork and run wget, it needs to be written to a temp file.

  const int ufd = sys_open("/dev/urandom", O_RDONLY, 0);
  if (ufd < 0) {
    static const char msg[] = "Cannot upload crash dump because /dev/urandom"
                              " is missing\n";
    sys_write(2, msg, sizeof(msg) - 1);
    return -1;
  }

  static const char temp_file_template[] =
      "/tmp/chromium-upload-XXXXXXXXXXXXXXXX";
  char buf[sizeof(temp_file_template)];
  memcpy(buf, temp_file_template, sizeof(temp_file_template));

  int fd = -1;
  for (unsigned i = 0; i < 10; ++i) {
    uint64_t t;
    read(ufd, &t, sizeof(t));
    write_uint64_hex(buf + sizeof(buf) - (16 + 1), t);

    fd = sys_open(buf, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd >= 0)
      break;
  }

  if (fd == -1) {
    static const char msg[] = "Failed to create temporary file in /tmp: cannot "
                              "upload crash dump\n";
    sys_write(2, msg, sizeof(msg) - 1);
    sys_close(ufd);
    return -1;
  }

  // The MIME boundary is 28 hypens, followed by a 64-bit nonce and a NUL.
  char mime_boundary[28 + 16 + 1];
  my_memset(mime_boundary, '-', 28);
  uint64_t boundary_rand;
  sys_read(ufd, &boundary_rand, sizeof(boundary_rand));
  write_uint64_hex(mime_boundary + 28, boundary_rand);
  mime_boundary[28 + 16] = 0;
  sys_close(ufd);

  // The define for the product version is a wide string, so we need to
  // downconvert it.
  static const wchar_t version[] = PRODUCT_VERSION;
  static const unsigned version_len = sizeof(version) / sizeof(wchar_t);
  char version_msg[version_len];
  for (unsigned i = 0; i < version_len; ++i)
    version_msg[i] = static_cast<char>(version[i]);

  // The MIME block looks like this:
  //   BOUNDARY \r\n (0, 1)
  //   Content-Disposition: form-data; name="prod" \r\n \r\n (2..6)
  //   Chrome_Linux \r\n (7, 8)
  //   BOUNDARY \r\n (9, 10)
  //   Content-Disposition: form-data; name="ver" \r\n \r\n (11..15)
  //   1.2.3.4 \r\n (16, 17)
  //   BOUNDARY \r\n (18, 19)
  //   Content-Disposition: form-data; name="guid" \r\n \r\n (20..24)
  //   1.2.3.4 \r\n (25, 26)
  //   BOUNDARY \r\n (27, 28)
  //
  //   zero or more:
  //   Content-Disposition: form-data; name="ptype" \r\n \r\n (0..4)
  //   abcdef \r\n (5, 6)
  //   BOUNDARY \r\n (7, 8)
  //
  //   zero or more:
  //   Content-Disposition: form-data; name="lsb-release" \r\n \r\n (0..4)
  //   abcdef \r\n (5, 6)
  //   BOUNDARY \r\n (7, 8)
  //
  //   zero or more:
  //   Content-Disposition: form-data; name="url-chunk-1" \r\n \r\n (0..5)
  //   abcdef \r\n (6, 7)
  //   BOUNDARY \r\n (8, 9)
  //
  //   Content-Disposition: form-data; name="dump"; filename="dump" \r\n (0,1,2)
  //   Content-Type: application/octet-stream \r\n \r\n (3,4,5)
  //   <dump contents> (6)
  //   \r\n BOUNDARY -- \r\n (7,8,9,10)

  static const char rn[] = {'\r', '\n'};
  static const char form_data_msg[] = "Content-Disposition: form-data; name=\"";
  static const char prod_msg[] = "prod";
  static const char quote_msg[] = {'"'};
  static const char chrome_linux_msg[] = "Chrome_Linux";
  static const char ver_msg[] = "ver";
  static const char guid_msg[] = "guid";
  static const char dashdash_msg[] = {'-', '-'};
  static const char dump_msg[] = "upload_file_minidump\"; filename=\"dump\"";
  static const char content_type_msg[] =
      "Content-Type: application/octet-stream";
  static const char url_chunk_msg[] = "url-chunk-";
  static const char process_type_msg[] = "ptype";
  static const char distro_msg[] = "lsb-release";

  struct kernel_iovec iov[29];
  iov[0].iov_base = mime_boundary;
  iov[0].iov_len = sizeof(mime_boundary) - 1;
  iov[1].iov_base = const_cast<char*>(rn);
  iov[1].iov_len = sizeof(rn);

  iov[2].iov_base = const_cast<char*>(form_data_msg);
  iov[2].iov_len = sizeof(form_data_msg) - 1;
  iov[3].iov_base = const_cast<char*>(prod_msg);
  iov[3].iov_len = sizeof(prod_msg) - 1;
  iov[4].iov_base = const_cast<char*>(quote_msg);
  iov[4].iov_len = sizeof(quote_msg);
  iov[5].iov_base = const_cast<char*>(rn);
  iov[5].iov_len = sizeof(rn);
  iov[6].iov_base = const_cast<char*>(rn);
  iov[6].iov_len = sizeof(rn);

  iov[7].iov_base = const_cast<char*>(chrome_linux_msg);
  iov[7].iov_len = sizeof(chrome_linux_msg) - 1;
  iov[8].iov_base = const_cast<char*>(rn);
  iov[8].iov_len = sizeof(rn);

  iov[9].iov_base = mime_boundary;
  iov[9].iov_len = sizeof(mime_boundary) - 1;
  iov[10].iov_base = const_cast<char*>(rn);
  iov[10].iov_len = sizeof(rn);

  iov[11].iov_base = const_cast<char*>(form_data_msg);
  iov[11].iov_len = sizeof(form_data_msg) - 1;
  iov[12].iov_base = const_cast<char*>(ver_msg);
  iov[12].iov_len = sizeof(ver_msg) - 1;
  iov[13].iov_base = const_cast<char*>(quote_msg);
  iov[13].iov_len = sizeof(quote_msg);
  iov[14].iov_base = const_cast<char*>(rn);
  iov[14].iov_len = sizeof(rn);
  iov[15].iov_base = const_cast<char*>(rn);
  iov[15].iov_len = sizeof(rn);

  iov[16].iov_base = const_cast<char*>(version_msg);
  iov[16].iov_len = sizeof(version_msg) - 1;
  iov[17].iov_base = const_cast<char*>(rn);
  iov[17].iov_len = sizeof(rn);

  iov[18].iov_base = mime_boundary;
  iov[18].iov_len = sizeof(mime_boundary) - 1;
  iov[19].iov_base = const_cast<char*>(rn);
  iov[19].iov_len = sizeof(rn);

  iov[20].iov_base = const_cast<char*>(form_data_msg);
  iov[20].iov_len = sizeof(form_data_msg) - 1;
  iov[21].iov_base = const_cast<char*>(guid_msg);
  iov[21].iov_len = sizeof(guid_msg) - 1;
  iov[22].iov_base = const_cast<char*>(quote_msg);
  iov[22].iov_len = sizeof(quote_msg);
  iov[23].iov_base = const_cast<char*>(rn);
  iov[23].iov_len = sizeof(rn);
  iov[24].iov_base = const_cast<char*>(rn);
  iov[24].iov_len = sizeof(rn);

  iov[25].iov_base = const_cast<char*>(info.guid);
  iov[25].iov_len = info.guid_length;
  iov[26].iov_base = const_cast<char*>(rn);
  iov[26].iov_len = sizeof(rn);

  iov[27].iov_base = mime_boundary;
  iov[27].iov_len = sizeof(mime_boundary) - 1;
  iov[28].iov_base = const_cast<char*>(rn);
  iov[28].iov_len = sizeof(rn);

  sys_writev(fd, iov, 29);

  if (info.process_type_length) {
    iov[0].iov_base = const_cast<char*>(form_data_msg);
    iov[0].iov_len = sizeof(form_data_msg) - 1;
    iov[1].iov_base = const_cast<char*>(process_type_msg);
    iov[1].iov_len = sizeof(process_type_msg) - 1;
    iov[2].iov_base = const_cast<char*>(quote_msg);
    iov[2].iov_len = sizeof(quote_msg);
    iov[3].iov_base = const_cast<char*>(rn);
    iov[3].iov_len = sizeof(rn);
    iov[4].iov_base = const_cast<char*>(rn);
    iov[4].iov_len = sizeof(rn);

    iov[5].iov_base = const_cast<char*>(info.process_type);
    iov[5].iov_len = info.process_type_length;
    iov[6].iov_base = const_cast<char*>(rn);
    iov[6].iov_len = sizeof(rn);
    iov[7].iov_base = mime_boundary;
    iov[7].iov_len = sizeof(mime_boundary) - 1;
    iov[8].iov_base = const_cast<char*>(rn);
    iov[8].iov_len = sizeof(rn);

    sys_writev(fd, iov, 9);
  }

  if (info.distro_length) {
    iov[0].iov_base = const_cast<char*>(form_data_msg);
    iov[0].iov_len = sizeof(form_data_msg) - 1;
    iov[1].iov_base = const_cast<char*>(distro_msg);
    iov[1].iov_len = sizeof(distro_msg) - 1;
    iov[2].iov_base = const_cast<char*>(quote_msg);
    iov[2].iov_len = sizeof(quote_msg);
    iov[3].iov_base = const_cast<char*>(rn);
    iov[3].iov_len = sizeof(rn);
    iov[4].iov_base = const_cast<char*>(rn);
    iov[4].iov_len = sizeof(rn);

    iov[5].iov_base = const_cast<char*>(info.distro);
    iov[5].iov_len = info.distro_length;
    iov[6].iov_base = const_cast<char*>(rn);
    iov[6].iov_len = sizeof(rn);
    iov[7].iov_base = mime_boundary;
    iov[7].iov_len = sizeof(mime_boundary) - 1;
    iov[8].iov_base = const_cast<char*>(rn);
    iov[8].iov_len = sizeof(rn);

    sys_writev(fd, iov, 9);
  }

  if (info.crash_url_length) {
    unsigned i = 0, done = 0, crash_url_length = info.crash_url_length;
    static const unsigned kMaxCrashChunkSize = 64;
    static const unsigned kMaxUrlLength = 8 * kMaxCrashChunkSize;
    if (crash_url_length > kMaxUrlLength)
      crash_url_length = kMaxUrlLength;

    while (crash_url_length) {
      char num[16];
      const unsigned num_len = my_int_len(++i);
      my_itos(num, i, num_len);

      iov[0].iov_base = const_cast<char*>(form_data_msg);
      iov[0].iov_len = sizeof(form_data_msg) - 1;
      iov[1].iov_base = const_cast<char*>(url_chunk_msg);
      iov[1].iov_len = sizeof(url_chunk_msg) - 1;
      iov[2].iov_base = num;
      iov[2].iov_len = num_len;
      iov[3].iov_base = const_cast<char*>(quote_msg);
      iov[3].iov_len = sizeof(quote_msg);
      iov[4].iov_base = const_cast<char*>(rn);
      iov[4].iov_len = sizeof(rn);
      iov[5].iov_base = const_cast<char*>(rn);
      iov[5].iov_len = sizeof(rn);

      const unsigned len = crash_url_length > kMaxCrashChunkSize ?
                           kMaxCrashChunkSize : crash_url_length;
      iov[6].iov_base = const_cast<char*>(info.crash_url + done);
      iov[6].iov_len = len;
      iov[7].iov_base = const_cast<char*>(rn);
      iov[7].iov_len = sizeof(rn);
      iov[8].iov_base = mime_boundary;
      iov[8].iov_len = sizeof(mime_boundary) - 1;
      iov[9].iov_base = const_cast<char*>(rn);
      iov[9].iov_len = sizeof(rn);

      sys_writev(fd, iov, 10);

      done += len;
      crash_url_length -= len;
    }
  }

  iov[0].iov_base = const_cast<char*>(form_data_msg);
  iov[0].iov_len = sizeof(form_data_msg) - 1;
  iov[1].iov_base = const_cast<char*>(dump_msg);
  iov[1].iov_len = sizeof(dump_msg) - 1;
  iov[2].iov_base = const_cast<char*>(rn);
  iov[2].iov_len = sizeof(rn);

  iov[3].iov_base = const_cast<char*>(content_type_msg);
  iov[3].iov_len = sizeof(content_type_msg) - 1;
  iov[4].iov_base = const_cast<char*>(rn);
  iov[4].iov_len = sizeof(rn);
  iov[5].iov_base = const_cast<char*>(rn);
  iov[5].iov_len = sizeof(rn);

  iov[6].iov_base = dump_data;
  iov[6].iov_len = st.st_size;

  iov[7].iov_base = const_cast<char*>(rn);
  iov[7].iov_len = sizeof(rn);
  iov[8].iov_base = mime_boundary;
  iov[8].iov_len = sizeof(mime_boundary) - 1;
  iov[9].iov_base = const_cast<char*>(dashdash_msg);
  iov[9].iov_len = sizeof(dashdash_msg);
  iov[10].iov_base = const_cast<char*>(rn);
  iov[10].iov_len = sizeof(rn);

  sys_writev(fd, iov, 11);

  sys_close(fd);

  // The --header argument to wget looks like:
  //   --header=Content-Type: multipart/form-data; boundary=XYZ
  // where the boundary has two fewer leading '-' chars
  static const char header_msg[] =
      "--header=Content-Type: multipart/form-data; boundary=";
  char* const header = reinterpret_cast<char*>(allocator.Alloc(
      sizeof(header_msg) - 1 + sizeof(mime_boundary) - 2));
  memcpy(header, header_msg, sizeof(header_msg) - 1);
  memcpy(header + sizeof(header_msg) - 1, mime_boundary + 2,
         sizeof(mime_boundary) - 2);
  // We grab the NUL byte from the end of |mime_boundary|.

  // The --post-file argument to wget looks like:
  //   --post-file=/tmp/...
  static const char post_file_msg[] = "--post-file=";
  char* const post_file = reinterpret_cast<char*>(allocator.Alloc(
       sizeof(post_file_msg) - 1 + sizeof(buf)));
  memcpy(post_file, post_file_msg, sizeof(post_file_msg) - 1);
  memcpy(post_file + sizeof(post_file_msg) - 1, buf, sizeof(buf));

  const pid_t child = sys_fork();
  if (!child) {
    // This code is called both when a browser is crashing (in which case,
    // nothing really matters any more) and when a renderer crashes, in which
    // case we need to continue.
    //
    // Since we are a multithreaded app, if we were just to fork(), we might
    // grab file descriptors which have just been created in another thread and
    // hold them open for too long.
    //
    // Thus, we have to loop and try and close everything.
    const int fd = sys_open("/proc/self/fd", O_DIRECTORY | O_RDONLY, 0);
    if (fd < 0) {
      for (unsigned i = 3; i < 8192; ++i)
        sys_close(i);
    } else {
      google_breakpad::DirectoryReader reader(fd);
      const char* name;
      while (reader.GetNextEntry(&name)) {
        int i;
        if (my_strtoui(&i, name) && i > 2 && i != fd)
          sys_close(fd);
        reader.PopEntry();
      }

      sys_close(fd);
    }

    sys_setsid();

    // Leave one end of a pipe in the wget process and watch for it getting
    // closed by the wget process exiting.
    int fds[2];
    sys_pipe(fds);

    const pid_t child = sys_fork();
    if (child) {
      sys_close(fds[1]);
      char id_buf[17];
      const int len = HANDLE_EINTR(read(fds[0], id_buf, sizeof(id_buf) - 1));
      if (len > 0) {
        id_buf[len] = 0;
        static const char msg[] = "\nCrash dump id: ";
        sys_write(2, msg, sizeof(msg) - 1);
        sys_write(2, id_buf, my_strlen(id_buf));
        sys_write(2, "\n", 1);
      }
      sys_unlink(info.filename);
      sys_unlink(buf);
      sys__exit(0);
    }

    sys_close(fds[0]);
    sys_dup2(fds[1], 3);
    static const char* const kWgetBinary = "/usr/bin/wget";
    const char* args[] = {
      kWgetBinary,
      header,
      post_file,
      kUploadURL,
      "-O",  // output reply to fd 3
      "/dev/fd/3",
      NULL,
    };

    execv("/usr/bin/wget", const_cast<char**>(args));
    static const char msg[] = "Cannot upload crash dump: cannot exec "
                              "/usr/bin/wget\n";
    sys_write(2, msg, sizeof(msg) - 1);
    sys__exit(1);
  }

  return child;
}

// This is defined in chrome/browser/google_update_settings_linux.cc, it's the
// static string containing the user's unique GUID. We send this in the crash
// report.
namespace google_update {
extern std::string linux_guid;
}

// This is defined in base/linux_util.cc, it's the static string containing the
// user's distro info. We send this in the crash report.
namespace base {
extern std::string linux_distro;
}

static bool CrashDone(const char* dump_path,
                      const char* minidump_id,
                      void* context,
                      bool succeeded) {
  // WARNING: this code runs in a compromised context. It may not call into
  // libc nor allocate memory normally.
  if (!succeeded)
    return false;

  google_breakpad::PageAllocator allocator;
  const unsigned dump_path_len = my_strlen(dump_path);
  const unsigned minidump_id_len = my_strlen(minidump_id);
  char *const path = reinterpret_cast<char*>(allocator.Alloc(
      dump_path_len + 1 /* '/' */ + minidump_id_len +
      4 /* ".dmp" */ + 1 /* NUL */));
  memcpy(path, dump_path, dump_path_len);
  path[dump_path_len] = '/';
  memcpy(path + dump_path_len + 1, minidump_id, minidump_id_len);
  memcpy(path + dump_path_len + 1 + minidump_id_len, ".dmp", 4);
  path[dump_path_len + 1 + minidump_id_len + 4] = 0;

  BreakpadInfo info;
  info.filename = path;
  info.process_type = "browser";
  info.process_type_length = 7;
  info.crash_url = NULL;
  info.crash_url_length = 0;
  info.guid = google_update::linux_guid.data();
  info.guid_length = google_update::linux_guid.length();
  info.distro = base::linux_distro.data();
  info.distro_length = base::linux_distro.length();
  UploadCrashDump(info);

  return true;
}

void EnableCrashDumping() {
  // We leak this object.

  new google_breakpad::ExceptionHandler("/tmp", NULL, CrashDone, NULL,
                                        true /* install handlers */);
}

// This is defined in chrome/common/child_process_logging_linux.cc, it's the
// static string containing the current active URL. We send this in the crash
// report.
namespace child_process_logging {
extern std::string active_url;
}

static bool
RendererCrashHandler(const void* crash_context, size_t crash_context_size,
             void* context) {
  const int fd = reinterpret_cast<intptr_t>(context);
  int fds[2];
  socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
  char guid[kGuidSize] = {0};
  char crash_url[kMaxActiveURLSize + 1] = {0};
  char distro[kDistroSize + 1] = {0};
  const size_t guid_len = std::min(google_update::linux_guid.size(),
                                   kGuidSize);
  const size_t crash_url_len =
      std::min(child_process_logging::active_url.size(), kMaxActiveURLSize);
  const size_t distro_len =
      std::min(base::linux_distro.size(), kDistroSize);
  memcpy(guid, google_update::linux_guid.data(), guid_len);
  memcpy(crash_url, child_process_logging::active_url.data(), crash_url_len);
  memcpy(distro, base::linux_distro.data(), distro_len);

  // The length of the control message:
  static const unsigned kControlMsgSize = CMSG_SPACE(sizeof(int));

  struct kernel_msghdr msg;
  my_memset(&msg, 0, sizeof(struct kernel_msghdr));
  struct kernel_iovec iov[4];
  iov[0].iov_base = const_cast<void*>(crash_context);
  iov[0].iov_len = crash_context_size;
  iov[1].iov_base = guid;
  iov[1].iov_len = kGuidSize + 1;
  iov[2].iov_base = crash_url;
  iov[2].iov_len = kMaxActiveURLSize + 1;
  iov[3].iov_base = distro;
  iov[3].iov_len = kDistroSize + 1;

  msg.msg_iov = iov;
  msg.msg_iovlen = 4;
  char cmsg[kControlMsgSize];
  memset(cmsg, 0, kControlMsgSize);
  msg.msg_control = cmsg;
  msg.msg_controllen = sizeof(cmsg);

  struct cmsghdr *hdr = CMSG_FIRSTHDR(&msg);
  hdr->cmsg_level = SOL_SOCKET;
  hdr->cmsg_type = SCM_RIGHTS;
  hdr->cmsg_len = CMSG_LEN(sizeof(int));
  *((int*) CMSG_DATA(hdr)) = fds[1];

  HANDLE_EINTR(sys_sendmsg(fd, &msg, 0));
  sys_close(fds[1]);

  char b;
  HANDLE_EINTR(sys_read(fds[0], &b, 1));

  return true;
}

void EnableRendererCrashDumping() {
  const int fd = Singleton<base::GlobalDescriptors>()->Get(kCrashDumpSignal);
  // We deliberately leak this object.
  google_breakpad::ExceptionHandler* handler =
      new google_breakpad::ExceptionHandler("" /* unused */, NULL, NULL,
                                            (void*) fd, true);
  handler->set_crash_handler(RendererCrashHandler);
}

void InitCrashReporter() {
  // Determine the process type and take appropriate action.
  const CommandLine& parsed_command_line = *CommandLine::ForCurrentProcess();
  const std::wstring process_type =
      parsed_command_line.GetSwitchValue(switches::kProcessType);
  if (process_type.empty()) {
    if (!GoogleUpdateSettings::GetCollectStatsConsent())
      return;
    base::GetLinuxDistro();  // Initialize base::linux_distro if needed.
    EnableCrashDumping();
  } else if (process_type == switches::kRendererProcess ||
             process_type == switches::kZygoteProcess) {
    // We might be chrooted in a zygote or renderer process so we cannot call
    // GetCollectStatsConsent because that needs access the the user's home
    // dir. Instead, we set a command line flag for these processes.
    if (!parsed_command_line.HasSwitch(switches::kEnableCrashReporter))
      return;
    // Get the guid and linux distro from the command line switch.
    std::string switch_value = WideToASCII(
        parsed_command_line.GetSwitchValue(switches::kEnableCrashReporter));
    size_t separator = switch_value.find(",");
    if (separator != std::string::npos) {
      google_update::linux_guid = switch_value.substr(0, separator);
      base::linux_distro = switch_value.substr(separator + 1);
    } else {
      google_update::linux_guid = switch_value;
    }
    EnableRendererCrashDumping();
  }
}

// -----------------------------------------------------------------------------

bool EnableCoreDumping(std::string* core_dump_directory) {
  // First we check that the core files will get dumped to the
  // current-directory in a file called 'core'. We could try to support other
  // setups by simulating the kernel's code, but it's extra complexity and we
  // only intend for this code to be run internally.
  static const char kCorePatternFd[] = "/proc/sys/kernel/core_pattern";

  ScopedFd core_pattern_fd(open(kCorePatternFd, O_RDONLY));
  if (core_pattern_fd.get() < 0) {
    LOG(WARNING) << "Cannot open " << kCorePatternFd << ": " << strerror(errno);
    return false;
  }

  char buf[6];
  if (read(core_pattern_fd.get(), buf, sizeof(buf)) != 5 ||
      memcmp(buf, "core\n", 5)) {
    LOG(WARNING) << "Your core pattern is not set to 'core\n', cannot dump";
    return false;
  }
  core_pattern_fd.Close();

  // We check that the rlimit on core file size is unlimited.
  struct rlimit core_dump_limit;
  if (getrlimit(RLIMIT_CORE, &core_dump_limit)) {
    LOG(WARNING) << "Failed to get core dump limit: " << strerror(errno);
    return false;
  }

  if (core_dump_limit.rlim_cur != RLIM_INFINITY) {
    if (core_dump_limit.rlim_max != RLIM_INFINITY) {
      LOG(WARNING) << "Cannot core dump: hard limit on core dumps found";
      return false;
    }

    core_dump_limit.rlim_cur = RLIM_INFINITY;
    if (setrlimit(RLIMIT_CORE, &core_dump_limit)) {
      LOG(WARNING) << "Failed to set core dump limit: " << strerror(errno);
      return false;
    }
  }

  // Finally, we move the current directory into a temp dir and return the path
  // to the temp dir so that we can clean up afterwards.
  char temp_dir_template[] = "/tmp/chromium-core-dump-XXXXXX";
  if (mkdtemp(temp_dir_template) == NULL) {
    LOG(WARNING) << "Failed to create temp dir for core dumping: "
                 << strerror(errno);
    return false;
  }

  if (chdir(temp_dir_template)) {
    LOG(WARNING) << "Cannot chdir into temp directory: " << strerror(errno);
    return false;
  }

  *core_dump_directory = temp_dir_template;

  return true;
}

static void UploadCoreFile(const pid_t child, std::string* core_filename) {
  *core_filename = "core";
  ScopedFd fd(open(core_filename->c_str(), O_RDONLY));
  if (fd.get() < 0) {
    *core_filename = StringPrintf("core.%d", child);
    fd.Set(open(core_filename->c_str(), O_RDONLY));
    if (fd.get() < 0) {
      LOG(WARNING) << "Cannot open resulting core dump from browser: "
                   << strerror(errno);
      return;
    }
  }

  struct stat st;
  if (fstat(fd.get(), &st)) {
    LOG(WARNING) << "Failed to stat core file: " << strerror(errno);
    return;
  }

  const uint32_t core_size = st.st_size;

  static const char kMyBinary[] = "/proc/self/exe";
  ScopedFd self_fd(open(kMyBinary, O_RDONLY));
  if (self_fd.get() < 0) {
    LOG(WARNING) << "Cannot open " << kMyBinary << ": " << strerror(errno);
    return;
  }

  if (fstat(self_fd.get(), &st)) {
    LOG(WARNING) << "Failed to stat " << kMyBinary << ": " << strerror(errno);
    return;
  }

  const uint64_t binary_size = st.st_size;

  DictionaryValue header;
  header.SetString(L"core-size", StringPrintf("%" PRIu32, core_size));
  header.SetString(L"chrome-version", FILE_VERSION);
  header.SetString(L"binary-size", StringPrintf("%" PRIu64, binary_size));
  header.SetString(L"user", getenv("USER"));
#if defined(GOOGLE_CHROME_BUILD)
  header.SetBoolean(L"offical-build", true);
#endif

  std::string json;
  JSONWriter::Write(&header, true /* pretty print */, &json);
  const uint32_t json_size = json.size();

  ScopedFd sock(socket(PF_INET, SOCK_STREAM, 0));
  if (sock.get() < 0) {
    LOG(WARNING) << "Cannot open socket: " << strerror(errno);
    return;
  }

  static const char kUploadIP[] = "172.22.68.141";
  static const uint16_t kUploadPort = 9999;

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = inet_addr(kUploadIP);
  sin.sin_port = htons(kUploadPort);

  if (connect(sock.get(), (struct sockaddr*) &sin, sizeof(sin))) {
    LOG(WARNING) << "Failed to connect to upload server (" << kUploadIP
                 << ":" << kUploadPort << "): " << strerror(errno);
    return;
  }

  if (HANDLE_EINTR(write(sock.get(), &json_size, sizeof(json_size))) !=
          static_cast<ssize_t>(sizeof(json_size)) ||
      HANDLE_EINTR(write(sock.get(), json.data(), json_size)) !=
          static_cast<ssize_t>(json_size) ||
      HANDLE_EINTR(write(sock.get(), &core_size, sizeof(core_size))) !=
          static_cast<ssize_t>(sizeof(core_size)) ||
      HANDLE_EINTR(sendfile(sock.get(), fd.get(), NULL, core_size)) !=
          static_cast<ssize_t>(core_size)) {
    LOG(WARNING) << "Failed to write all data to server";
    return;
  }
}

void MonitorForCoreDumpsAndReport(const std::string& core_dump_directory,
                                  const pid_t child) {
  int status;
  const pid_t result = HANDLE_EINTR(waitpid(child, &status, 0));
  if (result < 1) {
    LOG(ERROR) << "Failed to wait for browser child: " << strerror(errno);
    return;
  }

  if (WIFSIGNALED(status) && WCOREDUMP(status)) {
    std::string core_filename;
    UploadCoreFile(child, &core_filename);
    unlink(core_filename.c_str());
  }

  rmdir(core_dump_directory.c_str());
}
