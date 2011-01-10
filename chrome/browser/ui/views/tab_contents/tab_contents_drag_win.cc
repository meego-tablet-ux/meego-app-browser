// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tab_contents/tab_contents_drag_win.h"

#include <windows.h>

#include <string>

#include "base/file_path.h"
#include "base/message_loop.h"
#include "base/task.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_node_data.h"
#include "chrome/browser/browser_thread.h"
#include "chrome/browser/download/download_util.h"
#include "chrome/browser/download/drag_download_file.h"
#include "chrome/browser/download/drag_download_util.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/browser/tab_contents/web_drag_source_win.h"
#include "chrome/browser/tab_contents/web_drag_utils_win.h"
#include "chrome/browser/tab_contents/web_drop_target_win.h"
#include "chrome/browser/ui/views/tab_contents/tab_contents_view_win.h"
#include "chrome/common/url_constants.h"
#include "net/base/net_util.h"
#include "views/drag_utils.h"
#include "webkit/glue/webdropdata.h"

using WebKit::WebDragOperationsMask;
using WebKit::WebDragOperationCopy;
using WebKit::WebDragOperationLink;
using WebKit::WebDragOperationMove;

namespace {

HHOOK msg_hook = NULL;
DWORD drag_out_thread_id = 0;
bool mouse_up_received = false;

LRESULT CALLBACK MsgFilterProc(int code, WPARAM wparam, LPARAM lparam) {
  if (code == base::MessagePumpForUI::kMessageFilterCode &&
      !mouse_up_received) {
    MSG* msg = reinterpret_cast<MSG*>(lparam);
    // We do not care about WM_SYSKEYDOWN and WM_SYSKEYUP because when ALT key
    // is pressed down on drag-and-drop, it means to create a link.
    if (msg->message == WM_MOUSEMOVE || msg->message == WM_LBUTTONUP ||
        msg->message == WM_KEYDOWN || msg->message == WM_KEYUP) {
      // Forward the message from the UI thread to the drag-and-drop thread.
      PostThreadMessage(drag_out_thread_id,
                        msg->message,
                        msg->wParam,
                        msg->lParam);

      // If the left button is up, we do not need to forward the message any
      // more.
      if (msg->message == WM_LBUTTONUP || !(GetKeyState(VK_LBUTTON) & 0x8000))
        mouse_up_received = true;

      return TRUE;
    }
  }
  return CallNextHookEx(msg_hook, code, wparam, lparam);
}

}  // namespace

class DragDropThread : public base::Thread {
 public:
  explicit DragDropThread(TabContentsDragWin* drag_handler)
       : base::Thread("Chrome_DragDropThread"),
         drag_handler_(drag_handler) {
  }

  virtual ~DragDropThread() {
    Thread::Stop();
  }

 protected:
  // base::Thread implementations:
  virtual void Init() {
    int ole_result = OleInitialize(NULL);
    DCHECK(ole_result == S_OK);
  }

  virtual void CleanUp() {
    OleUninitialize();
  }

 private:
  // Hold a reference count to TabContentsDragWin to make sure that it is always
  // alive in the thread lifetime.
  scoped_refptr<TabContentsDragWin> drag_handler_;

  DISALLOW_COPY_AND_ASSIGN(DragDropThread);
};

TabContentsDragWin::TabContentsDragWin(TabContentsViewWin* view)
    : drag_drop_thread_id_(0),
      view_(view),
      drag_ended_(false),
      old_drop_target_suspended_state_(false) {
}

TabContentsDragWin::~TabContentsDragWin() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!drag_drop_thread_.get());
}

