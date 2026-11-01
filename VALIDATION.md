# Validation status

## openremark 0.3.0 and the MCP proxy — reMarkable 2, OS 3.29.1.0 (2026-11-01)

- Proxy at 2011570 running in Docker (Coolify) on a VPS behind HTTPS, configured through `CONFIG_JSON`, with one MCP server: the author's own content tool, reached with a personal key over Streamable HTTP. Proxy tests 46/46 and typecheck pass.
- Wrench button on the tablet (0.3.0) with `tools_base_url` and `tools_api_key`; model `anthropic/claude-haiku-5.5`, reasoning off.
- Before 2011570 the model got only the service name. Asked on the page to create a newsletter draft, it used no tool, wrote the text itself and invented a discount code. 2011570 passes each server's own MCP instructions to the model, tells it to use a service's tools for anything in that service, and forbids inventing codes, prices or offers.
- After 2011570, from a direct request to the proxy: "How many newsletters are drafts right now?", which does not name the service. The model called the service's newsletter list tool and answered with the real draft, in about 4 s.
- From the tablet: a handwritten request to create a draft. The model read the page in the same call, called the service's tool for creating ideas, and the service created an idea and a post awaiting approval. The answer on the page named both ids. About 62 s from capture to insertion (three model requests and one tool call), $0.0009 for the final request. One handwriting error: "frakt" was read as "frukt".
- Not yet validated: the yes-and-tap-again confirmation for tools that need it, other MCP servers such as Composio, and the free model with tools.

## openremark 0.1.1 — reMarkable 2, OS 3.29.1.0 (2026-10-31)

- Tablet: reMarkable 2, OS 3.29.1.0 (Codex Linux 6.0.112), Qt 6.10.3.
- Plugin built with the 3.28.0.172 rm2 SDK (Qt 6.10.3) in a Linux cloud session; host tests 9/9 PASS on Linux and macOS.
- Vellum bootstrap, xovi 0.3.3, xovi-extensions 19.0.1 and rm-shot 1.2.0 installed; `rebuild_hashtable` set system version 3.29.1.0 and cached 20,138 entries.
- After `xovi/start`: rm-shot loaded, `inkDiary.qmd` applied to `DocumentView.qml`, no Qt-version warning from the plugin.
- Demo mode: the **Ask** button appears and the demo text is inserted below the writing.
- Live answer via OpenRouter with `nvidia/nemotron-3.5-super-vl:free`: about 10 s from capture to insertion.
- `plot y = x^2`: graph rendered on the tablet.
- `Search weather in Stockholm today`: answer with sources from `url_citation` annotations on 0.1.1. On 0.1.0 the same request failed with "Answer incomplete": the model spent all 2,000 tokens on reasoning (`reasoning_tokens: 2000`, `finish_reason: length`). 0.1.1 raises the limit to 16,000; the same request then finished in 12 s with about 2,150 reasoning tokens.
- Not yet validated at the time: the MCP proxy against a real MCP server (see 0.3.0 above), other models, other OS versions.

## Upstream history (remarkable-chatgpt / remarkable-claude)

- ARMv7 shared library compiled with the official 3.27-series reMarkable SDK.
- Host tests passed: blank/occupied page boundaries, response parsing, incomplete/error/refusal handling, and handwriting font rendering.
- Qt qmlformat parsed the inserted QML control successfully.
- Target device reached over SSH at 10.11.99.1; confirmed reMarkable 2 and OS 3.27.3.0.
- The tablet's own hashtab resolves the patch selectors to DocumentView.qml / FocusScope / root.
- Plugin loading and broker communication verified on the target tablet after the conditional-import fix.
- Probe reports a 1404 × 1872 canvas and correctly rejects the library as having no active page.
- User verified page capture and editable ink insertion. A live OpenAI response remains untested; demo mode is retained.

- First device load exposed a required import being resolved in a child renderer process without rm-shot. Replaced that import with an XOVI conditional import. Retest succeeded; the app reached the library normally.
- Demo mode added for a full local capture/render/ink test without credentials or network requests.

