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

from impl.elements import HsmTransition
from impl.elements.private.HsmResizableElement import HsmResizableElement
from PySide6.QtWidgets import QGraphicsItem, QGraphicsObject
from PySide6.QtCore import QSizeF, QRectF, QPointF, Qt, Signal
from PySide6.QtGui import QPainterPath, QColor, QPen, QCursor, QTransform

class ElementConnectionArror(QGraphicsObject):
    # scene pos
    connectionPositionChanged = Signal(QGraphicsObject, QPointF)
    connectionStarted = Signal(QGraphicsObject, QPointF)
    connectionFinished = Signal(QGraphicsObject, QPointF)

    # arrow directions
    North = 1
    East = 3
    South = 5
    West = 7

    a = 10.0
    b = 16.0
    w = 3 * a

    def __init__(self, annotation_element, direction):
        super().__init__(annotation_element)
        # self.annotation_element = annotation_element
        # self.gripDirection = direction
        self.outerRect = QRectF(-self.w/2, -self.w/2, self.w, self.w)
        self.direction = direction
        self.shapeArrow = self.initShape(direction)
        self.arrowColor = QColor("green")
        self.setAcceptHoverEvents(True)
        self.setZValue(11)
        self.setCursor(QCursor(Qt.PointingHandCursor))

    def setPos(self, pos: QPointF):
        offset = 5
        if (self.North == self.direction):
            pos.setY(pos.y() - self.w/2 - offset)
        elif (self.East == self.direction):
            pos.setX(pos.x() + self.w/2 + offset)
        elif (self.South == self.direction):
            pos.setY(pos.y() + self.w/2 + offset)
        elif (self.West == self.direction):
            pos.setX(pos.x() - self.w/2 - offset)

        super(ElementConnectionArror, self).setPos(pos)

    def boundingRect(self):
        return self.outerRect

    def paint(self, painter, option, widget):
        painter.setBrush(self.arrowColor)
        painter.setPen(Qt.NoPen)
        painter.drawPath(self.shapeArrow)

    def initShape(self, direction):
        shape = QPainterPath()
        base = 0.55*self.b
        offset = (self.w - self.b) / 2
        shape.addRect(-base/2, -2.0*self.a, base, 2.0*self.a)
        shape.moveTo(-self.w/2 + offset, -2.0*self.a)
        shape.lineTo(self.w/2 - offset, -2.0*self.a)
        shape.lineTo(0, -self.w)
        shape.lineTo(-self.w/2 + offset, -2.0*self.a)

        t = QTransform()

        if (self.North == direction):
            t.translate(0, self.w/2)
        elif (self.East == direction):
            t.translate(-self.w/2, 0)
            t.rotate(90.0)
        elif (self.South == direction):
            t.translate(0, -self.w/2)
            t.rotate(180.0)
        elif (self.West == direction):
            t.translate(self.w/2, 0)
            t.rotate(-90.0)

        return t.map(shape)

    def hoverEnterEvent(self, event):
        self.arrowColor = QColor("red")
        super(ElementConnectionArror, self).hoverEnterEvent(event)

    def hoverLeaveEvent(self, event):
        self.arrowColor = QColor("green")
        super(ElementConnectionArror, self).hoverLeaveEvent(event)

    def mousePressEvent(self, event):
        print("PRESS - arrow")
        self.connectionStarted.emit(self, self.mapToScene(event.pos()))
        event.accept()
        # return super().mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        print("RELEASE - arrow")
        self.connectionFinished.emit(self, self.mapToScene(event.pos()))
        return super().mouseReleaseEvent(event)

    def mouseMoveEvent(self, event):
        self.connectionPositionChanged.emit(self, self.mapToScene(event.pos()))
        return super().mouseMoveEvent(event)


