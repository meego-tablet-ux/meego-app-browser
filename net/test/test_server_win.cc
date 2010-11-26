// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/test/test_server.h"

#include <windows.h>
#include <wincrypt.h>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/file_util.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/test/test_timeouts.h"
#include "base/thread.h"
#include "base/utf_string_conversions.h"

#pragma comment(lib, "crypt32.lib")

namespace {

bool LaunchTestServerAsJob(const CommandLine& cmdline,
                           bool start_hidden,
                           base::ProcessHandle* process_handle,
                           ScopedHandle* job_handle) {
  // Launch test server process.
  STARTUPINFO startup_info = {0};
  startup_info.cb = sizeof(startup_info);
  startup_info.dwFlags = STARTF_USESHOWWINDOW;
  startup_info.wShowWindow = start_hidden ? SW_HIDE : SW_SHOW;
  PROCESS_INFORMATION process_info;

  // If this code is run under a debugger, the test server process is
  // automatically associated with a job object created by the debugger.
  // The CREATE_BREAKAWAY_FROM_JOB flag is used to prevent this.
  if (!CreateProcess(
      NULL, const_cast<wchar_t*>(cmdline.command_line_string().c_str()),
      NULL, NULL, TRUE, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL,
      &startup_info, &process_info)) {
    LOG(ERROR) << "Could not create process.";
    return false;
  }
  CloseHandle(process_info.hThread);

  // If the caller wants the process handle, we won't close it.
  if (process_handle) {
    *process_handle = process_info.hProcess;
  } else {
    CloseHandle(process_info.hProcess);
  }

  // Create a JobObject and associate the test server process with it.
  job_handle->Set(CreateJobObject(NULL, NULL));
  if (!job_handle->IsValid()) {
    LOG(ERROR) << "Could not create JobObject.";
    return false;
  } else {
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit_info = {0};
    limit_info.BasicLimitInformation.LimitFlags =
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (0 == SetInformationJobObject(job_handle->Get(),
      JobObjectExtendedLimitInformation, &limit_info, sizeof(limit_info))) {
      LOG(ERROR) << "Could not SetInformationJobObject.";
      return false;
    }
    if (0 == AssignProcessToJobObject(job_handle->Get(),
                                      process_info.hProcess)) {
      LOG(ERROR) << "Could not AssignProcessToObject.";
      return false;
    }
  }
  return true;
}

// Writes |size| bytes to |handle| and sets |*unblocked| to true.
// Used as a crude timeout mechanism by ReadData().
void UnblockPipe(HANDLE handle, DWORD size, bool* unblocked) {
  std::string unblock_data(size, '\0');
  // Unblock the ReadFile in TestServer::WaitToStart by writing to the pipe.
  // Make sure the call succeeded, otherwise we are very likely to hang.
  DWORD bytes_written = 0;
  LOG(WARNING) << "Timeout reached; unblocking pipe by writing "
               << size << " bytes";
  CHECK(WriteFile(handle, unblock_data.data(), size, &bytes_written,
                  NULL));
  CHECK_EQ(size, bytes_written);
  *unblocked = true;
}

// Given a file handle, reads into |buffer| until |bytes_max| bytes
// has been read or an error has been encountered.  Returns
// true if the read was successful.
bool ReadData(HANDLE read_fd, HANDLE write_fd,
              DWORD bytes_max, uint8* buffer) {
  base::Thread thread("test_server_watcher");
  if (!thread.Start())
    return false;

  // Prepare a timeout in case the server fails to start.
  bool unblocked = false;
  thread.message_loop()->PostDelayedTask(
      FROM_HERE,
      NewRunnableFunction(UnblockPipe, write_fd, bytes_max, &unblocked),
      TestTimeouts::action_max_timeout_ms());

  DWORD bytes_read = 0;
  while (bytes_read < bytes_max) {
    DWORD num_bytes;
    if (!ReadFile(read_fd, buffer + bytes_read, bytes_max - bytes_read,
                  &num_bytes, NULL)) {
      PLOG(ERROR) << "ReadFile failed";
      return false;
    }
    if (num_bytes <= 0) {
      LOG(ERROR) << "ReadFile returned invalid byte count: " << num_bytes;
      return false;
    }
    bytes_read += num_bytes;
  }

  thread.Stop();
  // If the timeout kicked in, abort.
  if (unblocked) {
    LOG(ERROR) << "Timeout exceeded for ReadData";
    return false;
  }

  return true;
}

}  // namespace

