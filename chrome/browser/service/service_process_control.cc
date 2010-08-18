// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/service/service_process_control.h"

#include "base/command_line.h"
#include "base/process_util.h"
#include "base/thread.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_thread.h"
#include "chrome/browser/io_thread.h"
#include "chrome/common/child_process_host.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/service_messages.h"
#include "chrome/common/service_process_util.h"

// ServiceProcessControl::Launcher implementation.
// This class is responsible for launching the service process on the
// PROCESS_LAUNCHER thread.
class ServiceProcessControl::Launcher
    : public base::RefCountedThreadSafe<ServiceProcessControl::Launcher> {
 public:
  Launcher(ServiceProcessControl* process, CommandLine* cmd_line)
      : process_(process),
        cmd_line_(cmd_line),
        launched_(false) {
  }

  // Execute the command line to start the process asynchronously.
  // After the comamnd is executed |task| is called with the process handle on
  // the UI thread.
  void Run(Task* task) {
    DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));

    ChromeThread::PostTask(ChromeThread::PROCESS_LAUNCHER, FROM_HERE,
                           NewRunnableMethod(this, &Launcher::DoRun, task));
  }

  bool launched() const { return launched_; }

 private:
  void DoRun(Task* task) {
    launched_ = base::LaunchApp(*cmd_line_.get(), false, true, NULL);

    ChromeThread::PostTask(
        ChromeThread::IO, FROM_HERE,
        NewRunnableMethod(this, &Launcher::DoDetectLaunched, task));
  }

  void DoDetectLaunched(Task* task) {
    // TODO(hclam): We need to improve the method we are using to connect to
    // the service process. The approach we are using here is to check for
    // the existence of the service process lock file created after the service
    // process is fully launched.
    if (CheckServiceProcessRunning(kServiceProcessCloudPrint)) {
      // After the process is launched we listen on the file system for the
      // service process lock file to detect the service process has launched.
      ChromeThread::PostTask(ChromeThread::UI, FROM_HERE,
          NewRunnableMethod(this, &Launcher::Notify, task));
      return;
    }

    // If the service process is not launched yet then check again in 2 seconds.
    const int kDetectLaunchRetry = 2000;
    ChromeThread::PostDelayedTask(
        ChromeThread::IO, FROM_HERE,
        NewRunnableMethod(this, &Launcher::DoDetectLaunched, task),
        kDetectLaunchRetry);
  }

  void Notify(Task* task) {
    task->Run();
    delete task;
  }

  ServiceProcessControl* process_;
  scoped_ptr<CommandLine> cmd_line_;
  bool launched_;
};

// ServiceProcessControl implementation.
ServiceProcessControl::ServiceProcessControl(Profile* profile,
                                             ServiceProcessType type)
    : profile_(profile),
      type_(type),
      message_handler_(NULL) {
}

ServiceProcessControl::~ServiceProcessControl() {
}

void ServiceProcessControl::Connect(Task* task) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
  if (channel_.get()) {
    task->Run();
    delete task;
    return;
  }

  // Saves the task.
  connect_done_task_.reset(task);
  ConnectInternal();
}

void ServiceProcessControl::ConnectInternal() {
  LOG(INFO) << "Connecting to Service Process IPC Server";
  // Run the IPC channel on the shared IO thread.
  base::Thread* io_thread = g_browser_process->io_thread();

  // TODO(hclam): Determine the the channel id from profile and type.
  const std::string channel_id = GetServiceProcessChannelName(type_);
  channel_.reset(
      new IPC::SyncChannel(channel_id, IPC::Channel::MODE_CLIENT, this, NULL,
                           io_thread->message_loop(), true,
                           g_browser_process->shutdown_event()));
  channel_->set_sync_messages_with_no_timeout_allowed(false);
}

void ServiceProcessControl::Launch(Task* task) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
  if (channel_.get()) {
    task->Run();
    delete task;
    return;
  }

  // A service process should have a different mechanism for starting, but now
  // we start it as if it is a child process.
  FilePath exe_path = ChildProcessHost::GetChildPath(true);
  if (exe_path.empty()) {
    NOTREACHED() << "Unable to get service process binary name.";
  }

  CommandLine* cmd_line = new CommandLine(exe_path);
  cmd_line->AppendSwitchASCII(switches::kProcessType,
                              switches::kServiceProcess);

  const CommandLine& browser_command_line = *CommandLine::ForCurrentProcess();
  FilePath user_data_dir =
      browser_command_line.GetSwitchValuePath(switches::kUserDataDir);
  if (!user_data_dir.empty())
    cmd_line->AppendSwitchPath(switches::kUserDataDir, user_data_dir);

  std::string logging_level = browser_command_line.GetSwitchValueASCII(
      switches::kLoggingLevel);
  if (!logging_level.empty())
    cmd_line->AppendSwitchASCII(switches::kLoggingLevel, logging_level);

  // And then start the process asynchronously.
  launcher_ = new Launcher(this, cmd_line);
  launcher_->Run(
      NewRunnableMethod(this, &ServiceProcessControl::OnProcessLaunched, task));
}

void ServiceProcessControl::OnProcessLaunched(Task* task) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
  if (launcher_->launched()) {
    // Now use the launch task as the connect task.
    connect_done_task_.reset(task);

    // After we have successfully created the service process we try to connect
    // to it. The launch task is transfered to a connect task.
    ConnectInternal();
  } else {
    // If we don't have process handle that means launching the service process
    // has failed.
    task->Run();
    delete task;
  }

  // We don't need the launcher anymore.
  launcher_ = NULL;
}

void ServiceProcessControl::OnMessageReceived(const IPC::Message& message) {
  if (!message_handler_)
    return;

  if (message.type() == ServiceHostMsg_GoodDay::ID)
    message_handler_->OnGoodDay();
}

void ServiceProcessControl::OnChannelConnected(int32 peer_pid) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
  connect_done_task_->Run();
  connect_done_task_.reset();
}

void ServiceProcessControl::OnChannelError() {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
  channel_.reset();
  connect_done_task_->Run();
  connect_done_task_.reset();
}

bool ServiceProcessControl::Send(IPC::Message* message) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
  return channel_->Send(message);
}

bool ServiceProcessControl::SendHello() {
  return Send(new ServiceMsg_Hello());
}

bool ServiceProcessControl::Shutdown() {
  bool ret = Send(new ServiceMsg_Shutdown());
  channel_.reset();
  return ret;
}

bool ServiceProcessControl::EnableRemotingWithTokens(
    const std::string& user,
    const std::string& remoting_token,
    const std::string& talk_token) {
  return Send(
      new ServiceMsg_EnableRemotingWithTokens(user, remoting_token,
                                              talk_token));
}

DISABLE_RUNNABLE_METHOD_REFCOUNT(ServiceProcessControl);
