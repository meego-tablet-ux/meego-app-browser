#ifndef BROWSERDATAINFO_H
#define BROWSERDATAINFO_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "TabManagerInterface.h"

struct TabInfo {
   int tab_id;
   int win_id;
   QString url;
   QString title;
   QString thumbnail;
};

// Wrapper of tab manager in browser
class BrowserDataInfo: public QObject
{
    Q_OBJECT

public:
    enum OpenMode {
        URL_MODE,
        SEARCH_MODE,
        TAB_MODE
    };
    BrowserDataInfo();
    virtual ~BrowserDataInfo();
    // Get tab count opened in browser
    int getTabCount();
    // Get current tab selected in browser
    int getCurrentTabIndex() const;
    // Get tab info with the given index. If index is invalid, NULL is return
    // Should be freed manually when it is no use
    TabInfo* getTabAt(int index);
    // Close tab at the given index. If index is invalid, return false.
    bool closeTab(int index);
    // Open browser in the given mode with the given target.
    // If the browser is opened as URL_MODE, the target is parsed as a URL
    // If the browser is opened as SEARCH_MODE, the target is parsed as a
    // search keyword which will be searched with browser search engine.
    void openBrowser(OpenMode mode, const QString& target);
    // Update the URL, title and thumbnail of current tab.
    void updateCurrentTab();
    // Get the list of TabInfo object.
    const QList<TabInfo> & getDataInfoList() const;
    // refresh tab list
    void refreshTabList();

public Q_SLOTS:
    // DBus singal handler
    void dbusBrowserClosed();
    void dbusBrowserLaunched();
    void dbusTabInfoUpdated(int tabid);
    void dbusTabListUpdated();

Q_SIGNALS:
    // emit when browser is lanched
    void browserLaunched();
   // emit when browser is closed
    void browserClosed();
   // emit when tab info is changed, such as url changed.
    void tabInfoUpdated(int index);
   // emit when a new tab is opened or a tab is closed/changed
    void tabListUpdated();
protected:
    QSqlDatabase m_db;
    TabManagerInterface * m_interface;
    QList<TabInfo> m_dataInfoList;
    int m_showTabWithIndex;
    bool isBrowserRunning() const;
    void loadData();
    const QString getPicPath(const QString& s);
};


#endif // BROWSERDATAINFO_H
