#!/bin/sh
set -eu
umask 077
base=${INK_DIARY_CONFIG_DIR:-/home/root/.config/ink-diary}
if [ "${1:-}" = --model ]; then
    [ "$#" = 2 ] || { echo 'Usage: configure.sh --model MODEL'; exit 1; }
    exec "$base/config-model" "$2" "$base/config.json"
fi
[ "$#" = 0 ] || { echo 'Usage: configure.sh [--model MODEL]'; exit 1; }
mkdir -p "$base"
ask() {
    printf '%s' "$1" > /dev/tty
    IFS= read -r answer < /dev/tty
    printf '%s' "${answer:-$2}"
}
printf 'API key (hidden): ' > /dev/tty
stty -echo < /dev/tty
trap 'stty echo < /dev/tty' EXIT HUP INT TERM
IFS= read -r diary_key < /dev/tty
stty echo < /dev/tty
printf '\n' > /dev/tty
case "$diary_key" in ''|*[!A-Za-z0-9_.-]*) echo 'Invalid key format'; exit 1;; esac
diary_url=$(ask 'API base URL [https://openrouter.ai/api/v1]: ' https://openrouter.ai/api/v1)
case "$diary_url" in https://*|http://*) ;; *) echo 'Base URL must start with https:// or http://'; exit 1;; esac
case "$diary_url" in *[!A-Za-z0-9_.:/-]*) echo 'Invalid base URL'; exit 1;; esac
diary_model=$(ask 'Model [nvidia/nemotron-3.5-super-vl:free]: ' nvidia/nemotron-3.5-super-vl:free)
case "$diary_model" in *[!A-Za-z0-9_.:/-]*|/*|.*|*/*/*) echo 'Invalid model name'; exit 1;; esac
diary_zone=$(ask 'Timezone [UTC]: ' UTC)
case "$diary_zone" in *[!A-Za-z0-9_/+-]*) echo 'Invalid timezone'; exit 1;; esac
printf '{"api_key":"%s","base_url":"%s","model":"%s","web_search":true,"timezone":"%s"}\n' \
    "$diary_key" "$diary_url" "$diary_model" "$diary_zone" > "$base/config.json.new"
mv "$base/config.json.new" "$base/config.json"
unset diary_key
printf 'Configuration saved. No UI restart needed.\n'
