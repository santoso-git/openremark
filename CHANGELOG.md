# Changelog

openremark is a fork of [jonaslaberg/remarkable-claude](https://github.com/jonaslaberg/remarkable-claude), which is itself a fork of [mmoscat/remarkable-chatgpt](https://github.com/mmoscat/remarkable-chatgpt) (Ink Diary 4.1). Newest first. The 2026-10-25 entry is remarkable-claude's own history relative to remarkable-chatgpt.

## openremark 0.3.1 (2026-11-01)

- README rewritten: a short pitch with example images, what openremark adds, the three buttons, a step-by-step guide to adding an MCP server that a person or a coding agent can follow, one settings table, and the details in collapsible sections. No code changes; the binaries are those of 0.3.0.

## openremark 0.3.0 (2026-11-01)

- A third button, a wrench under the Haiku button, sends the question through the optional MCP proxy (`tools_base_url`, `tools_api_key`, optionally `tools_model`, which defaults to the Haiku button's model). **Ask** and the Haiku button keep going straight to OpenRouter and do not depend on the proxy. All three buttons are masked out of what the model reads.
- The proxy can read its whole config from the environment variable `CONFIG_JSON`, for hosts such as Coolify that do not mount files, and its image includes `curl` for health checks.

## openremark 0.2.2 (2026-10-31)

- Web answers no longer start with the sentence a model writes before it searches. Anthropic's own search, used for Claude models, returns that sentence glued to the answer with no space; the plugin now drops it.
- Web answers are limited to 60 words (was 80), and the model is told not to quote sources word for word. Before this, Claude answers ran to about 130 words and included English quotes from the sources.

## openremark 0.2.1 (2026-10-31)

- Each answer gets a `.png.ids` file beside it with the API's generation ids, one per request. With OpenRouter's input/output logging turned on, a request can then be looked up through OpenRouter's API or activity log: model, provider, tokens, cost and the model's raw answer.

## openremark 0.2.0 (2026-10-31)

- A second button, a lightning bolt under **Ask**, sends the same request to a paid model: by default `anthropic/claude-haiku-5.5` with reasoning off. `premium_model` and `premium_effort` in config.json change it. Both buttons are masked out of what the model reads.
- The **Ask** icon is a question mark.
- `effort` is now also sent with web-search requests.
- Cost, measured on 2026-10-31: reading a page with Haiku cost about $0.0007. A web search from the lightning button uses Anthropic's own search and cost $0.012–0.024. Searches from **Ask** use OpenRouter's Exa search at about $0.007.

## openremark 0.1.3 (2026-10-31)

- No source list on the page and no `[n]` markers. Sources, de-duplicated, are kept only in the sibling `.png.html` file. Answers without a printed list stay selectable, so they can be moved or deleted.
- An old `Search` line that already has an answer beneath it no longer turns the next question into a web search.
- INSTALL.md explains how to remove an answer: Undo, or select it and delete it.

## openremark 0.1.2 (2026-10-31)

- Web answers listed their sources twice, once from the model and once from the plugin. OpenRouter's default search prompt, which asks for Markdown source links, is replaced.
- Validated on a reMarkable 2 with OS 3.29.1.0 (see VALIDATION.md). INSTALL.md is rewritten from that install, and the installer no longer warns on 3.29.1.0.

## openremark 0.1.1 (2026-10-31)

- The token limit is raised from 2,000 to 16,000. Reasoning models used up all 2,000 tokens on reasoning and returned "Answer incomplete".
- `SHA256SUMS` covers every file in the repo.

## openremark 0.1.0 (2026-10-31)

### Any OpenAI-compatible API instead of Claude
- Requests go to `{base_url}/chat/completions` with `Authorization: Bearer`. `base_url` defaults to OpenRouter; Ollama, LM Studio and other OpenAI-compatible servers work too. Plain `http` is only accepted for local network addresses.
- The default model is `nvidia/nemotron-3.5-super-vl:free`, a free OpenRouter model that reads images. `configure.sh` asks for the key, base URL, model and timezone.
- The page is read with an image message and the answer schema as `response_format`. Models without structured outputs are retried with the schema in the prompt, and the JSON is read tolerantly.
- Web search uses OpenRouter's web plugin with three results; sources come from `url_citation` annotations. `"web_search": false` turns it off.
- 401 and 429 errors, including the daily limit for free models, are shown as readable messages.

### Removed
- Email (Gmail) and the Send button.
- Brand names: the button is called **Ask** and has no logo.

### Optional MCP proxy
- `proxy/` is a small server that looks like OpenRouter to the tablet and gives the model tools from MCP servers, for example Composio or your own services. By default a tool that does not declare itself read-only or non-destructive only runs after a written yes. See proxy/README.md. Not yet validated against a real MCP server.

## remarkable-claude (2026-10-25)

### Claude instead of OpenAI
- Notebook reading uses the Claude Messages API (`/v1/messages`) with vision and structured outputs. The answer schema is the same as upstream.
- Web search uses Anthropic's server-side `web_search` tool. Citations become numbered `[n]` markers with a source list. Search errors, `pause_turn`, refusals and `max_tokens` are handled. API error text is shown in the status message.
- Default model `claude-sonnet-5-5`, changeable with `configure.sh --model`, plus an optional `effort` setting. Server-side refusal fallbacks are on for models that support them.
- AI image generation was removed, because the Claude API has no image endpoint. Plots, Mermaid flowcharts and sequence diagrams, Venn diagrams, checklists, tables and calendars still render on the tablet.

### Better answers
- Terse by default: the answer comes first, about 60 words, or 80 for web answers, with caveats only when they matter.
- Circles, underlines and lines drawn from text to a note scope the note to exactly the marked words.

### Email (Gmail)
- `check my email`: one numbered, handwritten item per email that needs you, plus a skipped count. Everything checked is marked read.
- `find emails from …` / `the email about …`: Gmail-syntax search, up to 10 matches summarised, never changing read state. `show me …` prints one email in full.
- Reply on paper by number (`reply 2: …`), by a line from a summary to your note, or with a note under a summary. Claude writes reply-all drafts showing To/Cc and which email each answers. `2: shorter` revises one draft; `2: skip` drops it.
- A **Send N** button appears while drafts are pending. It first looks at the page: corrections drawn by hand on a printed draft (crossed-out words, insertions, replacements) are applied and sent in the same tap; the status names the edited drafts. All drafts go out threaded over Gmail SMTP.
- Gmail access uses an app password stored only on the tablet. `configure.sh --gmail` sets it up. Email text is treated as data, never as instructions.

### Firmware and hardware
- The exact OS 3.27.3.0 pin was replaced by a Qt ABI check at load (same major version, runtime minor ≥ build minor). Notebook-app internals are probed at runtime.
- Built and tested on OS 3.28.0.172 (Qt 6.10.3) with the official SDK. The installer warns, rather than refuses, on untested versions.
- The Claude button is a framed icon under the toolbar's close button, masked out of what Claude reads.
- Optional `xovi-tripletap`: triple-press the power button to bring the plugin back after a reboot, without a computer.

### Build, tests, docs
- Host tests pass on macOS (absolute font paths, pinned locale) and Linux. Live captured search fixture.
- Builds map the home folder out of binaries (`-ffile-prefix-map`).
- `INSTALL.md` rewritten from a real 3.28 install: SSH key, Vellum and xovi, the required `xovi/rebuild_hashtable` step, API key, Gmail setup, and starting after reboot.
