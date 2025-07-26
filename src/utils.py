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
import os
import sys
from PySide6.QtWidgets import QApplication, QFileDialog

class utils(object):
    @staticmethod
    def selectFile(title, filter, lastDirectory):
        selectedPath = None
        dlgFileSelector = QFileDialog(QApplication.activeWindow(), title)
        # NOTE: native file dialog on Ubuntu returns fake file path which are impossible to use in "recent files"
        if sys.platform != "win32":
            dlgFileSelector.setOption(QFileDialog.DontUseNativeDialog)
        dlgFileSelector.setDirectory(lastDirectory)
        dlgFileSelector.setNameFilter(filter)
        if dlgFileSelector.exec_():
            selectedPath = dlgFileSelector.selectedFiles()[0]
            lastDirectory = os.path.dirname(selectedPath)
        return (selectedPath, lastDirectory)