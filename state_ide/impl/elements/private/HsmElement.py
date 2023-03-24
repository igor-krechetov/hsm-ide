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

from PySide6.QtWidgets import QGraphicsObject, QGraphicsItem, QGraphicsSceneDragDropEvent
from PySide6.QtCore import QSizeF, QRectF

# TODO: add features
#  - mouse cursors (move, resize)
#  - context menu
#  - links
class HsmElement(QGraphicsObject):
    def __init__(self):
        super().__init__()
        self.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable)
        self.size = QSizeF(200.0, 40.0)
        self.setZValue(3)
        self.updateBoundingRect()

    def updateBoundingRect(self, newRect=None):
        self.prepareGeometryChange()
        if None == newRect:
            penWidth = 1.0
            self.outerRect = QRectF((-1) * self.size.width() / 2 - penWidth / 2, (-1) * self.size.height() / 2 - penWidth / 2,
                                    self.size.width() + penWidth, self.size.height() + penWidth)
        else:
            self.outerRect = newRect
            self.size = self.outerRect.size()

    def elementRect(self):
        return self.outerRect

    def boundingRect(self):
        return self.outerRect

    def dragEnterEvent(self, event: QGraphicsSceneDragDropEvent) -> None:
        print(f"ITEM: dragEnterEvent: {event.source()}, <{event.mimeData().formats()}>")
        return super().dragEnterEvent(event)

    def dropEvent(self, event: QGraphicsSceneDragDropEvent) -> None:
        print(f"ITEM: dragEnterEvent: {event.source()}, <{event.mimeData().formats()}>")
        return super().dropEvent(event)

    # def itemChange(self, change, value):
    #     if change == QGraphicsItem.ItemSelectedHasChanged:
    #         print(f"ROOT SELECTION CHANGED: {self.isSelected()}")
    #         self.gripSelected = self.isSelected()
    #         if False == self.gripSelected:
    #             for direction in self.grips:
    #                 if self.grips[direction].isUnderMouse():
    #                     self.gripSelected = True
    #                     break

    #         self.setGripVisibility(self.isSelected() or self.gripSelected)
    #     return super(HsmElement, self).itemChange(change, value)

    # def paint(self, painter, option, widget):
    #     painter.setPen(Qt.SolidLine)
    #     itemRect = QRectF(self.outerRect)
    #     # diff = 10
    #     # itemRect.adjust(diff, diff, -diff, -diff)
    #     painter.drawRoundedRect(itemRect, 5, 5)