- Fixed false “not enough room” on the user's ruled template. Tests cover gray rules, translucent rule intersections, handwriting crossing rules/margins, faint strokes, and real ink near the bottom.
- Verified with the captured device screenshot: the final question stroke is at screen row 502, leaving over 1,100 pixels for the response. The diagnostic screenshot is not bundled in the release.

## Letter-path renderer

- Replaced row-by-row bitmap injection with smoothed centre-line path tracing. The pen stays down along each connected path.
- Host tests pass for blank images, isolated dots, closed letter loops, image bounds, font rendering, and the existing placement/API checks.
- Sample sentence: 1,263 scanline strokes reduced to 171 connected strokes (2,627 path points). This measures pen lifts, not an on-device elapsed-time benchmark.
- Preview inspected: readable Caveat handwriting, no doubled glyph outlines.
- Added a completion message showing actual ink drawing time. Hardware appearance/timing awaits the next demo run.

## Native image renderer

- User reported the letter-path version was slower on the tablet. It is superseded by a transparent handwriting PNG submitted to native notebook image insertion.
- Host tests pass for transparent background, visible ink and lossless PNG round-trip, alongside previous placement and response tests.
- Firmware image insertion support, coordinate mapping, sizing and elapsed time are pending device validation.

- User verified native image rendering looks good and is quick.
- Button changed to a 96 px icon at bottom-right, outside the screenshot working area; stop control while busy and temporary status message on completion.

## Plot support (development after v1.0)

- Host tests pass for powers/unary precedence, arithmetic/functions, malformed expressions, command/range validation, domain errors, constant plots, insufficient room, and asymptote rendering.
- Parabola preview visually inspected; correct shape, equation, grid and labeled axes.
- Structured response format follows https://developers.openai.com/api/docs/guides/structured-outputs .
- Live handwritten plot recognition and device graph placement await user testing. v1.0 archive remains unchanged.

## Append beyond viewport

- Removed remaining-screen-space rejection. Answers have a fixed 1582 px render budget and graphs render at 800 px high.
- Screen-to-scene transform may place image centers below the viewport; insertion is also offset below the controller content bounding rectangle to avoid existing off-screen content.
- Native page growth and scrolling behavior await a near-bottom device test.
- Actual QML insertion function tested with mocked scene transforms: near-bottom placement, pre-existing off-screen content, zoom/scroll mapping and inverted-orientation rejection pass. Native helper QML parses and ARM build succeeds.

## Spacing after plots

- Confirmed an opaque plot hides the middle of pale vertical template margins. The old full-height detector treated their lower segments as content through screen row 1751.
- Added thin partial-column detection with sufficient coverage/continuous runs, including darker template intersections. Existing dark strokes and faint isolated handwriting remain detectable.
- On the user's screenshot, corrected question bottom is row 1241, matching the notebook content boundary (~1244); the PNG itself had no extra padding. Screenshot is kept out of the release.
- Host placement/rendering tests pass, including a synthetic opaque plot over a ruled template and later handwritten marks.

## Web search

- Added model-directed search routing (including explicit Search requests), required hosted web search, and verified citation parsing.
- Host tests cover search routing, forced tool configuration, completed-search/source validation, Markdown citation replacement, source fallback titles, and HTML escaping.
- A live request from the tablet using its private existing configuration completed successfully and returned an official reMarkable source. No API key was copied to the computer.
- The actual live response is used to verify handwriting font coverage and source-list rendering. Handwritten routing and final notebook insertion await user testing.
- Official API reference: https://developers.openai.com/api/docs/guides/tools-web-search .

## Weather without citations

- Reproduced the Lowell MA request on the tablet: a completed web-search call returned weather text with an empty annotations list. The previous strict parser discarded it.
- Completed answers without source links are retained with a visible missing-source notice in the notebook image and HTML. Missing/failed search calls and empty answers remain errors.
- A second live Lowell lookup passed with zero citations under the new parser. Host tests and font rendering also passed for the recorded weather response. No source URLs are fabricated.
- Prompt clarifies local-date interpretation and requires observation timestamps to come from sources, not the request clock. Weather accuracy/freshness remains dependent on the API's result.

