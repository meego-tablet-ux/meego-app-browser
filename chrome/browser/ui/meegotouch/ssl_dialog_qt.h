#ifndef SSL_DIALOG_QT_H
#define SSL_DIALOG_QT_H
#pragma once

#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "chrome/browser/ssl/ssl_blocking_page.h"
#include "chrome/browser/browser_list.h"

class SSLDialogQt;
class SSLDialogQtImpl;

class SSLDialogQt: public QObject
{
  public:
    explicit SSLDialogQt(BrowserWindowQt* browser);
    virtual ~SSLDialogQt();

    void CommandReceived(const std::string& command);
    void Show();
    void SetPageHandler(SSLBlockingPage* sslPage);
    void SetDetails(DictionaryValue* strings);
    void SetTabContentsHandler(TabContents* tab_contents)
	  {
		    tab_contents_ = tab_contents;
	  }
  private:
    BrowserWindowQt* window_;
    SSLBlockingPage* ssl_page_;
    SSLDialogQtImpl* impl_;
    TabContents* tab_contents_;
};

#endif // SSL_DIALOG_QT_H
