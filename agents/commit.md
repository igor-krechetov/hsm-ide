Version Rules
- Format X.Y.Z
- X - major. never change it
- Y - minor. increment when introducing new features
- Z - patch. increment it for all other changes

If incrementing version
- Update X.Y.Z in README.md:
[![Changelog](https://img.shields.io/badge/changelog-vX.Y.Z-green.svg)](https://github.com/igor-krechetov/hsm-ide/blob/main/CHANGELOG.md)
- Update PROJECT_VERSION in root CMakeLists.txt
- Update changes in CHANGELOG.md

Commit description
- Prepare description according to template below. Skip sections if empty or not applicable.
- Title format: Either [<version>] <description> or [test] <description>, [build] <description>, [refactoring] <description>. Example: [0.3.1] Recent files and workspaces features
- Requirements for commit description can be found in 'req/requirements.pu'

===== BEGIN EXAMPLE =====
Title: [x.y.z | test | build | refactoring] <title>

New
- [<requirement ID>] <description>
...

Fixed
- <change>
- <change>

<other changes>
===== END EXAMPLE =====