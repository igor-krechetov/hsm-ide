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

from PySide6.QtWidgets import QGraphicsItem
from PySide6.QtCore import QSizeF, QRectF, QPointF, Qt, Signal
from PySide6.QtGui import QColor, QPen, QCursor

from impl.elements.private.HsmElement import HsmElement
from impl.elements.private.ElementBoundaryGripItem import ElementBoundaryGripItem

# TODO: add features
#  - mouse cursors (move, resize)
#  - context menu
#  - links
class HsmResizableElement(HsmElement):
    geometryChanged = Signal(HsmElement)

    def __init__(self):
        super().__init__()
        self.setFlag(QGraphicsItem.ItemSendsGeometryChanges, True)
        # self.setFlag(QGraphicsItem.ItemSendsScenePositionChanges, True)
        self.createBoundaryGrips()
        self.setGripVisibility(False)
        self.penSelectedBorder = QPen(QColor("lightblue"))
        self.penSelectedBorder.setStyle(Qt.DotLine)

    def createBoundaryGrips(self):
        self.grips = {}

        for direction in [ElementBoundaryGripItem.North,
                          ElementBoundaryGripItem.NorthEast,
                          ElementBoundaryGripItem.East,
                          ElementBoundaryGripItem.SouthEast,
                          ElementBoundaryGripItem.South,
                          ElementBoundaryGripItem.SouthWest,
                          ElementBoundaryGripItem.West,
                          ElementBoundaryGripItem.NorthWest]:
            newGrip = ElementBoundaryGripItem(self, direction)
            pos = self.gripPoint(direction)
            newGrip.setEnabled(False)
            newGrip.setPos(pos)
            newGrip.setEnabled(True)
            self.grips[direction] = newGrip

    def updateGripsPosition(self, directionsList):
        for direction in directionsList:
            g = self.grips[direction]
            g.setEnabled(False)
            g.setPos(self.gripPoint(direction))
            g.setEnabled(True)

    def setGripVisibility(self, visible):
        self.resizeMode = visible
        for direction in self.grips:
            self.grips[direction].setVisible(visible)
        self.update()

    def onGripLostFocus(self, grip):
        self.gripSelected = self.isSelected()
        if False == self.gripSelected:
            for direction in self.grips:
                if self.grips[direction].isUnderMouse():
                    self.gripSelected = True
                    break
        self.setGripVisibility(self.isSelected() or self.gripSelected)

    def onGripMoved(self, selectedGrip, pos):
        updateGrips = [ElementBoundaryGripItem.North,
                       ElementBoundaryGripItem.NorthEast,
                       ElementBoundaryGripItem.East,
                       ElementBoundaryGripItem.SouthEast,
                       ElementBoundaryGripItem.South,
                       ElementBoundaryGripItem.SouthWest,
                       ElementBoundaryGripItem.West,
                       ElementBoundaryGripItem.NorthWest]
        updateGrips.remove(selectedGrip.direction)
        newOuterRect = self.outerRect

        if ElementBoundaryGripItem.North == selectedGrip.direction:
            newOuterRect.setTop(pos.y())
            updateGrips.remove(ElementBoundaryGripItem.South)
            updateGrips.remove(ElementBoundaryGripItem.SouthEast)
            updateGrips.remove(ElementBoundaryGripItem.SouthWest)
        elif ElementBoundaryGripItem.NorthEast == selectedGrip.direction:
            newOuterRect.setTopRight(pos)
            updateGrips.remove(ElementBoundaryGripItem.SouthWest)
        elif ElementBoundaryGripItem.East == selectedGrip.direction:
            newOuterRect.setRight(pos.x())
            updateGrips.remove(ElementBoundaryGripItem.West)
            updateGrips.remove(ElementBoundaryGripItem.NorthWest)
            updateGrips.remove(ElementBoundaryGripItem.SouthWest)
        elif ElementBoundaryGripItem.SouthEast == selectedGrip.direction:
            newOuterRect.setBottomRight(pos)
            updateGrips.remove(ElementBoundaryGripItem.NorthWest)
        elif ElementBoundaryGripItem.South == selectedGrip.direction:
            newOuterRect.setBottom(pos.y())
            updateGrips.remove(ElementBoundaryGripItem.North)
            updateGrips.remove(ElementBoundaryGripItem.NorthWest)
            updateGrips.remove(ElementBoundaryGripItem.NorthEast)
        elif ElementBoundaryGripItem.SouthWest == selectedGrip.direction:
            newOuterRect.setBottomLeft(pos)
            updateGrips.remove(ElementBoundaryGripItem.NorthEast)
        elif ElementBoundaryGripItem.West == selectedGrip.direction:
            newOuterRect.setLeft(pos.x())
            updateGrips.remove(ElementBoundaryGripItem.East)
            updateGrips.remove(ElementBoundaryGripItem.SouthEast)
            updateGrips.remove(ElementBoundaryGripItem.NorthEast)
        elif ElementBoundaryGripItem.NorthWest == selectedGrip.direction:
            newOuterRect.setTopLeft(pos)
            updateGrips.remove(ElementBoundaryGripItem.SouthEast)

        canResize = True

        # TODO: implement min size concept
        if (newOuterRect.height() > 0) and (newOuterRect.width() > 0):
            self.updateBoundingRect(newOuterRect)
            self.updateGripsPosition(updateGrips)
            self.update()
            # self.scene().update()
            self.geometryChanged.emit(self)
        else:
            canResize = False

        return canResize

    # TODO: create grips only when selected
    def itemChange(self, change, value):
        if change == QGraphicsItem.ItemSelectedHasChanged:
            self.gripSelected = self.isSelected()
            if False == self.gripSelected:
                for direction in self.grips:
                    if self.grips[direction].isUnderMouse():
                        self.gripSelected = True
                        break
            self.setGripVisibility(self.isSelected() or self.gripSelected)
        elif change == QGraphicsItem.ItemPositionHasChanged:
            self.geometryChanged.emit(self)

        return super(HsmResizableElement, self).itemChange(change, value)

    def indexOf(self, p):
        for i in range(4):
            if p == self.point(i):
                return i

    def gripPoint(self, gripDirection):
        xCenter = self.outerRect.center().x()
        yCenter = self.outerRect.center().y()
        positions = {ElementBoundaryGripItem.North: QPointF(xCenter, self.outerRect.top()),
                     ElementBoundaryGripItem.NorthEast: self.outerRect.topRight(),
                     ElementBoundaryGripItem.East: QPointF(self.outerRect.right(), yCenter),
                     ElementBoundaryGripItem.SouthEast: self.outerRect.bottomRight(),
                     ElementBoundaryGripItem.South: QPointF(xCenter, self.outerRect.bottom()),
                     ElementBoundaryGripItem.SouthWest: self.outerRect.bottomLeft(),
                     ElementBoundaryGripItem.West: QPointF(self.outerRect.left(), yCenter),
                     ElementBoundaryGripItem.NorthWest: self.outerRect.topLeft()}
        return positions[gripDirection]

    def paint(self, painter, option, widget):
        # super(HsmResizableElement, self).paint(painter, option, widget)
        if True == self.resizeMode:
            painter.setPen(self.penSelectedBorder)
            painter.drawRoundedRect(self.outerRect, 5, 5)

            painter.drawEllipse(QPointF(0, 0), 5, 5)
            painter.drawPoint(0, 0)