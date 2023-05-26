#include "RecentFilesManager.hpp"

RecentFilesManager::RecentFilesManager(const QString& configPath, QMenu* menuRecentHsm, QMenu* menuRecentLogs,
                                       const std::function<void(const QString&)>& callbackOpenRecentHSM,
                                       const std::function<void(const QString&)>& callbackOpenRecentLog)
    : mConfigPath(configPath), mMaxEntries(10), mMenuRecentHsm(menuRecentHsm), mMenuRecentLogs(menuRecentLogs),
      mCallbackOpenRecentHSM(callbackOpenRecentHSM), mCallbackOpenRecentLog(callbackOpenRecentLog)
{
    loadRecentItems();
    updateMenuRecentHsm();
    updateMenuRecentLogs();
}

void RecentFilesManager::loadRecentItems()
{
    QSettings settings(mConfigPath, QSettings::IniFormat);
    settings.beginGroup("recent_hsm");
    QStringList recentItems = settings.childKeys();
    for (const QString& itemId : recentItems)
    {
        mRecentHsmFiles.append(settings.value(itemId).toString());
    }
    settings.endGroup();

    settings.beginGroup("recent_logs");
    recentItems = settings.childKeys();
    for (const QString& itemId : recentItems)
    {
        mRecentLogFiles.append(settings.value(itemId).toString());
    }
    settings.endGroup();
}

void RecentFilesManager::onActionClearRecentHSM()
{
    QSettings settings(mConfigPath, QSettings::IniFormat);
    settings.remove("recent_hsm");
    settings.sync();
    mRecentHsmFiles.clear();
    updateMenuRecentHsm();
}

void RecentFilesManager::onActionClearRecentLogs()
{
    QSettings settings(mConfigPath, QSettings::IniFormat);
    settings.remove("recent_logs");
    settings.sync();
    mRecentLogFiles.clear();
    updateMenuRecentLogs();
}

void RecentFilesManager::addPathToList(const QString& newPath, const QString& settingsGroup, QStringList& items,
                                       const std::function<void()>& updateFunc)
{
    QSettings settings(mConfigPath, QSettings::IniFormat);
    settings.beginGroup(settingsGroup);
    QStringList recentItems = settings.childKeys();
    items.clear();
    items.append(newPath);
    for (const QString& itemId : recentItems)
    {
        QString itemPath = settings.value(itemId).toString();
        if (itemPath != newPath)
        {
            items.append(settings.value(itemId).toString());
            if (items.size() >= mMaxEntries)
            {
                break;
            }
        }
    }
    settings.endGroup();
    settings.remove(settingsGroup);
    int index = 1;
    for (const QString& curItem : items)
    {
        settings.setValue(QString("%1/item%2").arg(settingsGroup).arg(index), curItem);
        index++;
    }
    settings.sync();
    updateFunc();
}

void RecentFilesManager::addPathToRecentHSMList(const QString& path)
{
    addPathToList(path, "recent_hsm", mRecentHsmFiles, [this]() { updateMenuRecentHsm(); });
}

void RecentFilesManager::addPathToRecentLogsList(const QString& path)
{
    addPathToList(path, "recent_logs", mRecentLogFiles, [this]() { updateMenuRecentLogs(); });
}

void RecentFilesManager::updateMenuItems(QMenu* menu, const QStringList& items,
                                         const std::function<void(const QString&)>& callbackItem,
                                         const std::function<void()>& callbackClear)
{
    menu->clear();
    if (!items.isEmpty())
    {
        menu->setEnabled(true);
        for (const QString& curPath : items)
        {
            QAction* entryAction = new QAction(curPath, menu);
            connect(entryAction, &QAction::triggered, [this, callbackItem, curPath]() {
                callbackItem(curPath);
            });
            menu->addAction(entryAction);
        }
        menu->addSeparator();
        QAction* clearAction = menu->addAction("Clear Menu");
        connect(clearAction, &QAction::triggered, callbackClear);
    }
    else
    {
        menu->setEnabled(false);
    }
}

void RecentFilesManager::updateMenuRecentHsm()
{
    updateMenuItems(mMenuRecentHsm, mRecentHsmFiles, mCallbackOpenRecentHSM, [this]() {
        onActionClearRecentHSM();
    });
}

void RecentFilesManager::updateMenuRecentLogs()
{
    updateMenuItems(mMenuRecentLogs, mRecentLogFiles, mCallbackOpenRecentLog, [this]() {
        onActionClearRecentLogs();
    });
}
