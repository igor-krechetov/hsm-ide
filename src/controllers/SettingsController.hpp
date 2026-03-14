#ifndef SETTINGSCONTROLLER_HPP
#define SETTINGSCONTROLLER_HPP

#include <QSettings>
#include <QStringList>

class SettingsController {
public:
    SettingsController();
    virtual ~SettingsController() = default;

    void addRecentHsm(const QString& path);
    void addRecentWorkspace(const QString& path);
    void clearRecentHsm();
    void clearRecentWorkspaces();
    void removeRecentHsm(const QString& path);
    void removeRecentWorkspace(const QString& path);

    const QStringList& recentHsm() const;
    const QStringList& recentWorkspaces() const;

private:
    void loadRecentItems();
    void addPathToRecentList(const QString& path, QStringList& items, const QString& settingsKey);
    void removePathFromRecentList(const QString& path, QStringList& items, const QString& settingsKey);

private:
    int mMaxEntries = 10;
    QSettings mSettings;
    QStringList mRecentHsmFiles;
    QStringList mRecentWorkspaces;
};

#endif  // SETTINGSCONTROLLER_HPP
