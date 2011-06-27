#ifndef SSL_DIALOG_QT_H
#define SSL_DIALOG_QT_H
#pragma once

#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "chrome/browser/ui/meegotouch/ssl_modal_dialog_qt.h"
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
    void SetModel(SSLAppModalDialog* model)
    {
        model_ = model;
    }
  private:
    BrowserWindowQt* window_;
    SSLDialogQtImpl* impl_;
    SSLAppModalDialog* model_;
};

#endif // SSL_DIALOG_QT_H
