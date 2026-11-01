#!/bin/sh
set -eu
cd -- "$(dirname -- "$0")"
[ "$(id -u)" = 0 ] || { echo 'Run this on the tablet as root'; exit 1; }
[ "$(uname -m)" = armv7l ] && grep -q 'reMarkable 2' /sys/devices/soc0/machine 2>/dev/null || { echo 'Requires reMarkable 2'; exit 1; }
firmware=$(sed -n 's/^REMARKABLE_RELEASE_VERSION=//p' /usr/share/remarkable/update.conf)
# Firmware versions this plugin has been run on. Others may work: the plugin checks
# Qt compatibility at load and probes the notebook app at runtime.
case "$firmware" in
    3.27.3.0|3.29.1.0) ;;
    *) echo "Warning: openremark has not been tested on OS $firmware. Build bin/ink-diary.so with the SDK for this OS version."
       echo "If the notebook app misbehaves, run uninstall.sh and restart the UI." ;;
esac
[ -f bin/ink-diary.so ] || { echo 'Build the plugin first'; exit 1; }
/home/root/.vellum/bin/vellum add 'rm-shot>=1.2.0' xovi-message-broker framebuffer-spy qt-resource-rebuilder
base=/home/root/.config/ink-diary
mkdir -p "$base"
chmod 700 "$base"
cp icons/ask.svg "$base/ask.svg"
chmod 644 "$base/ask.svg"
cp icons/ask-haiku.svg "$base/ask-haiku.svg"
chmod 644 "$base/ask-haiku.svg"
# The tools button keeps an icon of your own: replace ask-tools.svg on the tablet and reinstalls leave it.
[ -f "$base/ask-tools.svg" ] || cp icons/ask-tools.svg "$base/ask-tools.svg"
chmod 644 "$base/ask-tools.svg"
cp fonts/DejaVuSansMono.ttf "$base/DejaVuSansMono.ttf"
chmod 644 "$base/DejaVuSansMono.ttf"
cp fonts/Caveat.ttf "$base/Caveat.ttf"
chmod 644 "$base/Caveat.ttf"
cp bin/config-model "$base/config-model"
chmod 700 "$base/config-model"
cp configure.sh "$base/configure.sh"
chmod 700 "$base/configure.sh"
cp bin/ink-diary.so /home/root/xovi/extensions.d/ink-diary.so.new
chmod 755 /home/root/xovi/extensions.d/ink-diary.so.new
mv /home/root/xovi/extensions.d/ink-diary.so.new /home/root/xovi/extensions.d/ink-diary.so
cp inkDiary.qmd /home/root/xovi/exthome/qt-resource-rebuilder/inkDiary.qmd
chmod 644 /home/root/xovi/exthome/qt-resource-rebuilder/inkDiary.qmd
printf 'Installed. Configure with: sh /home/root/.config/ink-diary/configure.sh\nRestart xochitl through /home/root/xovi/start to load the extension.\n'
