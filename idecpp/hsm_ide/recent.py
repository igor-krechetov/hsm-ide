# hsm-ide project
# Copyright (C) 2022 Igor Krechetov
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from PySide6.QtCore import QSettings
from PySide6.QtGui import QAction

class RecentFilesManager():
    def __init__(self, configPath, menuRecentHsm, menuRecentLogs, callbackOpenRecentHSM, callbackOpenRecentLog):
        self.configPath = configPath
        self.maxEntries = 10
        self.menuRecentHsm = menuRecentHsm
        self.menuRecentLogs = menuRecentLogs
        self.callbackOpenRecentHSM = callbackOpenRecentHSM
        self.callbackOpenRecentLog = callbackOpenRecentLog
        self.recentHsmFiles = []
        self.recentLogFiles = []
        self.loadRecentItems()
        self.updateMenuRecentHsm()
        self.updateMenuRecentLogs()

    def loadRecentItems(self):
        settings = QSettings(self.configPath, QSettings.IniFormat)
        settings.beginGroup("recent_hsm")
        recentItems = settings.childKeys()
        for itemId in recentItems:
            self.recentHsmFiles.append(settings.value(itemId))
        settings.endGroup()
        settings.beginGroup("recent_logs")
        recentItems = settings.childKeys()
        for itemId in recentItems:
            self.recentLogFiles.append(settings.value(itemId))
        settings.endGroup()

    def onActionClearRecentHSM(self):
        settings = QSettings(self.configPath, QSettings.IniFormat)
        settings.remove("recent_hsm")
        settings.sync()
        self.recentHsmFiles.clear()
        self.updateMenuRecentHsm()

    def onActionClearRecentLogs(self):
        settings = QSettings(self.configPath, QSettings.IniFormat)
        settings.remove("recent_logs")
        settings.sync()
        self.recentLogFiles.clear()
        self.updateMenuRecentLogs()

    def addPathToList(self, newPath, settingsGroup, items, updateFunc):
        settings = QSettings(self.configPath, QSettings.IniFormat)
        settings.beginGroup(settingsGroup)
        recentItems = settings.childKeys()
        items.clear()
        items.append(newPath)
        for itemId in recentItems:
            itemPath = settings.value(itemId)
            if itemPath != newPath:
                items.append(settings.value(itemId))
                if len(items) >= self.maxEntries:
                    break
        settings.endGroup()
        settings.remove(settingsGroup)
        index = 1
        for curItem in items:
            settings.setValue(f"{settingsGroup}/item{index}", curItem)
            index += 1
        settings.sync()
        updateFunc()

    def addPathToRecentHSMList(self, path):
        self.addPathToList(path, "recent_hsm", self.recentHsmFiles, self.updateMenuRecentHsm)

    def addPathToRecentLogsList(self, path):
        self.addPathToList(path, "recent_logs", self.recentLogFiles, self.updateMenuRecentLogs)

    def updateMenuItems(self, menu, items, callbackItem, callbackClear):
        menu.clear()
        if len(items) > 0:
            menu.setEnabled(True)
            for curPath in items:
                entryAction = QAction(curPath, menu)
                entryAction.triggered.connect(callbackItem)
                menu.addAction(entryAction)
            menu.addSeparator()
            menu.addAction("Clear Menu").triggered.connect(callbackClear)
        else:
            menu.setEnabled(False)

    def updateMenuRecentHsm(self):
        self.updateMenuItems(self.menuRecentHsm,
                             self.recentHsmFiles,
                             self.callbackOpenRecentHSM,
                             self.onActionClearRecentHSM)

    def updateMenuRecentLogs(self):
        self.updateMenuItems(self.menuRecentLogs,
                             self.recentLogFiles,
                             self.callbackOpenRecentLog,
                             self.onActionClearRecentLogs)
