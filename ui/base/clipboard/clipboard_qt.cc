// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/clipboard/clipboard.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QImage>
#include <QBuffer>

#include <map>
#include <set>
#include <string>
#include <utility>

#include "base/file_path.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"

#include "base/utf_string_conversions.h"
#include "ui/gfx/size.h"

namespace ui {

namespace {

const char kMimeBmp[] = "image/bmp";
const char kMimeHtml[] = "text/html";
const char kMimeText[] = "text/plain";
const char kMimeMozillaUrl[] = "text/x-moz-url";
const char kMimeWebkitSmartPaste[] = "chromium/x-webkit-paste";

}  // namespace

Clipboard::Clipboard(): clipboard_data_(NULL) {
///\todo FIXME: GTK provide different clipboard_ for operation on selection
//while Qt use QClipboard::Mode to differ operation target on the same QClipboard.
//So we might need to wrap it for this usage instead of just make it the same one.
  clipboard_ = QApplication::clipboard();
  qclipboard_mode_ = QClipboard::Clipboard;
  primary_selection_ = NULL;
}

Clipboard::~Clipboard() {
  // TODO(estade): do we want to save clipboard data after we exit?
  // gtk_clipboard_set_can_store and gtk_clipboard_store work
  // but have strangely awful performance.
}

void Clipboard::WriteObjects(const ObjectMap& objects) {
  clipboard_data_ = new TargetMap();
    
  for (ObjectMap::const_iterator iter = objects.begin();
       iter != objects.end(); ++iter) {
    DispatchObject(static_cast<ObjectType>(iter->first), iter->second);
  }

  SetQtClipboard();
}

// When a URL is copied from a render view context menu (via "copy link
// location", for example), we additionally stick it in the X clipboard. This
// matches other linux browsers.
void Clipboard::DidWriteURL(const std::string& utf8_text) {
  clipboard_->setText(QString::fromStdString(utf8_text),
                      QClipboard::Selection);
}

void Clipboard::SetQtClipboard() {
  int i = 0;
  QMimeData *mime = new QMimeData;
  for (Clipboard::TargetMap::iterator iter = clipboard_data_->begin();
       iter != clipboard_data_->end(); ++iter, ++i) {
    if(!iter->second.first)
      continue;
    if(iter->first == kMimeBmp)
    {
      QImage image;
      QByteArray* ba = reinterpret_cast<QByteArray*>(iter->second.first);
      image.loadFromData(*ba);
      mime->setImageData(image);
      delete ba;
    }
    else
    {
      char* data = iter->second.first;
      size_t size = iter->second.second;
      mime->setData(QString::fromStdString(iter->first),
                    QByteArray(data, size));
      delete data;
    }
  }

  //ownership of mime is transferred to clipboard
  clipboard_->setMimeData(mime, QClipboard::Mode(qclipboard_mode_));

  delete clipboard_data_;
  clipboard_data_ = NULL;
}

void Clipboard::WriteText(const char* text_data, size_t text_len) {
  char* data = new char[text_len];
  memcpy(data, text_data, text_len);

  InsertMapping(kMimeText, data, text_len);
}

void Clipboard::WriteHTML(const char* markup_data,
                          size_t markup_len,
                          const char* url_data,
                          size_t url_len) {
  // TODO(estade): We need to expand relative links with |url_data|.
  static const char* html_prefix = "<meta http-equiv=\"content-type\" "
                                   "content=\"text/html; charset=utf-8\">";
  size_t html_prefix_len = strlen(html_prefix);
  size_t total_len = html_prefix_len + markup_len + 1;

  char* data = new char[total_len];
  snprintf(data, total_len, "%s", html_prefix);
  memcpy(data + html_prefix_len, markup_data, markup_len);
  // Some programs expect NULL-terminated data. See http://crbug.com/42624
  data[total_len - 1] = '\0';

  InsertMapping(kMimeHtml, data, total_len);
}

// Write an extra flavor that signifies WebKit was the last to modify the
// pasteboard. This flavor has no data.
void Clipboard::WriteWebSmartPaste() {
  InsertMapping(kMimeWebkitSmartPaste, NULL, 0);
}

void Clipboard::WriteBitmap(const char* pixel_data, const char* size_data) {
  const gfx::Size* size = reinterpret_cast<const gfx::Size*>(size_data);

  QImage image(reinterpret_cast<const uchar*>(pixel_data), size->width(), size->height(), QImage::Format_ARGB32_Premultiplied);
  QByteArray *ba = new QByteArray;
  QBuffer buffer(ba);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "BMP");
  InsertMapping(kMimeBmp, reinterpret_cast<char*>(ba), 0);
}

void Clipboard::WriteBookmark(const char* title_data, size_t title_len,
                              const char* url_data, size_t url_len) {
  // Write as a mozilla url (UTF16: URL, newline, title).
  string16 url = UTF8ToUTF16(std::string(url_data, url_len) + "\n");
  string16 title = UTF8ToUTF16(std::string(title_data, title_len));
  int data_len = 2 * (title.length() + url.length());

  char* data = new char[data_len];
  memcpy(data, url.data(), 2 * url.length());
  memcpy(data + 2 * url.length(), title.data(), 2 * title.length());
  InsertMapping(kMimeMozillaUrl, data, data_len);
}