namespace net {

bool TestServer::LaunchPython(const FilePath& testserver_path) {
  FilePath python_exe;
  if (!PathService::Get(base::DIR_SOURCE_ROOT, &python_exe))
    return false;
  python_exe = python_exe
      .Append(FILE_PATH_LITERAL("third_party"))
      .Append(FILE_PATH_LITERAL("python_26"))
      .Append(FILE_PATH_LITERAL("python.exe"));

  CommandLine python_command(python_exe);
  python_command.AppendArgPath(testserver_path);
  if (!AddCommandLineArguments(&python_command))
    return false;

  HANDLE child_read = NULL;
  HANDLE child_write = NULL;
  if (!CreatePipe(&child_read, &child_write, NULL, 0)) {
    PLOG(ERROR) << "Failed to create pipe";
    return false;
  }
  child_read_fd_.Set(child_read);
  child_write_fd_.Set(child_write);

  // Have the child inherit the write half.
  if (!SetHandleInformation(child_write, HANDLE_FLAG_INHERIT,
                            HANDLE_FLAG_INHERIT)) {
    PLOG(ERROR) << "Failed to enable pipe inheritance";
    return false;
  }

  // Pass the handle on the command-line. Although HANDLE is a
  // pointer, truncating it on 64-bit machines is okay. See
  // http://msdn.microsoft.com/en-us/library/aa384203.aspx
  //
  // "64-bit versions of Windows use 32-bit handles for
  // interoperability. When sharing a handle between 32-bit and 64-bit
  // applications, only the lower 32 bits are significant, so it is
  // safe to truncate the handle (when passing it from 64-bit to
  // 32-bit) or sign-extend the handle (when passing it from 32-bit to
  // 64-bit)."
  python_command.AppendSwitchASCII(
      "startup-pipe",
      base::IntToString(reinterpret_cast<uintptr_t>(child_write)));

  if (!LaunchTestServerAsJob(python_command,
                             true,
                             &process_handle_,
                             &job_handle_)) {
    LOG(ERROR) << "Failed to launch " << python_command.command_line_string();
    return false;
  }

  return true;
}

bool TestServer::WaitToStart() {
  ScopedHandle read_fd(child_read_fd_.Take());
  ScopedHandle write_fd(child_write_fd_.Take());

  // Try to read two bytes from the pipe indicating the ephemeral port number.
  uint16 port = 0;
  if (!ReadData(read_fd.Get(), write_fd.Get(), sizeof(port),
                reinterpret_cast<uint8*>(&port))) {
    LOG(ERROR) << "Could not read port";
    return false;
  }

  host_port_pair_.set_port(port);
  return true;
}

bool TestServer::CheckCATrusted() {
  HCERTSTORE cert_store = CertOpenSystemStore(NULL, L"ROOT");
  if (!cert_store) {
    LOG(ERROR) << " could not open trusted root CA store";
    return false;
  }
  PCCERT_CONTEXT cert =
      CertFindCertificateInStore(cert_store,
                                 X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                 0,
                                 CERT_FIND_ISSUER_STR,
                                 L"Test CA",
                                 NULL);
  if (cert)
    CertFreeCertificateContext(cert);
  CertCloseStore(cert_store, 0);

  if (!cert) {
    LOG(ERROR) << " TEST CONFIGURATION ERROR: you need to import the test ca "
                  "certificate to your trusted roots for this test to work. "
                  "For more info visit:\n"
                  "http://dev.chromium.org/developers/testing\n";
    return false;
  }

  return true;
}

}  // namespace net
