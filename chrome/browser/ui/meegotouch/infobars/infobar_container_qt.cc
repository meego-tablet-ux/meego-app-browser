// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QAbstractListModel>
#include <QDeclarativeView>
#include <QDeclarativeContext>

#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/tab_contents/infobar_delegate.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/common/notification_service.h"

#include "chrome/browser/ui/meegotouch/infobars/infobar_container_qt.h"
#include "chrome/browser/ui/meegotouch/infobars/infobar_qt.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"

class InfoBarContainerQtImpl : public QAbstractListModel
{
  Q_OBJECT
  Q_ENUMS(ButtonType)

 public:
  enum SuggestionRoles {
    TypeRole = Qt::UserRole + 1,
    ButtonsRole,
    InfoRole,
    AcceptLabelRole,
    CancelLabelRole
  };

  InfoBarContainerQtImpl(InfoBarContainerQt* container, QObject *parent = 0);

  static InfoBar::ButtonType getButtonType(QString str);

  void addInfobar(InfoBar *infobar);
  void removeInfobarByDelegate(const InfoBarDelegate* delegate);
  void clear();

  ///reimp begin
  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
  ///reimp end

Q_SIGNALS:
  void show();

public Q_SLOTS:
  void infobarInvoked(int index, QString button);

private:
  QList<InfoBar *> infobar_item_list_;
  InfoBarContainerQt* container_;
};


InfoBarContainerQtImpl::InfoBarContainerQtImpl(InfoBarContainerQt* container, QObject *parent):
  QAbstractListModel(parent),
  container_(container)
{
  QHash<int, QByteArray> roles;
  roles[TypeRole] = "type";
  roles[ButtonsRole] = "buttons";
  roles[InfoRole] = "info";
  roles[AcceptLabelRole] = "acceptLabel";
  roles[CancelLabelRole] = "cancelLabel";

  setRoleNames(roles);
}

InfoBar::ButtonType InfoBarContainerQtImpl::getButtonType(QString str)
{
  if (str == QString("ButtonAccept"))
    return InfoBar::ButtonAccept;
  else if (str == QString("ButtonCancel"))
    return InfoBar::ButtonCancel;
  else if (str == QString("ButtonOKDefault"))
    return InfoBar::ButtonOKDefault;
  else if (str == QString("ButtonClose"))
    return InfoBar::ButtonClose;
  else
    return InfoBar::ButtonNone;
}

void InfoBarContainerQtImpl::addInfobar(InfoBar *infobar)
{
  if (rowCount() == 0) {
    emit show();
  }
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  infobar_item_list_ << infobar;
  endInsertRows();
}

void InfoBarContainerQtImpl::removeInfobarByDelegate(const InfoBarDelegate* delegate)
{
  int index = -1;
  for (int i = 0; i<infobar_item_list_.count(); i++)
  {
    InfoBar *item = infobar_item_list_.at(i);
    if (item->delegate() == delegate ) {
      item->Close();
      index = i;
    }
  }

  if (index >= 0) {
    beginRemoveRows(QModelIndex(), index, index);
    infobar_item_list_.removeAt(index);
    endRemoveRows();
  }
}

void InfoBarContainerQtImpl::clear()
{
  beginResetModel();
  infobar_item_list_.clear();
  endResetModel();
}

int InfoBarContainerQtImpl::rowCount(const QModelIndex &parent) const
{
  return infobar_item_list_.count();
}

QVariant InfoBarContainerQtImpl::data(const QModelIndex & index, int role) const
{
  if (index.row() < 0 || index.row() > infobar_item_list_.count())
    return QVariant();

  const InfoBar *infobaritem = infobar_item_list_[index.row()];
  if (role == TypeRole)
    return infobaritem->type();
  else if (role == ButtonsRole)
    return infobaritem->buttons();
  else if (role == InfoRole)
    return infobaritem->text();
  else if (role == AcceptLabelRole)
    return infobaritem->acceptLabel();
  else if (role == CancelLabelRole)
    return infobaritem->cancelLabel();
  return QVariant();
}

