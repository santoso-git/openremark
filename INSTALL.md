# openremark — installation and configuration

This takes about 20 minutes. The steps below were run exactly as written on a reMarkable 2 with **OS 3.29.1.0** (Qt 6.10.3) on 2026-10-31, with the plugin built from the 3.28.0.172 SDK (also Qt 6.10.3).

## Requirements

- **reMarkable 2.** Check the OS under Settings → General → Help → About. The plugin loads when the tablet's Qt has the same major version as the build and at least its minor version; other versions need a build with their SDK (see "Build and test" in README.md).
- A computer with `ssh` and `git` (macOS or Linux) and a USB **data** cable.
- Tablet Wi-Fi, for package downloads and API requests.
- An [OpenRouter](https://openrouter.ai) API key from [openrouter.ai/keys](https://openrouter.ai/keys), or the key of another OpenAI-compatible service.

The plugin, xovi and Vellum install under `/home/root`. xovi is *tethered*: after a reboot the tablet is stock again, and step 6 brings the plugin back. Notebooks are unaffected.

## 1. Enable SSH access

The root password and the tablet's IP addresses are at the bottom of **Settings → General → Help → Copyrights and licenses → GPLv3 Compliance** (`10.11.99.1` over USB, plus the Wi-Fi address). Keep the password private.

On OS versions above 3.20, SSH over Wi-Fi is off until you have connected over USB once. Plug the tablet in, wake and unlock it, and on your computer:

```sh
ssh-keygen -t ed25519 -f ~/.ssh/remarkable -N ""
ssh -o PubkeyAuthentication=no root@10.11.99.1 'mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys && chmod 700 ~/.ssh && chmod 600 ~/.ssh/authorized_keys && rm-ssh-over-wlan on && echo DONE' < ~/.ssh/remarkable.pub
```

Enter the tablet password when asked. This installs your key and turns on SSH over Wi-Fi in one connection; the setting survives OS updates. Then add a host alias with the tablet's Wi-Fi address:

```sh
printf '\nHost remarkable\n    HostName 192.168.1.206\n    User root\n    IdentityFile ~/.ssh/remarkable\n    IdentitiesOnly yes\n' >> ~/.ssh/config
ssh remarkable 'cat /etc/os-release | grep IMG_VERSION; ls /usr/lib/libQt6Core.so.*'
```

If the USB connection times out: use a data cable directly in the computer (no hub), keep the tablet awake and unlocked, and on macOS allow the accessory when asked. The USB link can drop when the tablet goes to sleep, which is why the command above does everything in one connection.

## 2. Install Vellum and xovi

[Vellum](https://github.com/vellum-dev/vellum-cli) is the package manager; [xovi](https://github.com/asivery/rm-xovi-extensions) loads extensions into the notebook app. On the tablet (`ssh remarkable`):

```sh
wget --no-check-certificate -O bootstrap.sh https://github.com/vellum-dev/vellum-cli/releases/latest/download/bootstrap.sh
echo "7b0deebc81b28a7d74d95c85e99a4a0a0f6ecaa5b9edb6b858ac61405978ebb9  bootstrap.sh" | sha256sum -c && bash bootstrap.sh
export PATH=/home/root/.vellum/bin:$PATH
vellum add xovi
```

The checksum is the one published in the Vellum README; if it doesn't match, the bootstrap script was updated, so take the current command from there.

## 3. Copy this repository to the tablet

On your computer:

```sh
git clone https://github.com/santoso-git/openremark
cd openremark
git archive --format=tar HEAD | ssh remarkable 'rm -rf /home/root/openremark && mkdir -p /home/root/openremark && tar -xf - -C /home/root/openremark'
```

## 4. Verify and install

On the tablet:

```sh
cd /home/root/openremark
sha256sum -c SHA256SUMS | grep -v ': OK$'     # prints nothing when every file verifies
export PATH=/home/root/.vellum/bin:$PATH
sh install.sh
```

The installer warns that your OS version is untested unless it is listed in `install.sh`; that is expected. It installs xovi's extensions through Vellum (`xovi-extensions` with `qt-resource-rebuilder`, `xovi-message-broker`, `framebuffer-spy`, and `rm-shot`) and copies the plugin, the button patch, fonts, icon and `configure.sh`.

Then build xovi's hashtable, which lets the patch find where to put the button. **Without it the button never appears.** Run it once after installing and again after every OS update:

```sh
bash /home/root/xovi/rebuild_hashtable
```

It stops the notebook app and asks you to press Enter. The app runs once on its own; enter your passcode if the tablet asks. It is done when it prints `Hashtable rebuilt`. The notebook app stays stopped afterwards; step 6 starts it.

## 5. Configure your API key

```sh
sh /home/root/.config/ink-diary/configure.sh
```

It asks for:

1. **API key** (hidden): paste it once. An OpenRouter key starts with `sk-or-` and is about 70 characters.
2. **API base URL:** Enter for OpenRouter. For a local server or the [MCP proxy](proxy/README.md), give its address ending in `/v1`; plain `http` only works for local network addresses.
3. **Model:** Enter for `nvidia/nemotron-3.5-super-vl:free`, or any model that reads images (see "Free models" in README.md).
4. **Timezone:** for example `Europe/Stockholm`.

The key is stored only on the tablet in `/home/root/.config/ink-diary/config.json`, readable only by root. To check the key without using your free quota:

```sh
K=$(sed -E 's/.*"api_key":"([^"]*)".*/\1/' /home/root/.config/ink-diary/config.json); wget -q -O - --header "Authorization: Bearer $K" https://openrouter.ai/api/v1/key; unset K
```

`401 Unauthorized` means the key was not pasted correctly. The answer also shows your free-model limit per day.

Optional settings in `config.json` (keep the JSON valid and `chmod 600` it afterwards):

- `web_search`: `false` turns web search off. Searches cost money even with free models.
- `effort`: `low`, `medium` or `high`, sent as OpenRouter's `reasoning.effort` to models that reason.
- `premium_model` and `premium_effort`: the model and reasoning effort of the Haiku button. Defaults: `anthropic/claude-haiku-5.5` and `none`.
- `tools_base_url`, `tools_api_key` and `tools_model`: the third button (a wrench) sends the question to the optional [MCP proxy](proxy/README.md) instead of OpenRouter. `tools_base_url` is the proxy's address ending in `/v1`, `tools_api_key` the proxy's `client_key`. `tools_model` defaults to the Haiku button's model. To give the button an icon of your own, replace `/home/root/.config/ink-diary/ask-tools.svg` with a square SVG; reinstalling keeps it. Without the first two the button only says what is missing. Set them with `config-model --set KEY`, which reads the value from stdin.
- `demo_mode`: `true` for an offline handwriting sample without any API request.

To change only the model later: `sh /home/root/.config/ink-diary/configure.sh --model MODEL_NAME`. Settings are read on each tap.

## 6. Start the plugin (and after every reboot)

```sh
bash /home/root/xovi/start
```

This restarts the notebook app with the extensions loaded; the screen returns to the library. Open a notebook in portrait orientation at normal zoom; the **Ask** button (a question mark) is at the top right, under the toolbar's close button, with the **Haiku** button (a lightning bolt) and the **tools** button (a wrench) below it. To test without an API key first, put `{"demo_mode":true}` in `config.json`.

**After every reboot the button is gone**, because xovi deliberately doesn't start on boot. Don't make it start automatically by editing systemd files: xovi's README warns this can leave the tablet in a boot loop. Either run `xovi/start` again over SSH, or install [xovi-tripletap](https://github.com/rmitchellscott/xovi-tripletap) and triple-press the power button after a reboot:

```sh
export PATH=/home/root/.vellum/bin:$PATH
vellum add tripletap
```

## 7. Use it

Write in the visible page area, away from the button. Tap **Ask** and keep the page still; the button becomes a stop square while it works (tap it to cancel). Hold the button to see the last status. An answer typically takes 10–30 seconds with a free model.

The **Haiku** button sends the same question to a paid model instead: by default `anthropic/claude-haiku-5.5` with reasoning off. It costs a fraction of a cent per question on your OpenRouter credits; web searches cost extra.

Try:

- `Why does the Moon shine?`
- `plot y = x^2`
- `Search weather in Stockholm today` — the sources are saved beside the answer, not shown on the page.
- `Create a 10 item checklist without labels`
- `Create a flow diagram of making tea`

Answers append below existing content as images, not editable strokes; scroll down if the page grows. To remove an answer, tap undo right after it arrives, or select it with the selection tool and delete it.

## Troubleshooting

- **No button:** check that `install.sh` finished and that you ran `xovi/rebuild_hashtable`, then `xovi/start`. Open a notebook, not the library. `journalctl -u xochitl -b | grep -E 'qmldiff|rm-shot|not loading'` shows whether the patch applied (`Processing file .../DocumentView.qml`) and any Qt-version message from the plugin.
- **Answer incomplete:** the model ran out of tokens. Reasoning models spend much of the budget thinking; the plugin allows 16,000 tokens. Try a shorter question or another model.
- **API key rejected (HTTP 401):** run `configure.sh` again; check the key as in step 5.
- **Daily free limit reached:** OpenRouter allows 50 free-model requests a day, or 1000 once your account has bought at least 10 USD of credits.
- **Page changed:** keep the page still until the answer arrives, then retry.

## Remove or restore

```sh
cd /home/root/openremark
sh uninstall.sh
```

Restart the notebook app with `xovi/start` (or reboot for a stock tablet). Uninstalling removes the plugin and the button patch and keeps `config.json` and the answer files; delete `config.json` yourself to remove the stored key. `vellum self uninstall --all` removes Vellum, xovi and its extensions.

## Privacy

The visible notebook crop is sent to `{base_url}/chat/completions` when you tap the button; web searches send the rewritten question in a second request. With OpenRouter, the data policy of OpenRouter and of the provider serving the model applies, and free models may log prompts. Share this repository or a release archive made from its committed files, never your tablet's configuration or answer directories.
