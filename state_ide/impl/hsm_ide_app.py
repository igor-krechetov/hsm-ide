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

# This Python file uses the following encoding: utf-8
import sys
import os
import yaml
import importlib.util
import subprocess
import threading
import hashlib
from pathlib import Path

# from impl.qclickableslider import QClickableSlider
# from impl.qimageviewarea import QImageViewArea
# from impl.recent import RecentFilesManager
# from impl.search import QFramesSearchModel
from impl.utils import utils
from impl.settings import QSettingsDialog

from impl.widgets.HsmGraphicsView import HsmGraphicsView
from impl.widgets.HsmElementsList import HsmElementsList
from impl.elements import HsmStateElement, HsmTransition, HsmElementsFactory

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QGraphicsScene, QApplication, QMessageBox, QLabel, QInputDialog, QGraphicsView, QGraphicsEllipseItem
from PySide6.QtCore import QFile, Signal, Slot, QObject
from PySide6.QtGui import QStandardItemModel, QStandardItem, QPixmap, QMovie, QIcon
from PySide6.QtUiTools import QUiLoader


class HsmIde(QObject):
    signalPlantumlDone = Signal(str, str)
    appTitle = "HSM IDE"
    appDir = Path(os.path.dirname(__file__))
    configPath = appDir / "config.ini"

    def __init__(self):
        super(HsmIde, self).__init__()
        if sys.platform == "win32":
            self.lastDirectory = ""
        else:
            self.lastDirectory = "~/"
        # self.currentScxmlPath = None

        # self.load_settings()
        # (loadScxml2genStatus, errMsg) = self.loadScxml2genModule()
        # if loadScxml2genStatus is False:
        #     QMessageBox.critical(None, "Error", f"Failed to load scxml2gen.py module.\n{errMsg}",
        #                          buttons=QMessageBox.Ok)
        #     exit(1)

        self.load_ui()
        self.configure_ui()
        # self.recentFiles = RecentFilesManager(str(self.configPath),
        #                                       self.window.menuFileRecentHSM,
        #                                       self.window.menuFileRecentLogs,
        #                                       self.onActionOpenRecentHSM,
        #                                       self.onActionOpenRecentLog)
        # self.prepareFramesModel()
        # self.configureActions()
        # self.configureStatusBar()
        self.window.show()
        # if self.settings.loadLastHSM is True:
        #     if len(self.recentFiles.recentHsmFiles) > 0:
        #         self.loadScxml(self.recentFiles.recentHsmFiles[0])

    def onActionOpenHsm(self):
        (scxmlPath, lastDir) = utils.selectFile("Select HSM file", "SCXML files (*.scxml);;All files (*)", self.lastDirectory)
        if scxmlPath:
            self.lastDirectory = lastDir
            self.loadScxml(scxmlPath)

    def onActionOpenHsmLog(self):
        if self.hsm:
            (logPath, lastDir) = utils.selectFile("Select HSM log file", "HSMCPP Log Files (*.hsmlog);;All files (*)", self.lastDirectory)
            if logPath:
                self.lastDirectory = lastDir
                self.loadHsmLog(logPath)

    def onActionOpenRecentHSM(self):
        self.loadScxml(self.sender().text())

    # def onActionSearch(self):
    #     if self.window.actionSearch.isChecked():
    #         self.window.searchFilter.show()
    #         self.window.searchFilter.setFocus()
    #         self.modelFrames.enableFilter()
    #     else:
    #         self.window.searchFilter.hide()
    #         self.modelFrames.disableFilter()

    def onActionZoomIn(self):
        print("TODO")

    def onActionZoomOut(self):
        print("TODO")

    def onActionFitToView(self):
        print("TODO")

    def onActionResetZoom(self):
        print("TODO")

    def onActionSettings(self):
        if self.settings.show():
            # TODO: only needed if colors changed
            self.updateHsmFrame(self.currentFrameIndex)

    def onActionShowOnlineHelp(self):
        print("TODO")

    def onActionAbout(self):
        # print("TODO")
        print("TODO")

    def load_settings(self):
        self.settings = QSettingsDialog(str(self.configPath))

    def load_ui(self):
        loader = QUiLoader()
        # loader.registerCustomWidget(QClickableSlider)
        # loader.registerCustomWidget(QImageViewArea)
        loader.registerCustomWidget(HsmGraphicsView)
        loader.registerCustomWidget(HsmElementsList)
        path = str(self.appDir.parent / "ui" / "main.ui")
        print(path)
        ui_file = QFile(path)
        ui_file.open(QFile.ReadOnly)
        self.window = loader.load(ui_file)
        ui_file.close()

    def configure_ui(self):
        scene = QGraphicsScene()
        self.window.mainView.setScene(scene)
        self.window.mainView.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
        self.window.mainView.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
        # self.window.mainView.setAlignment(Qt.AlignTop)
        # self.window.mainView.setResizeAnchor(QGraphicsView.ViewportAnchor.NoAnchor)
        # self.mainView.setAlignment(Qt.AlignLeft | Qt.AlignTop)

        # initialize list of HSM elements
        items = HsmElementsFactory.createElementsList()
        for i in items:
            self.window.listHsmElements.addItem(i)

        # ---------------------
        # TODO: debug
        e1 = HsmStateElement()
        e1.setPos(10, 10)
        scene.addItem(e1)
        e2 = HsmStateElement()
        e2.setPos(175, 120)
        scene.addItem(e2)
        e3 = HsmStateElement()
        e3.setPos(-100, 120)
        scene.addItem(e3)

        t = HsmTransition()
        # scene.addItem(t)
        # t.connectElements(e1, e2)
        e1.addTransition(t, e2)
        # ---------------------

    def configureActions(self):
        print("TODO")
        # self.window.actionOpenHsm.triggered.connect(self.onActionOpenHsm)
        # self.window.actionOpenHsmLog.triggered.connect(self.onActionOpenHsmLog)
        # self.window.actionSearch.triggered.connect(self.onActionSearch)
        # self.window.actionShowFrames.triggered.connect(self.onActionShowFrames)
        # self.window.actionPrevFrame.triggered.connect(self.onActionPrevFrame)
        # self.window.actionNextFrame.triggered.connect(self.onActionNextFrame)
        # self.window.actionZoomIn.triggered.connect(self.onActionZoomIn)
        # self.window.actionZoomOut.triggered.connect(self.onActionZoomOut)
        # self.window.actionResetZoom.triggered.connect(self.onActionResetZoom)
        # self.window.actionFitToView.triggered.connect(self.onActionFitToView)
        # self.window.actionSettings.triggered.connect(self.onActionSettings)
        # self.window.actionGoToFrame.triggered.connect(self.onActionGoToFrame)
        # self.window.actionShowOnlineHelp.triggered.connect(self.onActionShowOnlineHelp)
        # self.window.actionAbout.triggered.connect(self.onActionAbout)

        # self.window.searchFilter.textChanged.connect(self.onSearchFilterChanged)
        # self.window.frames.selectionModel().currentRowChanged.connect(self.onFrameSelected)
        # self.window.frameSelector.valueChanged.connect(self.onFrameSelectorUpdated)
        # self.signalPlantumlDone.connect(self.plantumlGenerationDone)
        # self.enableHsmActions(False)
        # self.enableLogActions(False)

    def configureStatusBar(self):
        self.statusBarFrame = QLabel("")
        self.statusBarLog = QLabel("")
        self.window.statusbar.addPermanentWidget(self.statusBarLog, 9999)
        self.window.statusbar.addPermanentWidget(self.statusBarFrame, 0)

    # def loadScxml2genModule(self):
    #     loaded = False
    #     msg = ""
    #     if os.path.exists(self.settings.pathScxml2gen):
    #         try:
    #             spec = importlib.util.spec_from_file_location("", self.settings.pathScxml2gen)
    #             self.scxml2gen = importlib.util.module_from_spec(spec)
    #             spec.loader.exec_module(self.scxml2gen)
    #             loaded = True
    #         except:
    #             loaded = False
    #             msg = f"<{self.settings.pathScxml2gen}> contains an error or has incorrect format"
    #     else:
    #         msg = f"<{self.settings.pathScxml2gen}> wasn't found. Check that configuration file contains correct path."
    #     return (loaded, msg)

    # def loadScxml(self, path):
    #     loaded = True
    #     if path and len(path) > 0:
    #         self.unloadHsmLog()
    #         self.enableHsmActions(False)
    #         try:
    #             self.hsm = self.scxml2gen.parseScxml(path)
    #         except:
    #             self.hsm = None
    #             loaded = False
    #             QMessageBox.critical(QApplication.activeWindow(), "Error", "Failed to load HSM",
    #                                  buttons=QMessageBox.Ok)

    #         if loaded:
    #             self.hsmId = self.getDataChecksum(str(self.hsm))
    #             self.currentScxmlPath = path
    #             self.recentFiles.addPathToRecentHSMList(path)
    #             self.enableHsmActions(True)
    #             self.updateStatusBar()
    #     else:
    #         loaded = False
    #     return loaded
