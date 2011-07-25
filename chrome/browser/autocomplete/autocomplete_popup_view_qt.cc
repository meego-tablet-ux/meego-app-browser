// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include <algorithm>
#include <string>

#include "ui/base/resource/resource_bundle.h"
#include "base/basictypes.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/autocomplete/autocomplete.h"
#include "chrome/browser/autocomplete/autocomplete_edit.h"
#include "chrome/browser/autocomplete/autocomplete_edit_view_qt.h"
#include "chrome/browser/autocomplete/autocomplete_popup_model.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url.h"
#include "chrome/browser/search_engines/template_url_model.h"
#include "content/common/notification_service.h"
#include "grit/theme_resources.h"


#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"

#include <QObject>
#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeItem>
#include <QGraphicsLineItem>
#include <QVariant>
#include <QAbstractListModel>
#include <QList>

#include "chrome/browser/autocomplete/autocomplete_popup_view_qt.h"

// TODO(deanm): Find some better home for this, and make it more efficient.
size_t GetUTF8Offset(const std::wstring& wide_text, size_t wide_text_offset) {
  return WideToUTF8(wide_text.substr(0, wide_text_offset)).size();
}

class SuggestionItem
{
public:
  SuggestionItem(int icon, QString& url, QString& desc, int line):
    icon_(icon),
    url_(url),
    desc_(desc),
    line_(line)
    {
    }
  
  int icon() const 
  {
    return icon_;
  }

  QString url() const
  {
    return url_;
  }

  QString desc() const
  {
    return desc_;
  }
  
  int line() const
  {
    return line_;
  }
  
private:
  int icon_;
  QString url_;
  QString desc_;
  int line_;
};

class AutocompletePopupViewQtImpl : public QAbstractListModel
{
  Q_OBJECT
public:
  enum SuggestionRoles {
    IconRole = Qt::UserRole + 1,
    UrlRole,
    DescRole,
    LineRole
  };

  AutocompletePopupViewQtImpl(AutocompletePopupViewQt* popup_view, QObject *parent = 0):
    QAbstractListModel(parent),
    popup_view_(popup_view)
  {
    QHash<int, QByteArray> roles;
    roles[IconRole] = "icon";
    roles[UrlRole] = "url";
    roles[DescRole] = "desc";
    roles[LineRole] = "line";
     
    setRoleNames(roles);
  }

  void addSuggestion(const SuggestionItem &suggestion)
  {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_suggestionList << suggestion;
    endInsertRows();
  }

  void clear()
  {
    if (m_suggestionList.empty())
      return;
    beginRemoveRows(QModelIndex(), 0, m_suggestionList.size());
    m_suggestionList.clear();
    endRemoveRows();
  }

  int rowCount(const QModelIndex & parent = QModelIndex()) const
  {
    return m_suggestionList.count();
  }
   

  QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const
  {
    if (index.row() < 0 || index.row() > m_suggestionList.count())
      return QVariant();

    const SuggestionItem &suggestion = m_suggestionList[index.row()];
    if (role == IconRole)
      return suggestion.icon();
    else if (role == UrlRole)
      return suggestion.url();
    else if (role == DescRole)
      return suggestion.desc();
    else if (role == LineRole)
      return suggestion.line();
    return QVariant();
  }

  // call from AutocompletePopupViewQt
  void Show()
  {
    emit show();
  }
  
  void Hide() 
  {
    emit hide();
  }
  
signals:
  void show();
  void hide();
       
public slots:
  void openLine(int line)
  {
    popup_view_->AcceptLine(line, CURRENT_TAB);
  }
  
  
private:
  QList<SuggestionItem> m_suggestionList;
  AutocompletePopupViewQt* popup_view_;
};

AutocompletePopupViewQt::AutocompletePopupViewQt(const gfx::Font& font,
                                                 AutocompleteEditView* edit_view,
                                                 AutocompleteEditModel* edit_model,
                                                 Profile* profile,
                                                 BrowserWindowQt* window)
  : model_(new AutocompletePopupModel(this, edit_model, profile)),
    edit_view_(edit_view),
    impl_(new AutocompletePopupViewQtImpl(this)),
    opened_(false),
    window_(window)
{
  QDeclarativeView* view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("autocompletePopupViewModel", impl_);

}

AutocompletePopupViewQt::~AutocompletePopupViewQt() {
  model_.reset();
  delete impl_;
}

