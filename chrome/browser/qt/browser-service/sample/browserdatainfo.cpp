#include <QCryptographicHash>
#include "browserdatainfo.h"


#define BROWSER_SERVICE "com.meego.browser.BrowserService"
#define BROWSER_SERVICE_PATH "/com/meego/browser/BrowserService"

static const QString DB_PATH(QDir::homePath() + QDir::separator() + ".config/internet-panel/chromium.db");

BrowserDataInfo::BrowserDataInfo()
    : m_showTabWithIndex(-1)
{
    m_interface = new TabManagerInterface(BROWSER_SERVICE, BROWSER_SERVICE_PATH, QDBusConnection::sessionBus(), this);
    connect(m_interface, SIGNAL(browserClosed()), this, SLOT(dbusBrowserClosed()));
    connect(m_interface, SIGNAL(browserLaunched()), this, SLOT(dbusBrowserLaunched()));
    connect(m_interface, SIGNAL(tabInfoUpdated(int)), this, SLOT(dbusTabInfoUpdated(int)));
    connect(m_interface, SIGNAL(tabListUpdated()), this, SLOT(dbusTabListUpdated()));

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(DB_PATH);
    if (m_db.open()) {
        loadData();
    }
}

BrowserDataInfo::~BrowserDataInfo()
{
    if (m_db.isOpen())
        m_db.close();
    delete m_interface;
}

int BrowserDataInfo::getTabCount()
{
    return m_dataInfoList.size();
}

TabInfo* BrowserDataInfo::getTabAt(int index)
{
    if (index < 0 || index >= m_dataInfoList.size())
        return NULL;
    else
        return &m_dataInfoList[index];
}

int BrowserDataInfo::getCurrentTabIndex() const
{
    int index(-1);
    index = m_interface->getCurrentTabIndex().value();
    return index;
}

bool BrowserDataInfo::closeTab(int index)
{
    m_interface->closeTab(index);
    return true;
}

void BrowserDataInfo::openBrowser(BrowserDataInfo::OpenMode mode, const QString &target)
{
    QString browser_path;
    QStringList arguments;

    char* env = getenv("BROWSER");
    if(env) browser_path = env;
    else browser_path = "/usr/bin/meego-app-browser";

    if(isBrowserRunning()) {
        m_interface->showBrowser(mode == URL_MODE ? "gotourl" :
                                 mode == SEARCH_MODE ? "search" : "selecttab", target);
    }else {
        if (mode == BrowserDataInfo::URL_MODE)
            arguments.append(target);
        else if (mode == BrowserDataInfo::SEARCH_MODE)
            arguments.append("? " + target);
        else {
            foreach(TabInfo i, m_dataInfoList) {
              arguments << i.url;
            }
            m_showTabWithIndex = target.toInt();
        }
    }
    QProcess::startDetached(browser_path, arguments);
}

void BrowserDataInfo::updateCurrentTab()
{
    m_interface->updateCurrentTab();
}

bool BrowserDataInfo::isBrowserRunning() const
{
        bool result = false;
        QDBusConnection bus = QDBusConnection::sessionBus();
        QDBusInterface iface("org.freedesktop.DBus", "/org/freedesktop/DBus",
                             "org.freedesktop.DBus", bus);

        QDBusReply<QStringList> reply = iface.call("ListNames");
        if(reply.isValid()) {
                QStringList names = reply.value();
                if(names.contains(BROWSER_SERVICE)) result = true;
        }
        else
                qDebug() << reply.error();

        return result;
}

void BrowserDataInfo::dbusBrowserClosed()
{
    emit browserClosed();
}

void BrowserDataInfo::dbusBrowserLaunched()
{
    if (m_showTabWithIndex > -1) {
        m_interface->showBrowser("selecttab", QString::number(m_showTabWithIndex).toUtf8());
        m_showTabWithIndex = -1;
    }
    emit browserLaunched();
}

void BrowserDataInfo::dbusTabInfoUpdated(int index)
{
    if (index < 0 || index >= m_dataInfoList.size())
        return;

    if (m_db.isOpen()) {
        QSqlQuery query;
        QString sqlString;
        sqlString.sprintf("select * from current_tabs where tab_id=%d", index);
        query.exec(sqlString);
        query.next();

        TabInfo & tabinfo = m_dataInfoList[index];
        tabinfo.tab_id = index;
        tabinfo.url = query.value(3).toString();
        tabinfo.title = query.value(4).toString();
        tabinfo.thumbnail = getPicPath(tabinfo.url);
        tabinfo.win_id = 0;
    }

    emit tabInfoUpdated(index);
}

void BrowserDataInfo::dbusTabListUpdated()
{
    loadData();
    emit tabListUpdated();
}

const QList<TabInfo>& BrowserDataInfo::getDataInfoList() const
{
    return m_dataInfoList;
}

const QString BrowserDataInfo::getPicPath(const QString& s){
    QByteArray ba = s.toUtf8();
    QByteArray md5 = QCryptographicHash::hash(ba, QCryptographicHash::Md5);
    QString csum("");
    for (int i = 0; i < md5.size(); i ++)
        csum += QString("%1").arg((int)(md5[i]&0xFF), 2,16, QLatin1Char('0'));

    QString thumbnailsPath = QDir::homePath() + "/.config/internet-panel/thumbnails/" + csum + ".jpg";
    return thumbnailsPath;
}

void BrowserDataInfo::loadData()
{
  if(!m_db.isOpen())
    m_db.open();

  QString sqlString("select * from current_tabs order by tab_id");
  QSqlQuery query(sqlString, m_db);

  m_dataInfoList.clear();
  while (query.next()) {
    TabInfo tabinfo;
    tabinfo.tab_id = query.value(1).toInt();
    tabinfo.win_id = query.value(2).toInt();
    tabinfo.url = query.value(3).toString();
    tabinfo.title = query.value(4).toString();
    tabinfo.thumbnail = getPicPath(tabinfo.url);

    m_dataInfoList.append(tabinfo);
  }
  query.finish();
}
