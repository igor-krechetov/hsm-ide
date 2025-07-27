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
from impl.elements import HsmStateElement, HsmEntryPointElement, HsmExitPointElement, HsmFinalElement, HsmHistoryElement, HsmStartElement
from PySide6.QtCore import Qt
from PySide6.QtGui import QIcon
from PySide6.QtWidgets import QListWidgetItem


class HsmElementsFactory:
    # def __init__(self):
        # print("--- CREATED")
    itemsCatalog = {"start": ["Start", "/../../res/element_start.png", "_createElementStart"],
                    "final": ["Final", "/../../res/element_final.png", "_createElementFinal"],
                    "state": ["State", "/../../res/element_state.png", "_createElementState"],
                    "entrypoint": ["Entry Point", "/../../res/element_entrypoint.png", "_createElementEntryPoint"],
                    "exitpoint": ["Exit Point", "/../../res/element_exitpoint.png", "_createElementExitPoint"],
                    "history": ["History", "/../../res/element_history.png", "_createElementHistory"]}

    @staticmethod
    def createElementsList():
        elements = []

        for id in HsmElementsFactory.itemsCatalog:
            itemInfo = HsmElementsFactory.itemsCatalog[id]
            curScriptDir = os.path.dirname(__file__)
            newItem = QListWidgetItem(QIcon(curScriptDir + itemInfo[1]), itemInfo[0])
            newItem.setData(Qt.UserRole, id)
            elements.append(newItem)

        return elements

    @staticmethod
    def createElement(id: str):
        element = None

        if id in HsmElementsFactory.itemsCatalog:
            callback = getattr(HsmElementsFactory, HsmElementsFactory.itemsCatalog[id][2])
            element = callback()
        return element

    # TODO: implement all elements

    @staticmethod
    def _createElementState():
        return HsmStateElement.HsmStateElement()

    @staticmethod
    def _createElementStart():
        return HsmStartElement.HsmStartElement()

    @staticmethod
    def _createElementFinal():
        return HsmFinalElement.HsmFinalElement()

    @staticmethod
    def _createElementEntryPoint():
        return HsmEntryPointElement.HsmEntryPointElement()

    @staticmethod
    def _createElementExitPoint():
        return HsmExitPointElement.HsmExitPointElement()

    @staticmethod
    def _createElementHistory():
        return HsmHistoryElement.HsmHistoryElement()
