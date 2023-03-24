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

from PySide6.QtWidgets import QGraphicsItem, QGraphicsObject
from PySide6.QtCore import QRectF, QPointF, Qt, Signal
from PySide6.QtGui import QColor, QCursor

class ElementGripItem(QGraphicsObject):
    onGripDoubleClick = Signal(QGraphicsObject)
    # onGripMoved = Signal(QGraphicsObject, QPointF)
    onGripLostFocus = Signal(QGraphicsObject)

    def __init__(self, annotation_element):
        super().__init__(annotation_element)

        self.annotation_element = annotation_element
        self.gripRect = QRectF(-4, -4, 8, 8)
        # setFlag(ItemIgnoresTransformations, true);

        self.tryConnectSignal(self.onGripDoubleClick, annotation_element, "onGripDoubleClick")
        # self.tryConnectSignal(self.onGripMoved, annotation_element, "onGripMoved")
        self.tryConnectSignal(self.onGripLostFocus, annotation_element, "onGripLostFocus")

        self.gripColor = QColor("green")
        self.setFlag(QGraphicsItem.ItemIsSelectable, True)
        self.setFlag(QGraphicsItem.ItemIsMovable, True)
        self.setFlag(QGraphicsItem.ItemSendsGeometryChanges, True)
        self.setAcceptHoverEvents(True)
        self.setZValue(11)
        self.setCursor(QCursor(Qt.PointingHandCursor))

    def tryConnectSignal(self, sig, object, functionName):
        functionObject = getattr(object, functionName, None)
        if callable(functionObject):
            sig.connect(functionObject)

    def boundingRect(self):
        return self.gripRect

    def paint(self, painter, option, widget):
        painter.setBrush(self.gripColor)
        painter.setPen(Qt.NoPen)
        painter.drawRect(self.gripRect)

    def hoverEnterEvent(self, event):
        self.gripColor = QColor("red")
        super().hoverEnterEvent(event)

    def hoverLeaveEvent(self, event):
        self.gripColor = QColor("green")
        super().hoverLeaveEvent(event)

    def mouseDoubleClickEvent(self, event):
        self.onGripDoubleClick.emit(self)
        return super().mouseDoubleClickEvent(event)

    def itemChange(self, change, value):
        p = None
        if change == QGraphicsItem.ItemPositionChange and self.isEnabled():
            p = QPointF(value)
            if False == self.annotation_element.onGripMoved(self, p):
                p = self.pos()
        else:
            p = super().itemChange(change, value)

        if change == QGraphicsItem.ItemSelectedHasChanged:
            if False == self.isSelected():
                self.onGripLostFocus.emit(self)

        return p


class ElementBoundaryGripItem(ElementGripItem):
    # grip directions
    North = 1
    NorthEast = 2
    East = 3
    SouthEast = 4
    South = 5
    SouthWest = 6
    West = 7
    NorthWest = 8
    DirectionPos = 9

    Horizontal = 1
    Vertical = 2
    Diagonal = 3

    def __init__(self, annotation_element, direction):
        super().__init__(annotation_element)
        self.gripDirection = direction

        if self.gripDirection in [self.North, self.South]:
            self.gripDirectionType = self.Vertical
        elif self.gripDirection in [self.West, self.East]:
            self.gripDirectionType = self.Horizontal
        elif self.gripDirection in [self.NorthEast, self.NorthWest, self.SouthWest, self.SouthEast]:
            self.gripDirectionType = self.Diagonal

    @property
    def direction(self):
        return self.gripDirection

    @property
    def directionType(self):
        return self.gripDirectionType

    def itemChange(self, change, value):
        p = None
        if change == QGraphicsItem.ItemPositionChange and self.isEnabled():
            p = QPointF(self.pos())
            if (self.gripDirectionType == self.Horizontal):
                p.setX(value.x())
            elif (self.gripDirectionType == self.Vertical):
                p.setY(value.y())
            else:
                p.setX(value.x())
                p.setY(value.y())

            if False == self.annotation_element.onGripMoved(self, p):
                p = self.pos()
        else:
            p = super().itemChange(change, value)

        if change == QGraphicsItem.ItemSelectedHasChanged:
            if False == self.isSelected():
                self.annotation_element.onGripLostFocus(self)

        return p

