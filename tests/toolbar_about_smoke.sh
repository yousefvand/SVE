#!/usr/bin/env bash
# Check that the two actual toolbar buttons sit right next to Open project
# and route to the existing, visible About dialogs.
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
python3 - "$root/src/mainwindow.cpp" "$root/CMakeLists.txt" <<'PY'
from pathlib import Path
import sys
source=Path(sys.argv[1]).read_text()
cmake=Path(sys.argv[2]).read_text()
assert 'project(SVE VERSION 0.1.0' in cmake
assert 'auto *aboutSveAction = helpMenu->addAction("About SVE"' in source
assert 'auto *aboutQtAction = helpMenu->addAction("About Qt"' in source
assert 'auto *aboutSveButton = makeButton("About SVE"' in source
assert 'auto *aboutQtButton = makeButton("About Qt"' in source
order=['bar->addWidget(aboutSveButton);', 'bar->addWidget(aboutQtButton);', 'bar->addWidget(projectOpen);', 'bar->addWidget(projectSave);']
indices=[source.index(pattern) for pattern in order]
assert indices == sorted(indices), indices
assert 'connect(aboutSveButton, &QPushButton::clicked, aboutSveAction, &QAction::trigger);' in source
assert 'connect(aboutQtButton, &QPushButton::clicked, aboutQtAction, &QAction::trigger);' in source
assert 'Remisa Phillips' in source and 'https://github.com/yousefvand/SVE' in source
assert 'QMessageBox::aboutQt(this, "About Qt");' in source
assert 'dialog.setFixedSize(400, 400);' not in source
assert 'QDialog about(this);' in source
assert 'details->setObjectName("aboutSveText");' in source
assert 'details->setAlignment(Qt::AlignLeft | Qt::AlignTop);' in source
assert 'layout->setContentsMargins(12, 16, 12, 12);' in source
assert 'details->setOpenExternalLinks(true);' in source
print('PASS: About buttons left of Open project; left-aligned About SVE; default Qt About dialog; version 0.1.0')
PY
