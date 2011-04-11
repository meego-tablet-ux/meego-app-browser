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

#include <QAbstractListModel>
#include <QDeclarativeView>
#include <QDeclarativeContext>

#include "chrome/browser/browser_window.h"

#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "webkit/glue/webmenuitem.h"
#include "chrome/browser/ui/meegotouch/popup_list_qt.h"
#include "chrome/browser/tab_contents/tab_contents_view_qt.h"

class PopupListItem {
 public:

   PopupListItem(QString label, int type);
   ~PopupListItem();
   QString label() const { return label_; }
   int type() const { return type_; }

 private:
  QString label_;
  int type_;
  
};

PopupListItem::PopupListItem(QString label, int type)
{
  label_ = label;
  type_ = type;
}

PopupListItem::~PopupListItem()
{
}

class PopupListQtImpl : public QAbstractListModel
{
  Q_OBJECT

 public:

  enum ListRoles {
    TypeRole = Qt::UserRole + 1,
    LabelRole
  };

  PopupListQtImpl(PopupListQt *popup_list, QObject *parent = 0);
  ~PopupListQtImpl();

  void beginAddItem() { beginResetModel(); }
  void endAddItem() {endResetModel(); }
  void addItem(QString item, int type);
  void clear();

  void setSelectedIndex(int index) {selected_index_ = index;}
  void show(int hx, int hy, int hw, int hh) { Q_EMIT showPopup(hx, hy, hw, hh); }
  void hide() { Q_EMIT hidePopup(); }

  Q_INVOKABLE int currentSelectedItem();

  ///reimp begin
  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
  ///reimp end

public Q_SLOTS:
  void itemInvoked(int index);
  void uiCanceled();

Q_SIGNALS:
  void showPopup(int hx, int hy, int hw, int hh);
  void hidePopup();

private:
  QList <PopupListItem *> popup_item_list_;  
  int selected_index_;
  PopupListQt *popup_list_;
};

PopupListQtImpl::PopupListQtImpl(PopupListQt *popup_list, QObject *parent):
  popup_list_(popup_list),
  QAbstractListModel(parent)
{
  QHash<int, QByteArray> roles;
  roles[TypeRole] = "type";
  roles[LabelRole] = "label";
  setRoleNames(roles);
}

PopupListQtImpl::~PopupListQtImpl()
{
  hide();
  clear();
}

void PopupListQtImpl::addItem(QString label, int type)
{
  PopupListItem *item = new PopupListItem(label, type);
  popup_item_list_.append(item);
}


void PopupListQtImpl::clear()
{
  beginResetModel();

  while (!popup_item_list_.isEmpty()) {
    delete popup_item_list_.takeFirst();
  }

  popup_item_list_.clear();
  endResetModel();
}

void PopupListQtImpl::itemInvoked(int index)
{
  if (index < 0 || index >= popup_item_list_.count()) {
    DLOG(ERROR) << "index out of scope";
    return;
  }

  popup_list_->currentView()->selectPopupItem(index);
  hide();
  
  //DNOTIMPLEMENTED();
}

void PopupListQtImpl::uiCanceled()
{
  popup_list_->currentView()->selectPopupItem(-1);
  // we don't need to hide UI, since ui already been canceled.
}

int PopupListQtImpl::currentSelectedItem()
{
  return selected_index_;
}

int PopupListQtImpl::rowCount(const QModelIndex &parent) const
{
  return popup_item_list_.count();
}

QVariant PopupListQtImpl::data(const QModelIndex & index, int role) const
{
  if (index.row() < 0 || index.row() > popup_item_list_.count())
    return QVariant();

  const PopupListItem *item = popup_item_list_[index.row()];

  if (role == TypeRole)
    return item->type();
  else if (role == LabelRole)
    return item->label();

  return QVariant();
}



PopupListQt::PopupListQt(BrowserWindowQt* window)
    : window_(window),
      view_(NULL),
      header_bounds_(QRect(0, 0, 0, 0)),
      impl_(new PopupListQtImpl(this)){

  QDeclarativeView* view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("PopupListModel", impl_);
}

PopupListQt::~PopupListQt() {
  delete impl_;
}

void PopupListQt::PopulateMenuItemData(int selected_item, const  std::vector < WebMenuItem > & items)
{
  impl_->clear();

  std::vector<WebMenuItem>::const_iterator iter;

  impl_->beginAddItem();
  for(iter = items.begin(); iter != items.end(); ++iter) {
    DLOG(INFO) << "-- popup item details "
      << ",label:" << iter->label
      << ",type: " << iter->type
      << ",enabled: " << iter->enabled
      << ",checked: " << iter->checked;

    impl_->addItem(QString::fromStdWString(UTF16ToWide(iter->label)), iter->type);
  }

  impl_->endAddItem();

  if (items.size() >= selected_item)
    impl_->setSelectedIndex(selected_item);
  else
    impl_->setSelectedIndex(-1);
}

void PopupListQt::SetHeaderBounds(gfx::Rect bounds)
{
  DLOG(INFO) << "bounding x-y:width-heigt = "
    << bounds.x() << "-"
    << bounds.y() << ":"
    << bounds.width() << "-"
    << bounds.height();

  header_bounds_ = QRect(bounds.x(), bounds.y(), bounds.width(), bounds.height());

};

void PopupListQt::show()
{
  QDeclarativeView* view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("PopupListModel", impl_);

  impl_->show(header_bounds_.x(), header_bounds_.y(), header_bounds_.width(), header_bounds_.height());
}

#include "moc_popup_list_qt.cc"
