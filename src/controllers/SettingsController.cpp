#include "SettingsController.hpp"

namespace {
constexpr const char* KEY_RECENT_HSM = "recent/hsm";
constexpr const char* KEY_RECENT_WORKSPACES = "recent/workspaces";
}  // namespace

SettingsController::SettingsController() {
    loadRecentItems();
}

void SettingsController::loadRecentItems() {
    mRecentHsmFiles = mSettings.value(KEY_RECENT_HSM).toStringList();
    mRecentWorkspaces = mSettings.value(KEY_RECENT_WORKSPACES).toStringList();
}

void SettingsController::addRecentHsm(const QString& path) {
    addPathToRecentList(path, mRecentHsmFiles, KEY_RECENT_HSM);
}

void SettingsController::addRecentWorkspace(const QString& path) {
    addPathToRecentList(path, mRecentWorkspaces, KEY_RECENT_WORKSPACES);
}

void SettingsController::clearRecentHsm() {
    mRecentHsmFiles.clear();
    mSettings.remove(KEY_RECENT_HSM);
    mSettings.sync();
}

void SettingsController::clearRecentWorkspaces() {
    mRecentWorkspaces.clear();
    mSettings.remove(KEY_RECENT_WORKSPACES);
    mSettings.sync();
}

void SettingsController::removeRecentHsm(const QString& path) {
    removePathFromRecentList(path, mRecentHsmFiles, KEY_RECENT_HSM);
}

void SettingsController::removeRecentWorkspace(const QString& path) {
    removePathFromRecentList(path, mRecentWorkspaces, KEY_RECENT_WORKSPACES);
}

const QStringList& SettingsController::recentHsm() const {
    return mRecentHsmFiles;
}

const QStringList& SettingsController::recentWorkspaces() const {
    return mRecentWorkspaces;
}

void SettingsController::addPathToRecentList(const QString& path, QStringList& items, const QString& settingsKey) {
    if (path.isEmpty() == false) {
        items.removeAll(path);
        items.prepend(path);

        while (items.size() > mMaxEntries) {
            items.removeLast();
        }

        mSettings.setValue(settingsKey, items);
        mSettings.sync();
    }
}

void SettingsController::removePathFromRecentList(const QString& path, QStringList& items, const QString& settingsKey) {
    if (path.isEmpty() == false) {
        if (items.removeAll(path) > 0) {
            mSettings.setValue(settingsKey, items);
            mSettings.sync();
        }
    }
}
