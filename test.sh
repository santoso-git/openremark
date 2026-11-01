#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
# Timezone abbreviations (EDT/EST) in the clock tests depend on the locale.
export LC_ALL=en_US.UTF-8
mkdir -p build-host
c++ -std=c++17 -fPIC tests/core.cpp -o build-host/core $(pkg-config --cflags --libs Qt6Gui Qt6Core)
QT_QPA_PLATFORM=offscreen ./build-host/core "$@"
c++ -std=c++17 -fPIC tests/plot.cpp -o build-host/plot $(pkg-config --cflags --libs Qt6Gui Qt6Core)
QT_QPA_PLATFORM=offscreen ./build-host/plot
c++ -std=c++17 -fPIC tests/api.cpp -o build-host/api $(pkg-config --cflags --libs Qt6Network Qt6Core)
QT_QPA_PLATFORM=offscreen ./build-host/api
c++ -std=c++17 -fPIC tests/web.cpp -o build-host/web $(pkg-config --cflags --libs Qt6Network Qt6Gui Qt6Core)
QT_QPA_PLATFORM=offscreen ./build-host/web
c++ -std=c++17 -fPIC tests/markdown.cpp -o build-host/markdown $(pkg-config --cflags --libs Qt6Gui Qt6Core)
QT_QPA_PLATFORM=offscreen ./build-host/markdown
c++ -std=c++17 -fPIC src/config-model.cpp -o build-host/config-model $(pkg-config --cflags --libs Qt6Core)
python3 tests/config-model.py
c++ -std=c++17 -fPIC tests/templates.cpp -o build-host/templates $(pkg-config --cflags --libs Qt6Gui Qt6Core)
QT_QPA_PLATFORM=offscreen ./build-host/templates
c++ -std=c++17 -fPIC tests/visuals.cpp -o build-host/visuals $(pkg-config --cflags --libs Qt6Gui Qt6Core)
QT_QPA_PLATFORM=offscreen ./build-host/visuals
c++ -std=c++17 -fPIC tests/calendar.cpp -o build-host/calendar $(pkg-config --cflags --libs Qt6Gui Qt6Core)
QT_QPA_PLATFORM=offscreen ./build-host/calendar
python3 tests/configure.py