void InfoBarContainerQtImpl::infobarInvoked(int index, QString button)
{
  if (index < 0 || index >= infobar_item_list_.count()) {
    DLOG(ERROR) << "index out of scope";
    return;
  }

  InfoBar::ButtonType btn_type = getButtonType(button);
  InfoBar *infobar = infobar_item_list_.at(index);

  infobar->ProcessButtonEvent(btn_type);
}



InfoBarContainerQt::InfoBarContainerQt(Profile* profile, BrowserWindowQt* window)
    : profile_(profile),
      window_(window),
      tab_contents_(NULL),
      impl_(new InfoBarContainerQtImpl(this)){

  QDeclarativeView* view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("infobarContainerModel", impl_);

}

InfoBarContainerQt::~InfoBarContainerQt() {
  ChangeTabContents(NULL);
  delete impl_;
}

void InfoBarContainerQt::ChangeTabContents(TabContents* contents) {
  if (tab_contents_)
    registrar_.RemoveAll();

  impl_->clear();

  tab_contents_ = contents;
  if (tab_contents_) {
    UpdateInfoBars();
    Source<TabContents> source(tab_contents_);
    registrar_.Add(this, NotificationType::TAB_CONTENTS_INFOBAR_ADDED, source);
    registrar_.Add(this, NotificationType::TAB_CONTENTS_INFOBAR_REMOVED,
                   source);
    registrar_.Add(this, NotificationType::TAB_CONTENTS_INFOBAR_REPLACED,
                   source);
  }
}

void InfoBarContainerQt::RemoveDelegate(InfoBarDelegate* delegate) {
  tab_contents_->RemoveInfoBar(delegate);
}

int InfoBarContainerQt::TotalHeightOfAnimatingBars() const {
  DNOTIMPLEMENTED();

  int sum = 0;
  return sum;
}

// InfoBarContainerQt, NotificationObserver implementation: -------------------

void InfoBarContainerQt::Observe(NotificationType type,
                                  const NotificationSource& source,
                                  const NotificationDetails& details) {
  if (type == NotificationType::TAB_CONTENTS_INFOBAR_ADDED) {
    AddInfoBar(Details<InfoBarDelegate>(details).ptr(), true);
  } else if (type == NotificationType::TAB_CONTENTS_INFOBAR_REMOVED) {
    RemoveInfoBar(Details<InfoBarDelegate>(details).ptr(), true);
  } else if (type == NotificationType::TAB_CONTENTS_INFOBAR_REPLACED) {
    std::pair<InfoBarDelegate*, InfoBarDelegate*>* delegates =
        Details<std::pair<InfoBarDelegate*, InfoBarDelegate*> >(details).ptr();

    // By not animating the removal/addition, this appears to be a replace.
    RemoveInfoBar(delegates->first, false);
    AddInfoBar(delegates->second, false);
  } else {
    NOTREACHED();
  }
}

// InfoBarContainerQt, private: -----------------------------------------------

void InfoBarContainerQt::UpdateInfoBars() {
  for (int i = 0; i < tab_contents_->infobar_count(); ++i) {
    InfoBarDelegate* delegate = tab_contents_->GetInfoBarDelegateAt(i);
    AddInfoBar(delegate, false);
  }
}

void InfoBarContainerQt::AddInfoBar(InfoBarDelegate* delegate, bool animate) {
  InfoBar* infobar = delegate->CreateInfoBar();
  if (infobar == NULL) return;

  infobar->set_container(this);
  impl_->addInfobar(infobar);
  if (animate)
    infobar->AnimateOpen();
  else
    infobar->Open();
}

void InfoBarContainerQt::RemoveInfoBar(InfoBarDelegate* delegate,
                                        bool animate) {
  impl_->removeInfobarByDelegate(delegate);
}

#include "moc_infobar_container_qt.cc"
