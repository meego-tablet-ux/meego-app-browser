#include <QApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDebug>
#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDesktopWidget>
#include <QFile>
#include <QFileInfo>
#include <QX11Info>

#include "webkit/glue/hwfmenu_qt.h"
#include "webkit/glue/mainhwfqml.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>

MainhwfQml::MainhwfQml(void *arg, QApplication *app, QWidget *parent) :
    QWidget(parent)
{
  CallFMenuClass *qml_ctrl = (CallFMenuClass *)arg;
  Papp = app;

  qmlView = new QDeclarativeView(this);
  connect(qmlView->engine(), SIGNAL(quit()),this, SLOT(handleChange()));

  qmlView->rootContext()->setContextProperty("fmenuObject", qml_ctrl);

  QString mainQml = QString("meego-app-browser/") + "HwMediaUxMain.qml";
  QString sharePath;

  if (QFile::exists(mainQml))
  {
    sharePath = QDir::currentPath() + "/";
  }
  else
  {
    sharePath = QString("/usr/share/");
    if (!QFile::exists(sharePath + mainQml))
    {
      qFatal("%s does not exist!", mainQml.toUtf8().data());
    }
  }

  qmlView->setSource(QUrl(sharePath + mainQml));  // Should locate HwMediaUxMain.qml and icons from "meego-app-browser"
  qmlView->raise();
  qmlView->setAttribute(Qt::WA_NoSystemBackground, true);
  qmlView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
  qmlView->show();

}


MainhwfQml::~MainhwfQml()
{

}


#include "webkit/glue/moc_mainhwfqml.cc"