## Local timestamp correction

- Verified the tablet's UTC clock and installed America/New_York zone data: 01:44 UTC was 21:44 EDT on the previous local date.
- Search instructions now include an explicit local clock and UTC offset, rather than asking the model to convert UTC.
- A separate Checked-at timestamp is generated locally when the response arrives and included in both the image and HTML; it is not labeled as an observation time.
- Host tests pass for the reported 2026-10-20 01:42Z → 2026-10-19 9:42 PM EDT case, winter EST, and both DST transitions.
- Live weather output still supplied a canned UTC preamble despite prompt instructions. For the specific uncited hosted-weather block format, the parser now retains the conditions/forecast block and omits that unverified preamble. Regression tests cover this behavior.

## Markdown rendering (2026-10-20, after version 2.0)
- Added native QTextDocument GitHub Markdown rendering with handwriting paragraphs/headings, print tables, bundled DejaVu Sans Mono code, explicit cell borders, nested lists/tasks, quotes, emphasis, links and rules.
- Existing core/plot/web suites pass. New Markdown tests cover document structure, literal fenced code, wrapped long code/cells, source hyperlinks, image placeholders, blocked HTML/unsafe links, and input size limits.
- Built and ran the renderer test directly on the reMarkable using Qt offscreen, without modifying any notebook. Visually inspected its 1174×1604 sample; this caught and fixed missing monospace fonts and device-specific table borders.
- Text render limit is now 8000 px high instead of 1582; the existing append-below-content insertion path is unchanged. Saved HTML preserves Markdown formatting and source hyperlinks. No API credentials or notebook contents are included in test fixtures.
- Raw HTML, remote images, math/diagram extensions and code execution are intentionally unsupported; these are documented. Full notebook interaction with a newly generated Markdown API response still needs a user test after reopening a notebook.
- Installed binary SHA256 `5948e0e4bd4914fefcfe4c81a1c158f2e7950e4d069b25670a478233f6aaf358` matches the successful build. Notebook app restarted and the message broker reports `busy:false`, `message:ChatGPT`. Version 2.0 archive checksum remains unchanged.

## Current-information routing fix (2026-10-20)
- Found that the first image-reading/routing request had no current date; only the second search request had the local clock. Added shared date context to the first request as API instructions and reinforced present-date handling in web search.
- Retain the latest question transcription in all answer types. A deterministic English fallback reroutes current/recent questions to required web search, discarding the unverified text answer. Historical and timeless questions and plots retain their normal paths.
- Full host suite passes, including current-president/recent-question routing, historical president and electrical-current exclusions, local midnight rollover and daylight-saving tests.
- Tablet reports the expected UTC date and `NTPSynchronized=yes`. Live required-web-search test returned a current-date answer with three cited source URLs, without a future-date refusal. No key or private notebook content was copied into test fixtures.
- Ran the exact production first-request function against a synthetic Caveat question image on the tablet. The configured live model selected `kind=search` for “Who is the current president of the United States today?” before the fallback was needed.

## Markdown output instructions and model-only configuration
- Both ordinary and web prompts now distinguish rendered Markdown from raw Markdown source. Tables/templates should be unfenced; explicit requests for source and programming code retain fences.
- Added QtCore `config-model` helper and `configure.sh --model MODEL`. Uses QSaveFile without direct-write fallback; validates model/JSON/key presence, preserves other fields, and writes with owner-only permissions. Credentials are never printed.
- Fixture tests cover field/key preservation, permissions, invalid names, malformed JSON and missing keys. No change to the user's selected model is made by installation.
- Live test on the tablet using a synthetic handwritten version of the user's pros/cons request passed: ordinary text route, Markdown table present, no code fences. Existing core, plot, web, Markdown and new configuration tests all pass.