void Clipboard::WriteData(const char* format_name, size_t format_len,
                          const char* data_data, size_t data_len) {
  std::string format(format_name, format_len);
  // We assume that certain mapping types are only written by trusted code.
  // Therefore we must upkeep their integrity.
  if (format == kMimeBmp)
    return;
  char* data = new char[data_len];
  memcpy(data, data_data, data_len);
  InsertMapping(format.c_str(), data, data_len);
}

// We do not use gtk_clipboard_wait_is_target_available because of
// a bug with the gtk clipboard. It caches the available targets
// and does not always refresh the cache when it is appropriate.
bool Clipboard::IsFormatAvailable(const Clipboard::FormatType& format,
                                  Clipboard::Buffer buffer) {
  ClipboardType* clipboard = LookupBackingClipboard(buffer);
  if (clipboard == NULL)
    return false;

  const QMimeData* mime = clipboard_->mimeData(QClipboard::Mode(qclipboard_mode_));
  return mime->hasFormat(QString::fromStdString(format));
}

bool Clipboard::IsFormatAvailableByString(const std::string& format,
                                          Clipboard::Buffer buffer){
  return IsFormatAvailable(format, buffer);
}

void Clipboard::ReadAvailableTypes(Clipboard::Buffer buffer,
                                   std::vector<string16>* types,
                                   bool* contains_filenames) const {
  if (!types || !contains_filenames) {
    NOTREACHED();
    return;
  }

  // TODO(dcheng): Implement me.
  types->clear();
  *contains_filenames = false;
}

void Clipboard::ReadText(Clipboard::Buffer buffer, string16* result){
  ClipboardType* clipboard = LookupBackingClipboard(buffer);
  if (clipboard == NULL)
    return;

  result->clear();

  QString text = clipboard_->text(QClipboard::Mode(qclipboard_mode_));

  if (text.isEmpty())
    return;

  UTF8ToUTF16(text.toUtf8().data(), text.toUtf8().size(), result);
}

void Clipboard::ReadAsciiText(Clipboard::Buffer buffer,
                              std::string* result) {
  ClipboardType* clipboard = LookupBackingClipboard(buffer);
  if (clipboard == NULL)
    return;

  result->clear();
  QString text = clipboard_->text(QClipboard::Mode(qclipboard_mode_));

  if (text.isEmpty())
    return;

  result->assign(text.toAscii().data());
}

void Clipboard::ReadFile(FilePath* file) {
  *file = FilePath();
}

// TODO(estade): handle different charsets.
// TODO(port): set *src_url.
void Clipboard::ReadHTML(Clipboard::Buffer buffer, string16* markup,
                         std::string* src_url) {
  ClipboardType* clipboard = LookupBackingClipboard(buffer);
  if (clipboard == NULL)
    return;
  markup->clear();

  QString format("html");
  QString text = clipboard_->text(format, QClipboard::Mode(qclipboard_mode_));

  if (text.isEmpty())
    return;

  UTF8ToUTF16(reinterpret_cast<char*>(text.toUtf8().data()), text.toUtf8().size(), markup);

  // If there is a terminating NULL, drop it.
  if (!markup->empty() && markup->at(markup->length() - 1) == '\0')
    markup->resize(markup->length() - 1);
}

void Clipboard::ReadImage(Buffer buffer, std::string* data) const {
  // TODO(dcheng): implement this.
  NOTIMPLEMENTED();
  if (!data) {
    NOTREACHED();
    return;
  }
}

void Clipboard::ReadBookmark(string16* title, std::string* url) {
  // TODO(estade): implement this.
  NOTIMPLEMENTED();
}

void Clipboard::ReadData(const std::string& format, std::string* result) {
  const QMimeData* mime = clipboard_->mimeData();
  if (!mime->hasFormat(QString::fromStdString(format)))
      return;

  result->assign(reinterpret_cast<char*>(mime->data(QString::fromStdString(format)).data()), mime->data(QString::fromStdString(format)).size());
}

// static
Clipboard::FormatType Clipboard::GetPlainTextFormatType() {
  return std::string(kMimeText);
}

// static
Clipboard::FormatType Clipboard::GetPlainTextWFormatType() {
  return GetPlainTextFormatType();
}

// static
Clipboard::FormatType Clipboard::GetHtmlFormatType() {
  return std::string(kMimeHtml);
}

// static
Clipboard::FormatType Clipboard::GetBitmapFormatType() {
  return std::string(kMimeBmp);
}

// static
Clipboard::FormatType Clipboard::GetWebKitSmartPasteFormatType() {
  return std::string(kMimeWebkitSmartPaste);
}

void Clipboard::InsertMapping(const char* key,
                              char* data,
                              size_t data_len) {
  DCHECK(clipboard_data_->find(key) == clipboard_data_->end());
  (*clipboard_data_)[key] = std::make_pair(data, data_len);
}

ClipboardType* Clipboard::LookupBackingClipboard(Buffer clipboard) {
  switch (clipboard) {
    case BUFFER_STANDARD:
      {
        qclipboard_mode_ = QClipboard::Clipboard;
        return clipboard_;
      }
    case BUFFER_SELECTION:
      {
        qclipboard_mode_ = QClipboard::Selection;
        return clipboard_;
      }
    default:
      NOTREACHED();
      return NULL;
  }  
}

}  // namespace ui
