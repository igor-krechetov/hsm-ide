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

from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import QGraphicsView
from PySide6.QtGui import QDragEnterEvent, QDragMoveEvent
from impl.elements import HsmElementsFactory

class HsmGraphicsView(QGraphicsView):
    def __init__(self, parent):
        super().__init__(parent)
        self.setAcceptDrops(True)

    def dragEnterEvent(self, event: QDragEnterEvent) -> None:
        if event.mimeData().hasFormat("hsm/element"):
            print(f"dragEnterEvent: {event.source()}, <{event.mimeData().formats()}>")
            print(f"{event.mimeData().data('hsm/element')}")
            event.setDropAction(Qt.CopyAction)
            event.accept()

    def dragMoveEvent(self, event: QDragMoveEvent) -> None:
        event.setDropAction(Qt.CopyAction)
        event.accept()

    def dropEvent(self, event: QDragEnterEvent) -> None:
        print(f"dropEvent: {event.position()}")
        event.setDropAction(Qt.CopyAction)
        event.accept()
        e1 = HsmElementsFactory.createElement(event.mimeData().data('hsm/element').data().decode())
        e1.setPos(self.mapToScene(event.position().toPoint()))
        self.scene().addItem(e1)
