#include "chrome/browser/ui/meegotouch/ssl_dialog_qt.h"

#include "base/utf_string_conversions.h"

#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QString>

class SSLDialogQtImpl: public QObject
{
  Q_OBJECT
  public:
    SSLDialogQtImpl(SSLDialogQt* dialog):
        QObject(NULL),
        dialog_(dialog),
        strings_(NULL)
    {
    }

    void SetDetails(DictionaryValue* strings){ 
		    strings_ = strings;
	  }

    void Show() { 
        string16 headline, description, moreInfo, buttonYes, buttonNo;
        strings_->GetString("headLine", &headline);
		    strings_->GetString("description", &description);
		    strings_->GetString("moreInfoTitle", &moreInfo);
        bool error;
        if(strings_->GetString("proceed", &buttonYes) && strings_->GetString("exit", &buttonNo))
            error = false;
        else
        {
            error=true;
            strings_->GetString("back", &buttonNo);
        }
        emit show(QString::fromStdWString(UTF16ToWide(headline)), QString::fromStdWString(UTF16ToWide(description)), 
			      QString::fromStdWString(UTF16ToWide(moreInfo)), QString::fromStdWString(UTF16ToWide(buttonYes)), 
				        QString::fromStdWString(UTF16ToWide(buttonNo)), error);
    }

    void Hide() { emit hide(); }

  public slots:
    void yesButtonClicked()
    {
        DLOG(INFO)<<"SSL: YES";
        //That`s important for hide() before CommandReceived()
        //or if there are more than one SSL_Dialog in DialogQueue
        //they won`t show correct
        emit hide();
        dialog_->CommandReceived("1");
    }
    void noButtonClicked()
    {
        DLOG(INFO)<<"SSL: NO";
        //That`s important for hide() before CommandReceived()
        //or if there are more than one SSL_Dialog in DialogQueue
        //they won`t show correct
        emit hide();
        dialog_->CommandReceived("0");
    }
  Q_SIGNALS:
    void show(QString headline, QString description, QString moreInfo, QString buttonYes, QString buttonNo, bool error);
    void hide();
  private:
    SSLDialogQt* dialog_;
    
    //details texts in the ssl dialog
    DictionaryValue* strings_;
};

SSLDialogQt::SSLDialogQt(BrowserWindowQt* window):
    window_(window),
    impl_(new SSLDialogQtImpl(this))
{
    QDeclarativeView* view = window_->DeclarativeView();
    QDeclarativeContext *context = view->rootContext();
    context->setContextProperty("sslDialogModel", impl_);
}

SSLDialogQt::~SSLDialogQt()
{
    delete impl_;
}

void SSLDialogQt::CommandReceived(const std::string& command)
{
    model_->ProcessCommand(command);
}

void SSLDialogQt::Show()
{
    impl_->SetDetails(model_->GetDetails());
    impl_->Show();
}

#include "moc_ssl_dialog_qt.cc"
