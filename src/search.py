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

import re
from PySide6.QtCore import QSortFilterProxyModel


class QFramesSearchModel(QSortFilterProxyModel):
    filterAction = ""
    filterArgs = ""
    regexAction = None
    regexArgs = None
    disableFiltering = True

    def disableFilter(self):
        self.disableFiltering = True
        self.invalidateFilter()

    def enableFilter(self):
        self.disableFiltering = False
        self.invalidateFilter()

    def setFilter(self, newFilter):
        if len(newFilter) > 0:
            posSeparator = newFilter.find(":")
            if posSeparator >= 0:
                self.filterAction = newFilter[:posSeparator]
                self.filterArgs = newFilter[posSeparator + 1:]
                self.regexArgs = re.compile(f".*{self.filterArgs}.*")
            else:
                self.filterAction = newFilter
                self.regexArgs = None

            self.regexAction = re.compile(f".*{self.filterAction}.*")
        else:
            self.filterAction = ""
            self.filterArgs = ""
            self.regexAction = None
            self.regexArgs = None
        self.invalidateFilter()

    def filterAcceptsRow(self, source_row, source_parent):
        accept = True
        if (self.disableFiltering is False) and (self.regexAction is not None):
            operationIndex = self.sourceModel().index(source_row, 2, source_parent)
            isCorrectAction = self.regexAction.match(self.sourceModel().data(operationIndex))
            isCorrectArgs = True

            if isCorrectAction and (self.regexArgs is not None):
                argsIndex = self.sourceModel().index(source_row, 3, source_parent)
                isCorrectArgs = self.regexArgs.match(self.sourceModel().data(argsIndex))
            accept = (isCorrectAction is not None) and (isCorrectArgs is not None)

        return accept

