#ifndef MAINHWFQMLWINDOW_H
#define MAINHWFQMLWINDOW_H

#include <X11/X.h>

class QDeclarativeView;

class MainhwfQml : public QWidget
{
    Q_OBJECT

public:
  MainhwfQml(void *arg, QApplication *app,QWidget *parent = NULL);
  ~MainhwfQml();

  QDeclarativeView* getDeclarativeView() {return qmlView;}
  Window subwindow;

public Q_SLOTS:
  void handleChange()
  {
    qmlView->close();
    Papp->quit();
  }

private:
  QApplication *Papp;
  QDeclarativeView *qmlView;
};

#endif // MAINHWFQMLWINDOW_H