void TabContentsDragWin::StartDragging(const WebDropData& drop_data,
                                       WebDragOperationsMask ops,
                                       const SkBitmap& image,
                                       const gfx::Point& image_offset) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  drag_source_ = new WebDragSource(view_->GetNativeView(),
                                   view_->tab_contents());

  const GURL& page_url = view_->tab_contents()->GetURL();
  const std::string& page_encoding = view_->tab_contents()->encoding();

  // If it is not drag-out, do the drag-and-drop in the current UI thread.
  if (drop_data.download_metadata.empty()) {
    DoDragging(drop_data, ops, page_url, page_encoding, image, image_offset);
    EndDragging(false);
    return;
  }

  // We do not want to drag and drop the download to itself.
  old_drop_target_suspended_state_ = view_->drop_target()->suspended();
  view_->drop_target()->set_suspended(true);

  // Start a background thread to do the drag-and-drop.
  DCHECK(!drag_drop_thread_.get());
  drag_drop_thread_.reset(new DragDropThread(this));
  base::Thread::Options options;
  options.message_loop_type = MessageLoop::TYPE_UI;
  if (drag_drop_thread_->StartWithOptions(options)) {
    drag_drop_thread_->message_loop()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this,
                          &TabContentsDragWin::StartBackgroundDragging,
                          drop_data,
                          ops,
                          page_url,
                          page_encoding,
                          image,
                          image_offset));
  }

  // Install a hook procedure to monitor the messages so that we can forward
  // the appropriate ones to the background thread.
  drag_out_thread_id = drag_drop_thread_->thread_id();
  mouse_up_received = false;
  DCHECK(!msg_hook);
  msg_hook = SetWindowsHookEx(WH_MSGFILTER,
                              MsgFilterProc,
                              NULL,
                              GetCurrentThreadId());

  // Attach the input state of the background thread to the UI thread so that
  // SetCursor can work from the background thread.
  AttachThreadInput(drag_out_thread_id, GetCurrentThreadId(), TRUE);
}

void TabContentsDragWin::StartBackgroundDragging(
    const WebDropData& drop_data,
    WebDragOperationsMask ops,
    const GURL& page_url,
    const std::string& page_encoding,
    const SkBitmap& image,
    const gfx::Point& image_offset) {
  drag_drop_thread_id_ = base::PlatformThread::CurrentId();

  DoDragging(drop_data, ops, page_url, page_encoding, image, image_offset);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      NewRunnableMethod(this, &TabContentsDragWin::EndDragging, true));
}

void TabContentsDragWin::PrepareDragForDownload(
    const WebDropData& drop_data,
    OSExchangeData* data,
    const GURL& page_url,
    const std::string& page_encoding) {
  // Parse the download metadata.
  string16 mime_type;
  FilePath file_name;
  GURL download_url;
  if (!drag_download_util::ParseDownloadMetadata(drop_data.download_metadata,
                                                 &mime_type,
                                                 &file_name,
                                                 &download_url))
    return;

  // Generate the download filename.
  std::string content_disposition =
      "attachment; filename=" + UTF16ToUTF8(file_name.value());
  FilePath generated_file_name;
  download_util::GenerateFileName(download_url,
                                  content_disposition,
                                  std::string(),
                                  UTF16ToUTF8(mime_type),
                                  &generated_file_name);

  // Provide the data as file (CF_HDROP). A temporary download file with the
  // Zone.Identifier ADS (Alternate Data Stream) attached will be created.
  linked_ptr<net::FileStream> empty_file_stream;
  scoped_refptr<DragDownloadFile> download_file =
      new DragDownloadFile(generated_file_name,
                           empty_file_stream,
                           download_url,
                           page_url,
                           page_encoding,
                           view_->tab_contents());
  OSExchangeData::DownloadFileInfo file_download(FilePath(),
                                                 download_file.get());
  data->SetDownloadFileInfo(file_download);

  // Enable asynchronous operation.
  OSExchangeDataProviderWin::GetIAsyncOperation(*data)->SetAsyncMode(TRUE);
}

void TabContentsDragWin::PrepareDragForFileContents(
    const WebDropData& drop_data, OSExchangeData* data) {
  // Images without ALT text will only have a file extension so we need to
  // synthesize one from the provided extension and URL.
  FilePath file_name(drop_data.file_description_filename);
  file_name = file_name.BaseName().RemoveExtension();
  if (file_name.value().empty()) {
    // Retrieve the name from the URL.
    file_name = net::GetSuggestedFilename(drop_data.url, "", "", FilePath());
    if (file_name.value().size() + drop_data.file_extension.size() + 1 >
        MAX_PATH) {
      file_name = FilePath(file_name.value().substr(
          0, MAX_PATH - drop_data.file_extension.size() - 2));
    }
  }
  file_name = file_name.ReplaceExtension(drop_data.file_extension);
  data->SetFileContents(file_name.value(), drop_data.file_contents);
}