void AutocompletePopupViewQt::Init()
{
  // this is a hack to avoid passing pan events to under rwhv
  QDeclarativeView* view = window_->DeclarativeView();
  QDeclarativeItem* item = view->rootObject()->findChild<QDeclarativeItem*>("autocompletePopupView");
  if (item) {
    item->setFlag(QGraphicsItem::ItemIsPanel);
  }
}

void AutocompletePopupViewQt::InvalidateLine(size_t line) {
  DNOTIMPLEMENTED();
}

void AutocompletePopupViewQt::UpdatePopupAppearance() {
  const AutocompleteResult& result = model_->result();
  if (result.empty()) {
    Hide();
    return;
  }

  Show(result.size());
}

gfx::Rect AutocompletePopupViewQt::GetTargetBounds() {
  DNOTIMPLEMENTED();
}
void AutocompletePopupViewQt::PaintUpdatesNow() {
  DNOTIMPLEMENTED();
}

void AutocompletePopupViewQt::OnDragCanceled() {
  DNOTIMPLEMENTED();
}

AutocompletePopupModel* AutocompletePopupViewQt::GetModel() {
  return model_.get();
}

int AutocompletePopupViewQt::GetMaxYCoordinate() {
  DNOTIMPLEMENTED();
  return 0;
}

void AutocompletePopupViewQt::Observe(NotificationType type,
                                       const NotificationSource& source,
                                       const NotificationDetails& details) {
  DNOTIMPLEMENTED();
}

static const int kMaxSuggestionItems = 20;
static const int kMaxSuggestionTextLen = 200;
static const int kConnectorTextLen = 3; // " - "

void AutocompletePopupViewQt::Show(size_t num_results) {
  const AutocompleteResult& result = model_->result();
  
  impl_->clear();
  
  for (int line = 0; line < result.size() && line < kMaxSuggestionItems; line++) {
    int textLenRemain = 0;
    const AutocompleteMatch& match = result.match_at(line);
    
    QString content = QString::fromStdWString(UTF16ToWide(match.contents));
    QString desr = QString::fromStdWString(UTF16ToWide(match.description));
          
    DLOG(INFO) << "line : " << line;
    DLOG(INFO) << "content : " << match.contents;
    DLOG(INFO) << "desr : " << match.description;
  
    ///\todo: is there a better way to do
    if(content.size() >= kMaxSuggestionTextLen - kConnectorTextLen){
        content.truncate(kMaxSuggestionTextLen);
        desr.truncate(0);
    }
    else{
        textLenRemain = kMaxSuggestionTextLen - content.size() - kConnectorTextLen;
        desr.truncate(textLenRemain);
        desr = " - " + desr;
    }
    
    int icon = match.starred?
      IDR_OMNIBOX_STAR : AutocompleteMatch::TypeToIcon(match.type);

    ///\todo: avoid hard code the icon_id
    int icon_id;
    switch (icon) {
    case IDR_OMNIBOX_HTTP:
      icon_id = 0; break;
    case IDR_OMNIBOX_HISTORY:
      icon_id = 1; break;
    case IDR_OMNIBOX_SEARCH:
      icon_id = 2; break;
    case IDR_OMNIBOX_STAR:
      icon_id = 4; break;
    }
    impl_->addSuggestion(SuggestionItem(icon_id, content, desr, line));
  }
  DLOG(INFO) << "result size = " << result.size();
      
  // TODO: compose embeded flash window with correct rect
  gfx::Rect rect(0, 0, 0, 0);
  window_->ComposeEmbededFlashWindow(rect);

  impl_->Show();
  opened_ = true;
}

void AutocompletePopupViewQt::Hide() {
  impl_->Hide();
  window_->ReshowEmbededFlashWindow();
  opened_ = false;
}

void AutocompletePopupViewQt::AcceptLine(size_t line,
                                          WindowOpenDisposition disposition) {
  const AutocompleteMatch& match = model_->result().match_at(line);
  // OpenURL() may close the popup, which will clear the result set and, by
  // extension, |match| and its contents.  So copy the relevant strings out to
  // make sure they stay alive until the call completes.
  const GURL url(match.destination_url);
  string16 keyword;
  const bool is_keyword_hint = model_->GetKeywordForMatch(match, &keyword);
  edit_view_->OpenURL(url, disposition, match.transition, GURL(), line,
                      is_keyword_hint ? string16() : keyword);
}

#include "moc_autocomplete_popup_view_qt.cc"
