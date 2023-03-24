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

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QScrollArea


class QImageViewArea(QScrollArea):
    isDragging = False

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.setCursor(Qt.CursorShape.OpenHandCursor)
            self.prevMousePosition = event.pos()
            self.isDragging = True
        else:
            super(QImageViewArea, self).mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.setCursor(Qt.CursorShape.ArrowCursor)
            self.isDragging = False
        else:
            super(QImageViewArea, self).mouseReleaseEvent(event)

    def mouseMoveEvent(self, event):
        super(QImageViewArea, self).mouseMoveEvent(event)
        if self.isDragging:
            delta = event.pos() - self.prevMousePosition
            self.prevMousePosition = event.pos()
            self.horizontalScrollBar().setValue(self.horizontalScrollBar().value() - delta.x())
            self.verticalScrollBar().setValue(self.verticalScrollBar().value() - delta.y())
