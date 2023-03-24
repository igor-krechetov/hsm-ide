#!/bin/sh

pyi-makespec --onefile --hidden-import PySide6.QtXml --add-data 'ui/main.ui':'ui' --add-data 'ui/settings.ui':'ui' ./hsm_ide.py
pyinstaller ./hsm_ide.spec
