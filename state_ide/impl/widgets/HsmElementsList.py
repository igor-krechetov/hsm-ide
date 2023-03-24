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

from PySide6.QtCore import Qt, QMimeData, QByteArray
from PySide6.QtWidgets import QListWidget

class HsmElementsList(QListWidget):
    def __init__(self, parent):
        super().__init__(parent)

    def mimeTypes(self):
        # types = ["hsm/element", "hsm/multiple"]
        types = ["hsm/element"]
        return types

    def mimeData(self, items) -> QMimeData:
        print(f"mimeData: {len(items)}")
        data = None

        if len(items) > 0:
            data = QMimeData()
            if len(items) == 1:
                print(items[0].data(Qt.UserRole))
                data.setData("hsm/element", QByteArray(items[0].data(Qt.UserRole)))
            else:
                data.setData("hsm/multiple", QByteArray())
        return data
