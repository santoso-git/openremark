# openremark — reMarkable 2

Write a question in a notebook, tap **Ask**, and the answer appears beneath it in handwriting. openremark sends the visible page to any OpenAI-compatible chat API — by default [OpenRouter](https://openrouter.ai), which has free models that read images — and renders the answer locally in Caveat handwriting. No companion computer is needed after setup.

**Installation and configuration: [INSTALL.md](INSTALL.md).** Optional tools from MCP servers (Composio, your own services): [proxy/README.md](proxy/README.md).

A fork of [jonaslaberg/remarkable-claude](https://github.com/jonaslaberg/remarkable-claude), itself a fork of [mmoscat/remarkable-chatgpt](https://github.com/mmoscat/remarkable-chatgpt) (Ink Diary 4.1). GPL-3.0.

Target: **reMarkable 2**, any OS version whose Qt is binary compatible with the SDK used for the build. The checked-in `bin/ink-diary.so` is built with the **3.28.0.172** SDK. Which OS versions it has actually run on is recorded in [VALIDATION.md](VALIDATION.md). This is a development prototype.

### Changes from remarkable-claude

- Requests go to `{base_url}/chat/completions` with `Authorization: Bearer`. `base_url` defaults to `https://openrouter.ai/api/v1`; any OpenAI-compatible service works, including Ollama or LM Studio on your own network. Plain `http` is only accepted for local network addresses.
- The page is read with an image message and the answer schema as `response_format` (structured outputs). Models without structured outputs are retried with the schema in the prompt, and the JSON is read tolerantly.
- Web search uses OpenRouter's web plugin; sources come from its `url_citation` annotations.
- Email was removed. The button is called **Ask** and has no brand logo.

## Free models

OpenRouter model ids ending in `:free` cost nothing. List the free ones that read images:

```sh
curl -s https://openrouter.ai/api/v1/models | python3 -c "import json,sys; [print(m['id']) for m in json.load(sys.stdin)['data'] if m['id'].endswith(':free') and 'image' in m['architecture']['input_modalities']]"
```

The model must accept images; structured-output support makes answers more reliable. The default is `nvidia/nemotron-3.5-super-vl:free`. Free models are limited by OpenRouter to 20 requests a minute and **50 requests a day**, or 1000 a day once your account has bought at least 10 USD of credits in total. When the daily limit is reached the button says so. The free selection changes over time.

## Demo mode

For offline testing, config.json may contain `{"demo_mode":true}`. Holding the icon shows **Ask (demo)** and inserts a fixed handwriting sample below your writing without making any API request. Running `configure.sh` replaces demo mode with the real API configuration.

## First use

1. Create an API key at [openrouter.ai/keys](https://openrouter.ai/keys) and configure it privately on the tablet:
   ```sh
   sh /home/root/.config/ink-diary/configure.sh
   ```
   It asks for the key (hidden), the base URL (Enter for OpenRouter), the model (Enter for the default) and your timezone, for example `Europe/Stockholm`.
2. Enable Wi-Fi. Open a disposable notebook with a **blank or lightly ruled template**, in **portrait orientation**, at normal zoom.
3. The answer image uses black handwriting independently of the selected pen.
4. Write a short question anywhere in the visible capture area, including at the top, away from the left toolbar. Leave the lower half clear.
5. Tap the **Ask** icon at the top right, under the toolbar's close button. Keep the page still and lift your pen until it finishes. While working, the icon becomes a stop square; tap it to cancel. Completion and error messages appear briefly above the button. Hold the icon to see its current status. The lightning-bolt button below it does the same with a paid model, by default `anthropic/claude-haiku-5.5` with reasoning off (`premium_model` and `premium_effort` in config.json). The wrench button below that sends the question through the optional [MCP proxy](proxy/README.md), so the model can use tools such as your calendar or your own services (`tools_base_url` and `tools_api_key`; see INSTALL.md). The other two buttons never go through the proxy.
6. The answer is laid out in the Caveat handwriting font and inserted as one transparent image. Letters are not editable pen strokes. The native notebook app controls image sizing and selection.

## Web search

Write `Search` followed by a question to explicitly request a web lookup, for example `Search what is the screen resolution of reMarkable 2?`. Questions about current facts, news, weather, prices and schedules are also routed to search automatically by the model. Include a location when needed.

The first request reads the page and routes the question. Search requests then make a second request with OpenRouter's web plugin (`plugins: [{"id":"web"}]`, three results). Only the answer goes on the page. The sources, built from the response's `url_citation` annotations, and the time of the search are saved in a sibling `.png.html` file in `/home/root/.local/share/ink-diary/answers/` with clickable links. Links are never invented; when a search returns no annotations, that file says source links are unavailable.

Search requests use your local timezone (the `timezone` field), and a tablet-generated "Checked at" line in the `.png.html` file records when the response arrived.

**Web search costs money even with free models** (OpenRouter bills the search per request). Set `"web_search": false` in config.json to turn it off; search requests then get a short note instead.

## Plot commands

Write `plot` followed by one equation, then tap the Ask icon. Examples:

- `plot y = x^2`
- `plot y = sin(x)`
- `plot y = 2x + 1 from x = -10 to 10`

Handwritten superscripts are accepted through image recognition. The model transcribes the equation; a restricted on-tablet math parser computes the curve. No generated code is executed and no image-generation service is used. The interpreted equation appears above the graph so you can check recognition.

The default x range is -5 to 5; y is automatically scaled from sampled values. Axes have independent scales. Trigonometric functions use radians; ln/log mean natural logarithm. Supported operations: +, -, multiplication, division, powers, parentheses, sin, cos, tan, sqrt, abs, exp, ln, log, pi and e. Only explicit single-variable functions y=f(x) are supported, not implicit equations, multiple curves or parametric plots. Very narrow features can be missed by sampling; undefined values and detected asymptotes break the curve.

Answers and plots append below existing page content, extending beyond the visible screen when necessary. Scroll down to see the rest. Graphs use a white background for legibility over ruled templates and are inserted as image objects. Ordinary questions retain handwriting-image answers. Demo mode remains the original fixed text sample. The configured model must support structured outputs (all current Claude models do).

## Generated images and diagrams

Image generation (comics, illustrations) is not available. Such requests get a short text reply, often suggesting a diagram instead.

Diagrams are drawn locally with 3–4 px black lines and print labels. Examples:

- `Create a Mermaid flowchart for making tea.`
- `Create a sequence diagram of a tablet sending a question to a server.`
- `Create a Venn diagram comparing cats and dogs.`

The model produces a restricted Mermaid subset: flowcharts using TD/TB source syntax (up to 12 nodes and 24 arrows), laid out in a three-column wrapping grid, or sequence diagrams (2–5 participants and up to 20 messages). This is a native subset renderer, not the complete Mermaid JavaScript engine. Class, ER, Gantt, state diagrams, arbitrary Mermaid directives and mixed Markdown/diagram documents are not supported. Flowcharts support rectangular, rounded and diamond nodes, arrows and short edge labels. Complex branching/loop layouts may need a simpler request.

Venn diagrams support two or three sets with numbered exclusive regions and a readable content legend underneath. Circle areas are not proportional to counts. Diagram source is saved privately beside answer PNGs as `.png.source.txt`; ordinary text keeps `.png.md`.

## Prototype limits

- Portrait orientation and short answers only. Light gray full-width horizontal rules and thin vertical margin lines (including those partly hidden by plots) are filtered out when locating handwriting. Dark or complex templates may still be treated as ink.
- The working area is screen pixels x=150..1323, y=0..1751. Keep questions there. The side and bottom UI margins are cropped out of the uploaded screenshot; the top edge is included.
- Native image insertion may extend the notebook page below the viewport. Scroll down to follow it; there is no automatic scrolling or new-page creation. The visible question still needs to be inside the capture area. Answers have a fixed image-size limit, independent of remaining screen space.
- Visible page content supplies conversation context; there is no separate persistent conversation history.
- Page identity and a second screenshot are checked before insertion. Keep the page still until completion. Native image insertion is asynchronous; cancellation cannot retract an insertion already handed to the notebook app.
- Font coverage limits output languages and symbols. Unsupported characters produce an error rather than missing glyphs.
- Native image insertion and screen-to-page coordinate mapping must be supported by the installed firmware.
- Firmware updates require revalidation. Instead of a fixed version, the extension declines to load if the firmware's Qt is older than, or a different major version from, the Qt it was built against. Notebook-app internals (page view, native image insertion, screen size) are probed at runtime, and a missing one produces a status message. A firmware update can still change those internals: rebuild with the matching SDK, and remove the QMD patch if the UI misbehaves.

## Installation

Follow **[INSTALL.md](INSTALL.md)**: SSH access, Vellum and xovi, copying this repo, `install.sh`, the required `xovi/rebuild_hashtable` step, your API key, and `xovi/start`. xovi is tethered, so the button is gone after a reboot: triple-press the power button (with the optional xovi-tripletap starter from INSTALL.md), or run `bash /home/root/xovi/start` over SSH.

## Removal

```sh
sh /home/root/openremark/uninstall.sh
```

Restart the UI afterwards. Your notebooks and existing Vellum packages remain. The API key/configuration remains at `/home/root/.config/ink-diary/config.json`; remove that specific file if you want to delete the saved credential.

## Privacy and network behavior

Only tapping Ask starts a request. The cropped visible notebook image goes to `{base_url}/chat/completions` over HTTPS with certificate verification (plain http only on your local network). The data policy of OpenRouter and of the model provider it routes to applies; free models may log prompts. Web searches send the rewritten question in a second request. Screenshots use a private temporary directory and are deleted on normal completion/error. Answer PNGs remain in the root-only directory `/home/root/.local/share/ink-diary/answers` because the notebook app imports them asynchronously. Beside each answer, a `.png.ids` file holds the API's generation ids (one per request), so the request can be looked up in OpenRouter's activity log; with input/output logging enabled there, that includes the page image and the model's raw answer. Notebook content and credentials are not printed to logs. The API key is saved locally with root-only permissions.

## Build and test

Build with the official rM2 SDK matching the tablet's OS version, listed at https://developer.remarkable.com/links . The checked-in binary uses 3.28.0.172:
https://storage.googleapis.com/remarkable-codex-toolchain/3.28.0.172/remarkable-production-image-5.8.203-rm2-public-aarch64-toolchain.sh (an x86_64 variant is also published).

The SDK runs on Linux. On a Mac, use a Linux VM (for example Lima, `limactl start template:ubuntu-lts`), and build from a copy on the VM's own disk: macOS filesystems are case-insensitive, and the repo's `VERSION` file then shadows the C++ `<version>` header.

```sh
export SDK_ENV=/absolute/path/to/sdk/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi
export XOVI_REPO=/absolute/path/to/xovi
./build.sh
```

Output: `bin/ink-diary.so` (ARMv7). Host checks require a C++ compiler and Qt6 Core/Gui development packages:

```sh
./test.sh
```

The checks cover placement boundaries, chat-completions JSON handling (finish reasons, refusals, tolerant JSON, url_citation sources, 401/429 messages, base_url rules), configure.sh, and actual font rendering. They do not test live network requests or physical pen injection.

Upstream provenance and licenses: THIRD_PARTY.md. API documentation: https://openrouter.ai/docs

Append-placement regression checks can also be run with `node tests/append.cjs`.

## Instruction notebook

`docs/notebook/build.py` makes a short how-to as a PDF the size of the rM2 screen, one feature to a page. It needs Python 3 and Google Chrome or Chromium (set `CHROME=/path/to/chrome` if it isn't found); the fonts are vendored, so the build makes no network requests:

```sh
python3 docs/notebook/build.py                          # writes docs/notebook/claude-notebook.pdf
uv run --with pymupdf docs/notebook/build.py --png DIR  # also renders each page to DIR/page-NN.png
```

Put `claude-notebook.pdf` on the tablet like any other PDF: drag it into the reMarkable desktop app, upload it in the mobile app, or use the tablet's USB web interface. The PDF is not committed.

## Markdown answers

Answers now render GitHub-style Markdown using the tablet's Qt text engine: six heading levels, bold/italic/strikethrough, ordered and unordered nested lists, task checkboxes, tables with column alignment, inline and fenced/indented code, blockquotes, links, horizontal rules, and Markdown escapes. Paragraphs and headings use Caveat handwriting; tables and code use readable print/monospaced fonts. Code is displayed, not executed, and has no syntax highlighting. Links and checkboxes in the notebook are static ink images; links work in the saved sibling HTML file.

Text answers can grow to 8,000 pixels high and append below existing content. Very large answers or tables are rejected with a request to shorten them. Lines wrap, including long code lines. The model is asked for concise answers and tables with at most five columns. Citations, source notices, and local checked-at times remain visible.

Markdown dialects differ: raw HTML, LaTeX, Mermaid, footnotes, and embedded remote images are not supported. Image markup becomes an alt-text placeholder; it never downloads an image or reads a local file. Function plots still use the existing `plot` command. Formatting changes apply to new answers; existing notebook images stay as they are.

A synthetic example is available in `markdown-preview.png`. The archived version 2.0 is unchanged.

### Current dates and recent facts

Both the notebook-reading stage and web-search stage receive the tablet's current local date, time, and timezone. The reading stage is instructed to treat this as the present even when it is later than the model's training cutoff, and to disregard stale prior notebook answers. A fallback checks the transcribed latest question for English freshness phrases and current-officeholder questions and forces the search route if the model incorrectly labels it ordinary text. Other languages still rely on model routing. This does not guarantee source accuracy; search is instructed to admit when current facts cannot be verified. The tablet clock must remain correct. Prefixing a question with `Search` remains available for explicit web lookup.

### Change only the model

In reManager's terminal, run:

```sh
sh /home/root/.config/ink-diary/configure.sh --model google/gemma-4-31b-it:free
```

The `--model` option preserves the saved API key and every other configuration field. It validates the name and existing JSON, then saves atomically with owner-only permissions. No restart is needed; the next question uses the new model, for both notebook reading and web search. Use a model that accepts images, for example `nvidia/nemotron-3.5-super-vl:free` (default) or `google/gemma-4-31b-it:free`; see Free models above. Quality and latency differ. Running the helper without `--model` repeats the full key and model setup.

For models that reason, `"effort":"low"` (or `medium`, `high`) in config.json is sent as OpenRouter's `reasoning.effort`.

The prompts say the answer is rendered as GitHub Markdown. Tables and templates must be emitted outside fences; fences are reserved for programming code or explicitly requested raw Markdown source.

Blank checklists are supported: request `create a 10 item checklist with checkboxes without labels`. Each empty task item gets an invisible layout placeholder so the checkbox keeps its own writable row. Text answers also keep the model's original Markdown in a private sibling `.png.md` file. The diagnostic message `ink-diary$probe` with argument `checklist` tests ten synthetic blank rows inside the notebook process without inserting anything.

## Structured templates

Checklist and single-table creation requests now use dedicated response kinds in the existing API call. The model supplies a count and optional labels for checklists, or columns and rows for tables. The plugin generates blank rows itself, escapes cell/label text, validates counts and dimensions, and renders the result. Blank templates cannot collapse because of empty model-generated Markdown. Supported limits are 1–50 items/body rows and up to 8 table columns; image size limits still apply. Mixed explanatory answers and web-search answers continue to use Markdown. No additional classifier request is needed.

Examples: `Create a 10 item checklist without labels`; `Create a pros and cons table with 6 blank rows`. With no number specified, the prompts default to ten checklist items or five blank table rows. Handwriting recognition/classification can still make mistakes.

Checkbox outlines, table borders, horizontal dividers, quote bars and plot frames/axes use 3-pixel strokes at source-image resolution; plot gridlines use 2 pixels. Checkbox outlines are painted explicitly for consistency. These changes apply to new answers. Saved `.png.md` files contain model Markdown for ordinary answers and locally generated Markdown for structured templates.

### Writable calendar/table cells

Table templates now carry `cell_lines` (1–10). `1` keeps the compact layout; each additional handwriting line reserves 60 source-image pixels beneath the cell's text. Header rows stay compact. For example: `Create a calendar for November 2026 with each day cell 3 rows tall so I can write in it`. For monthly calendars the model identifies the month, year and requested height; the tablet computes the Gregorian dates and weekday positions using QDate. Sunday is the default first column; ask for Monday-start if preferred. Calendar cells default to three writing lines. HTML `<br>` tags are not used for spacing. This also works for other tables with requested writing space. Large layouts may extend down the notebook page or reach the existing image-height limit.

Calendar requests now default to three handwriting lines per day cell, even when you do not specify a height. An explicit requested height takes precedence. Ordinary tables retain their compact one-line default. This change is newer than the frozen version 3.0 snapshot.
