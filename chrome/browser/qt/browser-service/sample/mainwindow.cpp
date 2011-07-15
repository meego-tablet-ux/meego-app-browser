#include <QPixmap>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(&m_dataInfo, SIGNAL(browserClosed()), this, SLOT(browserClosed()));
    connect(&m_dataInfo, SIGNAL(browserLaunched()), this, SLOT(browserLaunched()));
    connect(&m_dataInfo, SIGNAL(tabInfoUpdated(int)), this, SLOT(tabInfoUpdated(int)));
    connect(&m_dataInfo, SIGNAL(tabListUpdated()), this, SLOT(tabListUpdated()));
    onBtnRefreshDBClicked();
}

MainWindow::~MainWindow()
{
}

void MainWindow::onBtnShowBrowserClicked()
{
    // Try to open page with url directly
    QString url = ui->lineEditURL->text();

    BrowserDataInfo::OpenMode mode =
            ui->radioBtnUrl->isChecked() ? BrowserDataInfo::URL_MODE :
            ui->radioBtnSearch->isChecked() ? BrowserDataInfo::SEARCH_MODE : BrowserDataInfo::TAB_MODE;

    m_dataInfo.openBrowser(mode, url);
}

void MainWindow::onBtnCloseTabClicked()
{
    int index = ui->spinBoxTabIndex->text().toInt();
    m_dataInfo.closeTab(index);
}

void MainWindow::onBtnUpdateCurrentTabClicked()
{
    m_dataInfo.updateCurrentTab();
}

void MainWindow::onBtnGetCurrentTabIndexClicked()
{
    int index = m_dataInfo.getCurrentTabIndex();
    ui->lineEditCurrentIndex->setText(QString::number(index));
}

void MainWindow::onBtnRefreshDBClicked()
{
    int size = m_dataInfo.getTabCount();
    this->ui->labelTabCount->setText(QString::number(size));
    ui->spinBoxTabIndex->setRange(0, size);
    //m_dataInfo.refreshTabList();
}

void MainWindow::onSpinBoxValueChanged(int index)
{
    if(index < 0 && index >= m_dataInfo.getTabCount())
      return;

    TabInfo* tabInfo = m_dataInfo.getTabAt(index);

    if(tabInfo) {
      this->ui->lineEditDbUrl->setText(tabInfo->url);
      this->ui->lineEditTitle->setText(tabInfo->title);
      this->ui->lineEditThumbnailPath->setText(tabInfo->thumbnail);
      this->ui->lineEditTabID->setText(QString::number(tabInfo->tab_id));

      QPixmap pm(ui->lineEditThumbnailPath->text());
      this->ui->labelImage->setPixmap(pm);
    }
}

// DBus signal handler
void MainWindow::browserClosed()
{
    ui->textEditSignalLog->insertPlainText("browser closed manager message handled\n");
}

void MainWindow::browserLaunched()
{
    ui->textEditSignalLog->insertPlainText("browser launched message handled\n");
}

void MainWindow::tabInfoUpdated(int tabid)
{
    QString log;
    log.sprintf("tab info updated message handled: %d\n", tabid);
    ui->textEditSignalLog->insertPlainText(log);
    //ui->spinBoxDBIndex->setValue(index);
    //onSpinBoxValueChanged(index);
}

void MainWindow::tabListUpdated()
{
    ui->textEditSignalLog->insertPlainText("tab list updated message handled\n");
    //onBtnRefreshDBClicked();
    int size = m_dataInfo.getTabCount();
    this->ui->labelTabCount->setText(QString::number(size));
    ui->spinBoxTabIndex->setRange(0, size);
}