void TabContentsDragWin::PrepareDragForUrl(const WebDropData& drop_data,
                                           OSExchangeData* data) {
  if (drop_data.url.SchemeIs(chrome::kJavaScriptScheme)) {
    // We don't want to allow javascript URLs to be dragged to the desktop,
    // but we do want to allow them to be added to the bookmarks bar
    // (bookmarklets). So we create a fake bookmark entry (BookmarkNodeData
    // object) which explorer.exe cannot handle, and write the entry to data.
    BookmarkNodeData::Element bm_elt;
    bm_elt.is_url = true;
    bm_elt.url = drop_data.url;
    bm_elt.title = drop_data.url_title;

    BookmarkNodeData bm_drag_data;
    bm_drag_data.elements.push_back(bm_elt);

    // Pass in NULL as the profile so that the bookmark always adds the url
    // rather than trying to move an existing url.
    bm_drag_data.Write(NULL, data);
  } else {
    data->SetURL(drop_data.url, drop_data.url_title);
  }
}

void TabContentsDragWin::DoDragging(const WebDropData& drop_data,
                                    WebDragOperationsMask ops,
                                    const GURL& page_url,
                                    const std::string& page_encoding,
                                    const SkBitmap& image,
                                    const gfx::Point& image_offset) {
  OSExchangeData data;

  if (!drop_data.download_metadata.empty()) {
    PrepareDragForDownload(drop_data, &data, page_url, page_encoding);

    // Set the observer.
    OSExchangeDataProviderWin::GetDataObjectImpl(data)->set_observer(this);
  } else {
    // We set the file contents before the URL because the URL also sets file
    // contents (to a .URL shortcut).  We want to prefer file content data over
    // a shortcut so we add it first.
    if (!drop_data.file_contents.empty())
      PrepareDragForFileContents(drop_data, &data);
    if (!drop_data.text_html.empty())
      data.SetHtml(drop_data.text_html, drop_data.html_base_url);
    // We set the text contents before the URL because the URL also sets text
    // content.
    if (!drop_data.plain_text.empty())
      data.SetString(drop_data.plain_text);
    if (drop_data.url.is_valid())
      PrepareDragForUrl(drop_data, &data);
  }

  // Set drag image.
  if (!image.isNull()) {
    drag_utils::SetDragImageOnDataObject(
        image, gfx::Size(image.width(), image.height()), image_offset, &data);
  }

  // We need to enable recursive tasks on the message loop so we can get
  // updates while in the system DoDragDrop loop.
  bool old_state = MessageLoop::current()->NestableTasksAllowed();
  MessageLoop::current()->SetNestableTasksAllowed(true);
  DWORD effect;
  DoDragDrop(OSExchangeDataProviderWin::GetIDataObject(data), drag_source_,
             web_drag_utils_win::WebDragOpMaskToWinDragOpMask(ops), &effect);
  MessageLoop::current()->SetNestableTasksAllowed(old_state);

  // This works because WebDragSource::OnDragSourceDrop uses PostTask to
  // dispatch the actual event.
  drag_source_->set_effect(effect);
}

void TabContentsDragWin::EndDragging(bool restore_suspended_state) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (drag_ended_)
    return;
  drag_ended_ = true;

  if (restore_suspended_state)
    view_->drop_target()->set_suspended(old_drop_target_suspended_state_);

  if (msg_hook) {
    AttachThreadInput(drag_out_thread_id, GetCurrentThreadId(), FALSE);
    UnhookWindowsHookEx(msg_hook);
    msg_hook = NULL;
  }

  view_->EndDragging();
}

void TabContentsDragWin::CancelDrag() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  drag_source_->CancelDrag();
}

void TabContentsDragWin::CloseThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  drag_drop_thread_.reset();
}

void TabContentsDragWin::OnWaitForData() {
  DCHECK(drag_drop_thread_id_ == base::PlatformThread::CurrentId());

  // When the left button is release and we start to wait for the data, end
  // the dragging before DoDragDrop returns. This makes the page leave the drag
  // mode so that it can start to process the normal input events.
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      NewRunnableMethod(this, &TabContentsDragWin::EndDragging, true));
}

void TabContentsDragWin::OnDataObjectDisposed() {
  DCHECK(drag_drop_thread_id_ == base::PlatformThread::CurrentId());

  // The drag-and-drop thread is only closed after OLE is done with
  // DataObjectImpl.
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      NewRunnableMethod(this, &TabContentsDragWin::CloseThread));
}
