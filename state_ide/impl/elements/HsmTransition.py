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

import math
from PySide6.QtWidgets import QGraphicsItem
from PySide6.QtCore import QPointF, QLineF, Qt, QRectF
from PySide6.QtGui import QPen, QPolygonF, QPainterPath

from impl.elements.private.HsmElement import HsmElement
from impl.elements.private.ElementBoundaryGripItem import ElementGripItem

# TODO: recalculate start and end positions when related items were moved
class HsmTransition(HsmElement):
    # Consts
    SELECTION_DISTANCE = 2

    def __init__(self):
        self.fromElement = None
        self.toElement = None
        self.selectionPath = QPainterPath()
        self.linePath = QPolygonF()
        # NOTE: there are no grips at the beginning and and of the poly-line
        self.lineGrips = [None, None]
        self.debugShowSelectionPolygon = False

        super().__init__()
        self.setFlag(QGraphicsItem.ItemIsMovable, False)
        self.setFlag(QGraphicsItem.ItemIsSelectable, True)
        self.setZValue(0)
        self.updateBoundingRect()

    # -----------------------------------
    # bounding rect
    def updateBoundingRect(self):
        self.prepareGeometryChange()
        self.updateSelectionPolygon()
        self.outerRect = self.linePath.boundingRect().normalized().adjusted(-self.SELECTION_DISTANCE,
                                                                            -self.SELECTION_DISTANCE,
                                                                            self.SELECTION_DISTANCE,
                                                                            self.SELECTION_DISTANCE)

    def calculateLinePolygon(self, selectionOffset, line: QLineF):
        nPolygon = QPolygonF()
        radAngle = line.angle() * math.pi / 180.0
        dx = selectionOffset * math.sin(radAngle)
        dy = selectionOffset * math.cos(radAngle)
        offset1 = QPointF(dx, dy)
        offset2 = QPointF(-dx, -dy)
        nPolygon.append(line.p1() + offset1)
        nPolygon.append(line.p1() + offset2)
        nPolygon.append(line.p2() + offset2)
        nPolygon.append(line.p2() + offset1)
        nPolygon.append(line.p1() + offset1)
        return nPolygon

    def updateSelectionPolygon(self):
        self.selectionPath.clear()
        offset = 5

        for i in range(len(self.linePath) - 1):
            p1 = self.linePath[i]
            p2 = self.linePath[i+1]
            self.selectionPath.addPolygon(self.calculateLinePolygon(offset, QLineF(p1, p2)))

    def shape(self):
        return self.selectionPath

    # -----------------------------------
    # Visualization
    def paint(self, painter, option, widget):
        if False == self.isSelected():
            painter.setPen(Qt.SolidLine)
        else:
            painter.setPen(Qt.DashLine)

        for i in range(len(self.linePath) - 1):
            painter.drawLine(self.linePath[i], self.linePath[i+1])

        # NOTE: Debug code
        if self.debugShowSelectionPolygon:
            painter.setPen(QPen("red"))
            painter.drawPath(self.selectionPath)

    # -----------------------------------
    # Connected elements
    def beginConnection(self, fromElement, pos):
        print(f"{fromElement}, {pos}")
        self.removeConnection()
        self.fromElement = fromElement
        self.currentMovePos = pos
        self.recalculateLine()

    def isConnecting(self):
        return (self.fromElement is not None) and (self.toElement is None)

    def moveConnectionTo(self, pos):
        if self.isConnecting():
            self.currentMovePos = pos
            self.recalculateLine()
        else:
            print("WARNING: unexpected moveConnectionTo call")

    def connectElements(self, fromElement, toElement):
        self.removeConnection()
        self.fromElement = fromElement
        self.toElement = toElement
        self.recalculateLine()
        self.fromElement.geometryChanged.connect(self.recalculateLine)
        self.toElement.geometryChanged.connect(self.recalculateLine)

    def removeConnection(self):
        if self.fromElement is not None:
            try:
                self.fromElement.geometryChanged.disconnect(self.recalculateLine)
            except:
                print("from not connected")
        if self.toElement is not None:
            try:
                self.toElement.geometryChanged.disconnect(self.recalculateLine)
            except:
                print("to not connected")
        self.fromElement = None
        self.toElement = None
        # self.recalculateLine()

    def findStartingPointFromPoint(self, rectFrom, pointTo):
        pointFrom = QPointF()

        if (rectFrom.bottom() <= pointTo.y()):
            if rectFrom.right() <= pointTo.x():
                # print("1")
                pointFrom = QPointF(rectFrom.right(), rectFrom.center().y())
            elif rectFrom.left() >= pointTo.x():
                # print("3")
                pointFrom = QPointF(rectFrom.left(), rectFrom.center().y())
            else:
                # print("2, 12, 23")
                pointFrom = QPointF(rectFrom.center().x(), rectFrom.bottom())
        elif (rectFrom.top() >= pointTo.y()):
            if rectFrom.right() <= pointTo.x():
                # print("6")
                pointFrom = QPointF(rectFrom.right(), rectFrom.center().y())
            elif rectFrom.left() >= pointTo.x():
                # print("8")
                pointFrom = QPointF(rectFrom.left(), rectFrom.center().y())
            else:
                # print("7, 67, 78")
                pointFrom = QPointF(rectFrom.center().x(), rectFrom.top())
        elif (rectFrom.right() <= pointTo.x()):
            # print("4, 14, 46")
            pointFrom = QPointF(rectFrom.right(), rectFrom.center().y())
        elif (rectFrom.left() >= pointTo.x()):
            # print("5, 35, 58")
            pointFrom = QPointF(rectFrom.left(), rectFrom.center().y())
        else:
            # print("overlapping")
            pointFrom = rectFrom.center()

        return pointFrom

    def findStartingPointFromRect(self, rectFrom, rectTo):
        pointFrom = None
        if (rectFrom.bottom() <= rectTo.top()):
            if rectFrom.right() <= rectTo.left():
                # print("1")
                pointFrom = QPointF(rectFrom.right(), rectFrom.center().y())
            elif rectFrom.left() >= rectTo.right():
                # print("3")
                pointFrom = QPointF(rectFrom.left(), rectFrom.center().y())
            else:
                # print("2, 12, 23")
                pointFrom = QPointF(rectFrom.center().x(), rectFrom.bottom())
        elif (rectFrom.top() >= rectTo.bottom()):
            if rectFrom.right() <= rectTo.left():
                # print("6")
                pointFrom = QPointF(rectFrom.right(), rectFrom.center().y())
            elif rectFrom.left() >= rectTo.right():
                # print("8")
                pointFrom = QPointF(rectFrom.left(), rectFrom.center().y())
            else:
                # print("7, 67, 78")
                pointFrom = QPointF(rectFrom.center().x(), rectFrom.top())
        elif (rectFrom.right() <= rectTo.left()):
            # print("4, 14, 46")
            pointFrom = QPointF(rectFrom.right(), rectFrom.center().y())
        elif (rectFrom.left() >= rectTo.right()):
            # print("5, 35, 58")
            pointFrom = QPointF(rectFrom.left(), rectFrom.center().y())
            pointTo = QPointF(rectTo.right(), rectTo.center().y())
        else:
            # print("overlapping")
            pointFrom = rectFrom.center()
        return pointFrom

    def recalculateLine(self, element=None):
        # TODO: decide how to snap transition to items
        if (self.fromElement is not None) and (self.toElement is not None):
            # posFrom = self.fromElement.mapToScene(self.fromElement.pos())
            # posTo = self.fromElement.mapToScene(self.toElement.pos())
            rectFrom = self.mapRectFromItem(self.fromElement, self.fromElement.elementRect())
            rectTo = self.mapRectFromItem(self.toElement, self.toElement.elementRect())

            # determine relative position of elements agains eachother
            # if can be one of these 8 positions or a combination of them
            #     1 2 3
            #     4 @ 5
            #     6 7 8
            #
            #    12 23
            #  14 @@@ 35
            #     @@@
            #  46 @@@ 58
            #    67 78
            #
            # 1, 3, 6, 8: angle line
            # others: Z-line
            # rectFrom = QRectF()
            # rectTo = QRectF()

            # print(f"from: {rectFrom}, to: {rectTo}")
            # print(f"from.b={rectFrom.bottom()}, to.t={rectTo.top()}")

            if (self.linePath.size() > 0):
                self.linePath.removeFirst()
                self.linePath.removeLast()

            pointFrom = None
            pointTo = None

            if (len(self.linePath) > 0):
                pointFrom = self.findStartingPointFromPoint(rectFrom, self.linePath.first())
                pointTo = self.findStartingPointFromPoint(rectTo, self.linePath.last())
            else:
                pointFrom = self.findStartingPointFromRect(rectFrom, rectTo)
                pointTo = self.findStartingPointFromRect(rectTo, rectFrom)

            self.linePath.insert(0, pointFrom)
            self.linePath.append(pointTo)
        elif (self.fromElement is not None):
            self.linePath.clear()
            rectFrom = self.mapRectFromItem(self.fromElement, self.fromElement.elementRect())
            pointFrom = self.findStartingPointFromPoint(rectFrom, self.currentMovePos)
            self.linePath.append(pointFrom)
            self.linePath.append(self.currentMovePos)
        else:
            self.linePath.clear()
        self.updateBoundingRect()

    # -----------------------------------
    # Grip notifications
    def onGripMoved(self, grip, pos):
        gripIndex = self.findGripIndex(grip)
        if None != gripIndex:
            self.linePath[gripIndex] = pos
            self.recalculateLine()
            self.updateBoundingRect()
        return True

    def onGripDoubleClick(self, grip):
        gripIndex = self.findGripIndex(grip)
        if None != gripIndex:
            grip.setParentItem(None)
            self.linePath.removeAt(gripIndex)
            del self.lineGrips[gripIndex]
            self.updateBoundingRect()

    # def onGripLostFocus(self, grip):
    #     # TODO
    #     return True

    def findGripIndex(self, grip):
        gripIndex = None
        for i in range(len(self.lineGrips)):
            if self.lineGrips[i] == grip:
                gripIndex = i
                break
        return gripIndex

    def mouseDoubleClickEvent(self, event):
        (sel, p, index) = self.isPointOnTheLine(event.pos())

        if None != index:
            e = ElementGripItem(self)
            e.setPos(p)
            self.linePath.insert(index + 1, p)
            self.lineGrips.insert(index + 1, e)

        return super().mouseDoubleClickEvent(event)

    # -----------------------------------
    # Utils
    def isPointOnTheLine(self, pos):
        intersects = False
        linePoint = QPointF()
        itPrevPoint = None

        cornersCount = len(self.linePath)
        for i in range(cornersCount):
            if (QLineF(self.linePath[i], pos).length() <= self.SELECTION_DISTANCE):
                # Is pressed point close enough of the grip-point
                intersects = True
                break
            else:
                if (i < self.linePath.count() - 1):
                    line = QLineF(self.linePath[i], self.linePath[i + 1])
                    # Is pressed point close enough of the line
                    line2 = QLineF(pos, pos + QPointF(self.SELECTION_DISTANCE, self.SELECTION_DISTANCE))
                    line2.setAngle(line.angle() + 90)
                    (intersection, linePoint) = line.intersects(line2)

                    if QLineF.BoundedIntersection == intersection:
                        intersects = True
                    else:
                        line2.setAngle(line.angle() - 90)
                        (intersection, linePoint) = line.intersects(line2)
                        intersects = (QLineF.BoundedIntersection == intersection)

                    if True == intersects:
                        itPrevPoint = i
                        break

        return (intersects, linePoint, itPrevPoint)