## Empty checklist rows
- Reproduced Qt dropping empty `- [ ]` items and joining standalone checkbox symbols across soft line breaks. Normalize blank task items with a zero-width content placeholder and standalone checkbox lines to Markdown tasks before parsing.
- Both prompts specify one task per line for unlabeled checklists. No visible placeholder labels are added.
- Host and tablet tests pass for ten empty tasks, trailing spaces, standalone square symbols, bare bracket markers and checked tasks. Assert ten task blocks and a multi-row image; preserve labeled items, fenced source, indented code and deliberately inline symbols. Visually inspected tablet preview.
- Live API test using the exact handwritten prompt returned ten unlabeled task blocks. Device pixel checks also count ten separate visible checkbox bands, beyond merely counting document blocks.

## Follow-up: reported single checkbox
- Latest two saved HTML answers contained one task item, proving missing rows were already absent before notebook insertion. Original Markdown was not retained, so those artifacts alone cannot distinguish a one-item model response from parser loss.
- A fresh live test of “create a 10 item checklist” returned ten literal `- [ ]` lines. The standalone parser produced ten tasks. Added an in-app, read-only `probe:checklist` diagnostic; the actual notebook process also returned ten boxes and a 782 px image.
- New text answers retain owner-only sibling `.png.md` files alongside PNG/HTML so a recurrence can be diagnosed without guessing. User reproduction requested; no claim of a confirmed root cause or fix yet.

## Structured templates and stronger graphical lines (2026-10-21)
- Extended the existing strict response schema with checklist/table kinds and a template object (title, count, labels, columns, rows). Blank rows are generated locally; labeled data must match declared counts. Literal cell text is escaped. Added bounds/type/count tests and retained existing text/search/plot routing.
- Table cells, checkbox outlines, horizontal dividers, quote bars and plot frames/axes now use 3 px strokes; plot gridlines use 2 px. Custom checkbox painting is restricted to real list items because Qt can retain marker metadata on subsequent non-list blocks.
- Full host suite passes, including structured exact-count checks and malformed template rejection. Visually checked blank table, mixed Markdown and checkbox previews.
- Live tablet tests using synthetic handwriting and the configured model selected checklist/table kinds with counts 10 and 6 respectively. Both rendered successfully; inspected device-produced images for full checkbox outlines and six blank table body rows.

## Writable table and calendar cell heights (2026-10-21)
- Added `cell_lines` to structured template responses and carried it through decoding to the renderer. Bounds: 1–10, compact default 1. Each additional line adds 60 px bottom padding to body cells only. Labels/date numbers remain at the top and headers remain compact.
- Prompts distinguish body-row count from height within a cell and route calendars as tables; explicitly prohibit using HTML br tags or fake rows to create writing space.
- Regression tests verify a five-row table with three-line cells grows by at least 575 px without changing table row count, and reject out-of-range cell heights. Other host suites pass.
- Live tablet request for a November 2026 calendar with three-line day cells selected a seven-column table with `cell_lines=3`. Visually inspected the device image: dates remain top-aligned, weekday headers compact, and all five week rows have additional writing space.

## Calendar default height (2026-10-22)
- Changed the generation instructions to default calendars to `cell_lines=3`, preserving explicit height requests and ordinary table defaults. No renderer/schema change; frozen v3.0 is unchanged.
- Live tablet test with synthetic handwriting “Create a calendar for November 2026” returned a seven-column table with `cell_lines=3` and rendered successfully. ARM build passed.