class HsmConnectableElement(HsmResizableElement):
    def __init__(self):
        super(HsmConnectableElement, self).__init__()
        self.updateHoverRect(False)
        self.setAcceptHoverEvents(True)
        self.drawConnectionLine = False
        self.connection = None
        self.arrows = {}
        self.transitions = []

    def addTransition(self, transition: HsmTransition, target: 'HsmConnectableElement'):
        if transition.parentItem() is None:
            self.scene().addItem(transition)
        transition.connectElements(self, target)
        self.transitions.append(transition)

    # -----------------------------------
    # Geometry
    def boundingRect(self):
        return self.hoverRect

    def updateHoverRect(self, expendArea):
        self.update()
        self.prepareGeometryChange()
        arrowOffset = 0
        if True == expendArea:
            arrowOffset = ElementConnectionArror.w

        self.hoverRect = QRectF(self.outerRect.left() - arrowOffset,
                                self.outerRect.top() - arrowOffset,
                                self.outerRect.width() + 2*arrowOffset,
                                self.outerRect.height() + 2*arrowOffset)

    # -----------------------------------
    # Connection arrows
    def createConnectionArrows(self):
        if len(self.arrows) == 0:
            self.arrows = {}
            self.updateHoverRect(True)
            for direction in [ElementConnectionArror.North,
                            ElementConnectionArror.East,
                            ElementConnectionArror.South,
                            ElementConnectionArror.West]:
                newArrow = ElementConnectionArror(self, direction)
                newArrow.setPos(self.getArrowPos(direction))
                newArrow.connectionPositionChanged.connect(self.updateConnectionLine)
                newArrow.connectionStarted.connect(self.beginConnection)
                newArrow.connectionFinished.connect(self.finishConnectionLine)
                self.arrows[direction] = newArrow

    def removeConnectionArrows(self):
        if len(self.arrows) > 0:
            for direction in self.arrows:
                self.arrows[direction].setParentItem(None)
            self.arrows = {}
            self.updateHoverRect(False)

    def updateConnectionArrowsPos(self):
        for direction in self.arrows:
            self.arrows[direction].setPos(self.getArrowPos(direction))

    def onGripMoved(self, selectedGrip, pos):
        super(HsmConnectableElement, self).onGripMoved(selectedGrip, pos)
        self.updateHoverRect(True)
        self.updateConnectionArrowsPos()

    def getArrowPos(self, arrowDirection):
        xCenter = self.outerRect.center().x()
        yCenter = self.outerRect.center().y()
        positions = {ElementConnectionArror.North: QPointF(xCenter, self.outerRect.top()),
                     ElementConnectionArror.East: QPointF(self.outerRect.right(), yCenter),
                     ElementConnectionArror.South: QPointF(xCenter, self.outerRect.bottom()),
                     ElementConnectionArror.West: QPointF(self.outerRect.left(), yCenter)}
        return positions[arrowDirection]

    def hoverMoveEvent(self, event):
        if len(self.arrows) == 0:
            sceneRect = self.mapRectToScene(self.outerRect)
            sceneMousePos = self.mapToScene(event.pos())
            if True == sceneRect.contains(sceneMousePos):
                self.createConnectionArrows()
        return super().hoverMoveEvent(event)

    def hoverLeaveEvent(self, event):
        self.removeConnectionArrows()
        super().hoverLeaveEvent(event)

    def itemChange(self, change, value):
        if change == QGraphicsItem.ItemPositionHasChanged:
            self.removeConnectionArrows()
            #TODO: hide grips
        return super().itemChange(change, value)

    # -----------------------------------
    # New transition logic
    def beginConnection(self, arrow, pos):
        self.drawConnectionLine = False
        self.connection = HsmTransition.HsmTransition()
        self.connection.beginConnection(self, pos)
        self.scene().addItem(self.connection)

    def finishConnectionLine(self, arrow, pos):
        targetItem = self.scene().itemAt(pos, QTransform())
        if targetItem == arrow:
            targetItem = None
        self.drawConnectionLine = False
        print(f"FINISH - arrow. Target {targetItem},  {pos}")

        if targetItem == self:
            print("TODO: target-self")
            self.connection.setParentItem(None)
        elif isinstance(targetItem, HsmConnectableElement):
            print("TODO: target-state")
            self.addTransition(self.connection, targetItem)
        else:
            print("TODO: cancel connection")
            self.connection.setParentItem(None)
        # TODO: target - child object
        self.connection = None

    def updateConnectionLine(self, arrow, pos):
        if self.connection is not None:
            self.connection.moveConnectionTo(pos)

