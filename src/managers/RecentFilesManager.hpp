#ifndef RECENTFILESMANAGER_HPP
#define RECENTFILESMANAGER_HPP

#include <QStringList>
#include <QSettings>
#include <QAction>
#include <QMenu>

class RecentFilesManager
{
public:
    RecentFilesManager(const QString& configPath, QMenu* menuRecentHsm, QMenu* menuRecentLogs,
                       const std::function<void(const QString&)>& callbackOpenRecentHSM,
                       const std::function<void(const QString&)>& callbackOpenRecentLog);
    virtual ~RecentFilesManager() = default;

    void loadRecentItems();
    void onActionClearRecentHSM();
    void onActionClearRecentLogs();
    void addPathToRecentHSMList(const QString& path);
    void addPathToRecentLogsList(const QString& path);

private:
    void addPathToList(const QString& newPath, const QString& settingsGroup, QStringList& items,
                       const std::function<void()>& updateFunc);
    void updateMenuItems(QMenu* menu, const QStringList& items,
                         const std::function<void(const QString&)>& callbackItem,
                         const std::function<void()>& callbackClear);
    void updateMenuRecentHsm();
    void updateMenuRecentLogs();

    QString mConfigPath;
    int mMaxEntries;
    QMenu* mMenuRecentHsm;
    QMenu* mMenuRecentLogs;
    std::function<void(const QString&)> mCallbackOpenRecentHSM;
    std::function<void(const QString&)> mCallbackOpenRecentLog;
    QStringList mRecentHsmFiles;
    QStringList mRecentLogFiles;
};

#endif // RECENTFILESMANAGER_HPP