## Grayscale artwork and native diagrams (2026-10-22)
- Added explicit image, Mermaid and Venn routing. Artwork uses the Images API with gpt-image-1.5 by default; optional image_model is independent of text model. PNGs are bounded/decoded, composited over white and converted to Grayscale8 before the existing native-image insertion path.
- Network wrapper keeps 120 s/1 MiB defaults for Responses and uses 300 s/32 MiB for images, with bounded streaming reads and the existing cancellation/page guard. No redirect following or credential logging.
- Native diagram renderer supports a documented Mermaid subset (top-down flowcharts and sequence diagrams), and two/three-set Venn diagrams with numbered region legends. Rejects unsupported syntax/undeclared nodes/malformed Venn regions; no generated code execution or third-party rendering service.
- Full host suite and append-coordinate tests pass. New checks cover grayscale/alpha conversion, malformed image data, routing, flow/sequence/Venn outputs, and rejected unsupported inputs. ARM build passed.
- Live API tests executed on the tablet using synthetic handwriting and the user's privately loaded configuration: flowchart, sequence, two-set Venn and image routing all passed. Generated one three-panel robot/gardening comic via the actual Images API, converted to 1174×1761 Grayscale8. Inspected comic and diagram previews. These tests rendered temporary files; they did not insert test content into a user notebook. Notebook insertion reuses the previously tested image path and awaits user confirmation for these new content types.

## Flowchart parser compatibility fix (2026-10-22, after v4.0)
- Reproduced the reported “create a flow diagram of making tea” error with a live synthetic-handwriting request: the model returned unquoted node labels such as A[Start], rejected by the previous quoted-only parser.
- Replaced line regex matching with bounded parsing of quoted/unquoted labels, inline declarations, chained arrows, semicolon-separated statements, implicit references, and both pipe/text arrow labels. Label delimiters are tracked so semicolons/arrows inside quoted labels remain literal. Repeated matching declarations are accepted; conflicting labels, malformed delimiters and unsupported syntax are rejected. Existing node/edge limits remain.
- Visual regression tests pass, including the exact previously rejected response, graph structure/count assertions, malformed input, and unchanged image/sequence/Venn tests. Inspected the reproduced tea flowchart. ARM plugin build passed. Three new live tablet requests using the exact tea prompt all rendered successfully (two 1174×2370, one 1174×2670); no test images inserted into a notebook.
- Frozen v4.0 backup remains unchanged.

## Include top-of-page handwriting (2026-10-22, after v4.0)
- Removed the obsolete 170-pixel top exclusion left over from the former button position. Production capture now uses QRect(150,0,1174,1752), preserving the prior side and bottom boundaries and excluding the bottom-right button. Both the API upload and page-change comparison use the same expanded crop.
- Ink placement maps back using the crop origin, so handwriting near the top is recognized and lower-page answer placement stays unchanged. Added production-crop regression checks for strokes at screen rows 0 and 90, lower-page ink at 1700, and exclusion of the bottom-right button.
- Core tests and append-coordinate regressions pass. Existing v4.0 release archives remain unchanged.

## Compact flowcharts and computed calendars (2026-10-23, after v4.0)
- Flowchart nodes now wrap in topological order across up to three columns, reversing direction on alternate rows. Adjacent arrows use direct connections; longer/return arrows use row gaps and side gutters. Nine-step tea regression shrank from 2670 to 960 pixels high without shrinking text. Complex diagrams may still have crossing connectors.
- Added a calendar response kind with year/month/week_start fields. QDate computes the first weekday, number of days, and all date cells locally; model-generated date rows are not used for this route. Sunday/Monday starts supported; existing three-line calendar spacing is retained. Plain current-month calendar requests bypass the freshness/search fallback.
- Full host suite passed, including every month in 1900, 2000, 2024, 2026 and 2100 with both week starts, malformed calendar fields, explicit October 1 Thursday/31 Saturday assertions, rendering and calendar routing. Re-ran affected visual/calendar checks after adjusting an edge-label position. Append-coordinate tests and ARM build passed.
- Live tablet API tests with synthetic handwriting passed: tea flowchart 1174×960, October 2026 Sunday-start and Monday-start calendars both 1174×1121. Verified requested month/year/start, three-line spacing and first date under Thursday; inspected device-rendered flowchart and Monday-start preview. Test output was saved to temporary files, not inserted into a notebook. Frozen v4.0 remains unchanged.

## Claude port (fork, 2026-10-25)

Everything above describes upstream's OpenAI build. For the Claude port:

