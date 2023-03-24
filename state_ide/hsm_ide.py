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

# This Python file uses the following encoding: utf-8
import sys
import os
import yaml
import importlib.util
import subprocess
import threading
import hashlib
from pathlib import Path

from impl.hsm_ide_app import HsmIde

from PySide6.QtCore import Qt
from PySide6.QtWidgets import QApplication
from PySide6.QtGui import QIcon

if __name__ == "__main__":
    if sys.platform == 'win32':
        # NOTE: This is needed to display the app icon on the taskbar on Windows 7
        import ctypes
        myappid = 'igorkrechetov.hsmide'
        ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(myappid)

    app = QApplication(sys.argv)
    main = HsmIde()
    app.setWindowIcon(QIcon(str(Path("./res/hsm_ide.ico"))))
    sys.exit(app.exec_())
