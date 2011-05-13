#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include "browserdatainfo.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:    
    // UI slots
    void onBtnShowBrowserClicked();
    void onBtnCloseTabClicked();
    void onBtnUpdateCurrentTabClicked();
    void onBtnGetCurrentTabIndexClicked();
    void onBtnRefreshDBClicked();
    void onSpinBoxValueChanged(int index);

    // DBus singal handler
    void browserClosed();
    void browserLaunched();
    void tabInfoUpdated(int index);
    void tabListUpdated();

private:
    Ui::MainWindow *ui;
    BrowserDataInfo m_dataInfo;
};

#endif // MAINWINDOW_H
