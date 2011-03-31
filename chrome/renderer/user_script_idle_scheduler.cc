// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/user_script_idle_scheduler.h"

#include "base/message_loop.h"
#include "chrome/common/extensions/extension_messages.h"
#include "chrome/renderer/extension_groups.h"
#include "chrome/renderer/extensions/extension_dispatcher.h"
#include "chrome/renderer/render_thread.h"
#include "chrome/renderer/user_script_slave.h"
#include "content/renderer/render_view.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebString.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"

namespace {
// The length of time to wait after the DOM is complete to try and run user
// scripts.
const int kUserScriptIdleTimeoutMs = 200;
}

using WebKit::WebFrame;
using WebKit::WebString;
using WebKit::WebView;

UserScriptIdleScheduler::UserScriptIdleScheduler(RenderView* render_view,
                                                 WebFrame* frame)
    : RenderViewObserver(render_view),
      ALLOW_THIS_IN_INITIALIZER_LIST(method_factory_(this)),
      frame_(frame),
      has_run_(false) {
}

UserScriptIdleScheduler::~UserScriptIdleScheduler() {
}

bool UserScriptIdleScheduler::OnMessageReceived(const IPC::Message& message) {
  if (message.type() != ExtensionMsg_ExecuteCode::ID)
    return false;

  // chrome.tabs.executeScript() only supports execution in either the top frame
  // or all frames.  We handle both cases in the top frame.
  WebFrame* main_frame = GetMainFrame();
  if (main_frame && main_frame != frame_)
    return false;

  IPC_BEGIN_MESSAGE_MAP(UserScriptIdleScheduler, message)
    IPC_MESSAGE_HANDLER(ExtensionMsg_ExecuteCode, OnExecuteCode)
  IPC_END_MESSAGE_MAP()
  return true;
}

void UserScriptIdleScheduler::DidFinishDocumentLoad(WebFrame* frame) {
  if (frame != frame_)
    return;

  MessageLoop::current()->PostDelayedTask(FROM_HERE,
      method_factory_.NewRunnableMethod(&UserScriptIdleScheduler::MaybeRun),
      kUserScriptIdleTimeoutMs);
}

void UserScriptIdleScheduler::DidFinishLoad(WebFrame* frame) {
  if (frame != frame_)
    return;

  // Ensure that running scripts does not keep any progress UI running.
  MessageLoop::current()->PostTask(FROM_HERE,
      method_factory_.NewRunnableMethod(&UserScriptIdleScheduler::MaybeRun));
}

void UserScriptIdleScheduler::DidStartProvisionalLoad(WebKit::WebFrame* frame) {
  // The frame is navigating, so reset the state since we'll want to inject
  // scripts once the load finishes.
  has_run_ = false;
  method_factory_.RevokeAll();
  while (!pending_code_execution_queue_.empty())
    pending_code_execution_queue_.pop();
}

void UserScriptIdleScheduler::FrameDetached(WebFrame* frame) {
  if (frame != frame_)
    return;

  delete this;
}

void UserScriptIdleScheduler::MaybeRun() {
  if (has_run_)
    return;

  // Note: we must set this before calling ExecuteCodeImpl, because that may
  // result in a synchronous call back into MaybeRun if there is a pending task
  // currently in the queue.
  // http://code.google.com/p/chromium/issues/detail?id=29644
  has_run_ = true;

  if (RenderThread::current()) {  // Will be NULL during unit tests.
    ExtensionDispatcher::Get()->user_script_slave()->InjectScripts(
        frame_, UserScript::DOCUMENT_IDLE);
  }

  while (!pending_code_execution_queue_.empty()) {
    linked_ptr<ExtensionMsg_ExecuteCode_Params>& params =
        pending_code_execution_queue_.front();
    ExecuteCodeImpl(GetMainFrame(), *params);
    pending_code_execution_queue_.pop();
  }
}

void UserScriptIdleScheduler::OnExecuteCode(
    const ExtensionMsg_ExecuteCode_Params& params) {
  WebFrame* main_frame = GetMainFrame();
  if (!main_frame) {
    Send(new ViewHostMsg_ExecuteCodeFinished(
        routing_id(), params.request_id, false));
    return;
  }

  if (!has_run_) {
    pending_code_execution_queue_.push(
        linked_ptr<ExtensionMsg_ExecuteCode_Params>(
            new ExtensionMsg_ExecuteCode_Params(params)));
    return;
  }

  ExecuteCodeImpl(main_frame, params);
}

void UserScriptIdleScheduler::ExecuteCodeImpl(
    WebFrame* frame, const ExtensionMsg_ExecuteCode_Params& params) {
  std::vector<WebFrame*> frame_vector;
  frame_vector.push_back(frame);
  if (params.all_frames)
    GetAllChildFrames(frame, &frame_vector);

  for (std::vector<WebFrame*>::iterator frame_it = frame_vector.begin();
       frame_it != frame_vector.end(); ++frame_it) {
    WebFrame* frame = *frame_it;
    if (params.is_javascript) {
      const Extension* extension =
          ExtensionDispatcher::Get()->extensions()->GetByID(
              params.extension_id);

    // Since extension info is sent separately from user script info, they can
    // be out of sync. We just ignore this situation.
    if (!extension)
      continue;

      if (!extension->CanExecuteScriptOnPage(frame->url(), NULL, NULL))
        continue;

      std::vector<WebScriptSource> sources;
      sources.push_back(
          WebScriptSource(WebString::fromUTF8(params.code)));
      UserScriptSlave::InsertInitExtensionCode(&sources, params.extension_id);
      frame->executeScriptInIsolatedWorld(
          UserScriptSlave::GetIsolatedWorldId(params.extension_id),
          &sources.front(), sources.size(), EXTENSION_GROUP_CONTENT_SCRIPTS);
    } else {
      frame->insertStyleText(WebString::fromUTF8(params.code), WebString());
    }
  }

  Send(new ViewHostMsg_ExecuteCodeFinished(
      routing_id(), params.request_id, true));
}

bool UserScriptIdleScheduler::GetAllChildFrames(
    WebFrame* parent_frame,
    std::vector<WebFrame*>* frames_vector) const {
  if (!parent_frame)
    return false;

  for (WebFrame* child_frame = parent_frame->firstChild(); child_frame;
       child_frame = child_frame->nextSibling()) {
    frames_vector->push_back(child_frame);
    GetAllChildFrames(child_frame, frames_vector);
  }
  return true;
}

WebFrame* UserScriptIdleScheduler::GetMainFrame() {
  WebView* webview = render_view()->webview();
  return webview ? webview->mainFrame() : NULL;
}