- **Device test passed** on 2026-10-25: reMarkable 2, OS 3.28.0.172, model `claude-sonnet-5-5`. Installed with Vellum 0.3.1, xovi 0.3.3 and xovi-extensions 19.0.0. The hashtable was rebuilt on the device, and the QMD patch applied to `/qml/device/view/documentview/DocumentView.qml`. `ink-diary.so` was mapped into xochitl with no Qt-gate warning. A handwritten weather-forecast question was read from the page, routed to `kind=search`, and answered with two citations. The answer was inserted below the writing as a 1174×1432 image.
- **ARM build** on 2026-10-25 with the official 3.28.0.172 rM2 SDK (aarch64 host toolchain, Ubuntu LTS VM under Lima). Built against Qt 6.10.3, and the binary only requires `Qt_6`/`Qt_6.10` symbol versions. The target tablet runs 3.28.0.172 with `libQt6Core.so.6.10.3`, and every linked Qt library (Core, Gui, Qml, Quick, Svg, Network) is present in `/usr/lib`. The exact 3.27.3.0 firmware pin was replaced with a load-time Qt compatibility check: same major version, and a runtime minor version at least the build's.
- **Live web search verified** on 2026-10-25 with `claude-sonnet-5-5`. The request was built by `diarySearchPayload` + `diaryWithFallbacks` and sent with the fallback beta header. It returned HTTP 200 in 4.6 s with one search and 10 results, and `parseDiarySearch` produced the cited answer and source list. A live response, with encrypted fields stripped, is kept as `tests/fixtures/search-tallest-building.json` and checked in `tests/web.cpp`.
- **All host tests pass** on macOS (Homebrew Qt 6), 2026-10-25: `core`, `plot`, `web`, `markdown`, `config-model`, `templates`, `visuals`, `calendar`. Two test-only fixes made that possible. macOS Qt only registers application fonts from absolute paths, so the tests now load `fonts/*.ttf` via `QFileInfo::absoluteFilePath()`; the plugin already uses absolute paths on the tablet. `test.sh` now pins `LC_ALL=en_US.UTF-8`, because the EDT/EST clock assertions depend on the locale.
- `src/diary.cpp` was compile-checked on the host with `-fsyntax-only` against Qt 6 (stub Linux input headers). There were no errors in `diary.cpp` or the API headers; the reported errors were all in `inserter.inc` and caused by the stub headers.
- The web-search fixtures in `tests/web.cpp` follow the example response in Anthropic's web-search tool documentation, not a captured live response.

## Email (2026-10-25)

- Host tests (`tests/email.cpp`): RFC 2047 headers, reply-all recipients, `Re:` subjects, quoted-printable MIME, state round-trip and file mode, literal-safe IMAP parsing (a body containing `A003 OK` stays intact), multipart/HTML/base64 bodies, attachment names, triage and summary layout, drafts with revise/skip, placeholder-draft rejection, SMTP dot-stuffing, and partial send results. `config-model --set` reads secrets from stdin and accepts only known keys.
- Live from the Mac (read-only, `EXAMINE` + `BODY.PEEK`): login, 30 unread before and after, Gmail search (`from:google newer_than:7d` → 4), and a UTF-8 search via literal. All Mail isn't exposed over IMAP on the test account, so searches fall back to the inbox and the page says so.
- Live triage of 30 real unread messages: 1 needing attention, 29 skipped.
- Live drafts: the first version returned placeholder bodies (`x`, 3 of 3). Stating the ask explicitly before the email data fixed it: 3 of 3 proper replies, reply-all recipients computed locally, `In-Reply-To` set.
- On the tablet (OS 3.28.0.172), driven through the xovi-message-broker pipe (`/run/xovi-mb`):
  - The status reports `drafts`.
  - The read-only check probe triaged on-device over TLS IMAP and the Claude API; the search probe found 4.
  - A seeded test draft to the account's own address was sent with `ink-diary$send` (`Sent 1`) and arrived intact (UTF-8 and a lone `.` line preserved).
  - The unread count was unchanged by the probes.
- **Not yet exercised on the device:** the handwritten path end to end (reading "check my email" from a page, inserting the summary, marking read, replying on paper, and the on-screen Send button).
