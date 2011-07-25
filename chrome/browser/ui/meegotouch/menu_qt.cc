/*
 * Copyright (c) 2010, Intel Corporation. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following disclaimer 
 * in the documentation and/or other materials provided with the 
 * distribution.
 *     * Neither the name of Intel Corporation nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "chrome/browser/ui/meegotouch/menu_qt.h"

#include <map>

#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeItem>
#include <QGraphicsLineItem>
#include <QVariant>
#include <QStringList>

#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/menu_model.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/stl_util-inl.h"
#include "base/utf_string_conversions.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"

QString ConvertMenuLabelFromWindowsStyle(const std::string& label) {
/// we need to eliminate acceralator here, and we need to convert label from utf8 to unicode
///\todo in chinese , the label's acceralator has the format of (&T), we just eliminate & for now.
  std::string tmp;
  tmp.reserve(label.length() * 2);
  for (size_t i = 0; i < label.length(); ++i) {
    if ('&' == label[i]) {
      if (i + 1 < label.length() && '&' == label[i + 1]) {
        tmp.push_back(label[i]);
        ++i;
      } else {
        // just ignore the single &
      }
    } else {
      tmp.push_back(label[i]);
    }
  }

  return QString::fromUtf8(tmp.c_str());
}

class MenuQtImpl: public QObject
{
  Q_OBJECT;
 public:
  MenuQtImpl(MenuQt* menu):
      QObject(),
      menu_(menu)
  {}

  void Append(QString& label, int id)
  {
    list_.append(label);
    id_list_.append(id);
  }

  void Clear()
  {
    list_.clear();
    id_list_.clear();
  }

  QStringList& GetList()
  {
    return list_;
  }

  void PopupAt(int x, int y)
  {
    emit popupAt(x, y);
  }
        
 public slots:
  void activateAt(int index)
  {
    if(menu_->model_ && menu_->model_->IsEnabledAt(id_list_.at(index)))
      menu_->model_->ActivatedAt(id_list_.at(index));
  }
  void close()
  {
    menu_->CloseMenu();
  }
 signals:
  void popupAt(int x, int y);
 private:
  MenuQt* menu_;
  QStringList list_;
  QList<int> id_list_;
};

MenuQt::MenuQt(BrowserWindowQt* window):
    window_(window),
    model_(NULL)
{
  impl_ = new MenuQtImpl(this);

  QDeclarativeView* view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("browserMenuObject", impl_);
  context->setContextProperty("browserMenuModel", QVariant::fromValue(impl_->GetList()));
}

MenuQt::~MenuQt()
{
  delete impl_;
}

void MenuQt::SetModel(ui::MenuModel* model)
{
  model_ = model;
  impl_->Clear();

  for (int i = 0; i < model->GetItemCount(); ++i)
  {
    SkBitmap icon;
    QString label;

    label.clear();
    switch (model->GetTypeAt(i)) {
      case ui::MenuModel::TYPE_SEPARATOR:
        DNOTIMPLEMENTED();
        continue;

      case ui::MenuModel::TYPE_CHECK:
        DNOTIMPLEMENTED();
        break;

      case ui::MenuModel::TYPE_RADIO: {
        DNOTIMPLEMENTED();
        break;
      }

      case ui::MenuModel::TYPE_BUTTON_ITEM: {
        DNOTIMPLEMENTED();
        break;
      }

      case ui::MenuModel::TYPE_SUBMENU:
      case ui::MenuModel::TYPE_COMMAND:
        label = ConvertMenuLabelFromWindowsStyle(UTF16ToUTF8(model->GetLabelAt(i)));
        break;

      default:
        NOTREACHED();
    }

    if(!label.isEmpty())
      impl_->Append(label, i);
  }

  QDeclarativeView* view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("browserMenuModel", QVariant::fromValue(impl_->GetList()));  
}
  
void MenuQt::Popup()
{
  
}
  // Displays the menu at the given coords. |point| is intentionally not const.
void MenuQt::PopupAt(gfx::Point point)
{
  // TODO: compose embeded flash window with correct menu rect
  gfx::Rect rect(point.x(), point.y(), 0, 0);

  window_->ComposeEmbededFlashWindow(rect);
  impl_->PopupAt(point.x(), point.y());
}

void MenuQt::PopupAsContextAt(unsigned int event_time, gfx::Point point)
{
  PopupAt(point);
}

  // Closes the menu.
void MenuQt::CloseMenu()
{
  window_->ReshowEmbededFlashWindow ();
}

#include "moc_menu_qt.cc"

