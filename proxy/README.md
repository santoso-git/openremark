# openremark proxy (optional)

Gives the tablet's **Ask** button tools from any [MCP](https://modelcontextprotocol.io) server — for example your mail, calendar, Notion or GitHub through Composio, or your own service.

The tablet does not know the proxy exists. To the tablet it looks exactly like OpenRouter: an OpenAI-compatible `POST /v1/chat/completions`. The tablet's wrench button sends its questions to the proxy (`tools_base_url`); the **Ask** and Haiku buttons keep going straight to OpenRouter, so they still work when the proxy is down. For each question the proxy:

1. lists the tools of every configured MCP server (cached for 5 minutes),
2. offers them to the model as functions named `<server>__<tool>`,
3. runs the tools the model asks for (at most `max_tool_steps` rounds),
4. asks the model for the final answer in the tablet's answer format and returns it.

Web searches from the tablet are passed through unchanged. Without any MCP servers the proxy is a plain pass-through.

## Configuration

Copy `config.example.json` to `config.json` (never commit it) and fill it in. Any string may be written as `"${NAME}"` to read it from the environment variable `NAME`, so keys can stay out of the file.

| Field | Default | Meaning |
|---|---|---|
| `listen` | `8787` | Port. |
| `client_key` | — | The key the tablet sends (`api_key` on the tablet). At least 24 characters, for example `openssl rand -hex 24`. |
| `upstream.base_url` | — | The real API, normally `https://openrouter.ai/api/v1`. |
| `upstream.api_key` | — | Your OpenRouter key. It stays on the proxy, never on the tablet. |
| `upstream.model` | — | Optional. Overrides the model the tablet asks for. |
| `max_tool_steps` | `5` | Tool rounds per question. |
| `pending_ttl_minutes` | `10` | How long a turn stays open for a follow-up tap (see below). |
| `mcp_servers[]` | `[]` | The MCP servers, see below. |

Each MCP server:

| Field | Default | Meaning |
|---|---|---|
| `name` | — | Short name: `a-z`, `0-9`, `_` and `-`, at most 32 characters, no `__`. |
| `url` | — | Streamable HTTP endpoint of the server. |
| `headers` | `{}` | Sent with every request, for example an API key. |
| `confirm` | `[]` | Tools that must never run without a written yes. |
| `confirm_destructive` | `true` | Also require a yes for every tool that does not declare itself read-only (`readOnlyHint`) or non-destructive (`destructiveHint: false`), which is the MCP default. Turn it off only for servers that ask for confirmation themselves. |

### Example: Composio

[Composio Connect](https://docs.composio.dev/docs/composio-connect) is one MCP server that reaches 1000+ apps (Gmail, Calendar, Notion, Slack, GitHub, …):

```json
{
  "name": "composio",
  "url": "https://connect.composio.dev/mcp",
  "headers": { "x-consumer-api-key": "${COMPOSIO_CONSUMER_KEY}" },
  "confirm": [],
  "confirm_destructive": true
}
```

Copy the consumer API key from the Composio dashboard. **Connect your apps in the Composio dashboard first.** When an app is not connected yet, Composio answers with an OAuth link, and a link cannot be clicked on the tablet.

### Any other MCP server

Add another entry with its `url` and whatever `headers` it needs for authentication, for example `{"Authorization": "Bearer ${MY_SERVICE_TOKEN}"}`. Servers that only support interactive OAuth logins cannot be used; the proxy needs a key it can send in a header.

## Confirmation: "write yes and tap again"

When the model wants to run a tool that needs a yes, the proxy does **not** run it. The model writes a summary of what it would do, which appears on the page. The proxy keeps the conversation for `pending_ttl_minutes` minutes.

Write **yes** under the summary and tap **Ask** again. The model sees the earlier turn and your yes, asks for the same tool with the same arguments, and only then does the proxy run it. Each yes runs the action at most once, even if the answer then fails and you tap again.

What the proxy enforces: the tool, its exact arguments, the time limit and the single use. Different arguments or a tap after the time limit never run the pending action. Whether what is on the page counts as a yes is decided by the model. Within the time limit, a tap with an old "yes" still on the page, or text returned by another tool, could be read as approval. Use `confirm` for actions you never want the model to judge, and turn the page before asking something new.

Every turn that used a tool is kept open the same way, so a server with its own confirmation step (it returns a code and waits for a second call) works too: the code stays inside the proxy and never appears on the page.

## Running it

Node.js 24 or later:

```bash
cd proxy
pnpm install --prod
OPENREMARK_CLIENT_KEY=... OPENROUTER_API_KEY=... COMPOSIO_CONSUMER_KEY=... node src/server.ts
```

Docker:

```bash
docker build -t openremark-proxy proxy
docker run -p 8787:8787 -v "$PWD/proxy/config.json:/config/config.json:ro" \
  -e OPENREMARK_CLIENT_KEY -e OPENROUTER_API_KEY -e COMPOSIO_CONSUMER_KEY openremark-proxy
```

On a host that sets environment variables but does not mount files, such as Coolify, put the whole config in the variable `CONFIG_JSON` instead of a file. `${NAME}` references in it are still read from the other environment variables, so the keys can stay in variables of their own.

`GET /health` answers `{"ok":true}`.

Then, on the tablet, give the wrench button the proxy's address and its `client_key`. Each command reads its value from stdin, so the key stays out of the shell history:

```sh
cd /home/root/.config/ink-diary
echo 'https://proxy.example.com/v1' | ./config-model --set tools_base_url
read -rs KEY && echo "$KEY" | ./config-model --set tools_api_key; unset KEY
```

Use `http://<computer-on-your-network>:8787/v1` for a proxy on your own network; the tablet only allows plain `http` to local network addresses.

## Notes

- Each question costs two model requests when MCP servers are configured (one with tools, one for the formatted answer). OpenRouter's free models allow 50 requests a day, or 1000 a day after buying at least 10 USD of credits in total.
- Logs contain the time, status, duration and the names of the tools used. Never keys, page content or tool results.
- One proxy serves one tablet (one `client_key`).

## Tests

```bash
pnpm test && pnpm typecheck
```
