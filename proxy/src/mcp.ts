import { Client } from '@modelcontextprotocol/sdk/client/index.js'
import { StreamableHTTPClientTransport } from '@modelcontextprotocol/sdk/client/streamableHttp.js'
import type { McpServerConfig } from './config.ts'

export type ToolEntry = {
  fqName: string
  server: string
  tool: string
  description: string
  inputSchema: object
  needsConfirm: boolean
}

const CACHE_MS = 300_000
// A server's own instructions tell the model what the service is for; a runaway text must not fill the prompt.
const INSTRUCTIONS_MAX = 4000
const TIMEOUT_MS = 10_000

// One short-lived client per server and request: nothing is shared between calls.
async function withClient<T>(server: McpServerConfig, use: (client: Client) => Promise<T>): Promise<T> {
  const client = new Client({ name: 'openremark-proxy', version: '0.1.0' })
  const transport = new StreamableHTTPClientTransport(new URL(server.url), { requestInit: { headers: server.headers } })
  try {
    await client.connect(transport, { timeout: TIMEOUT_MS })
    return await use(client)
  } finally {
    await client.close().catch(() => {})
  }
}

export class McpRegistry {
  #servers: McpServerConfig[]
  #now: () => number
  #cache = new Map<string, { at: number; tools: ToolEntry[]; instructions: string }>()

  constructor(servers: McpServerConfig[], now: () => number = Date.now) {
    this.#servers = servers
    this.#now = now
  }

  async listTools(): Promise<{ tools: ToolEntry[]; unavailable: string[]; instructions: Record<string, string> }> {
    const results = await Promise.all(this.#servers.map(async (server) => {
      try { return { server, listed: await this.#toolsOf(server) } } catch { return { server, listed: null } }
    }))
    return {
      tools: results.flatMap((r) => r.listed?.tools ?? []),
      unavailable: results.filter((r) => r.listed === null).map((r) => r.server.name),
      instructions: Object.fromEntries(results.filter((r) => r.listed?.instructions).map((r) => [r.server.name, r.listed!.instructions])),
    }
  }

  async #toolsOf(server: McpServerConfig): Promise<{ tools: ToolEntry[]; instructions: string }> {
    const cached = this.#cache.get(server.name)
    if (cached && this.#now() - cached.at < CACHE_MS) return cached
    const { listed, about } = await withClient(server, async (client) => ({
      listed: await client.listTools(undefined, { timeout: TIMEOUT_MS }),
      about: client.getInstructions() ?? '',
    }))
    const tools = listed.tools.map((tool): ToolEntry => ({
      fqName: `${server.name}__${tool.name}`,
      server: server.name,
      tool: tool.name,
      description: tool.description ?? '',
      inputSchema: tool.inputSchema,
      // MCP defaults: a tool that is not read-only counts as destructive unless it says otherwise.
      needsConfirm: server.confirm.includes(tool.name) ||
        (server.confirm_destructive && tool.annotations?.readOnlyHint !== true && tool.annotations?.destructiveHint !== false),
    }))
    const entry = { at: this.#now(), tools, instructions: about.trim().slice(0, INSTRUCTIONS_MAX) }
    this.#cache.set(server.name, entry)
    return entry
  }

  async call(fqName: string, args: object): Promise<string> {
    const split = fqName.indexOf('__')
    const server = split > 0 ? this.#servers.find((s) => s.name === fqName.slice(0, split)) : undefined
    if (!server) throw new Error(`Unknown tool ${fqName}`)
    let result: Awaited<ReturnType<Client['callTool']>>
    try {
      result = await withClient(server, (client) =>
        client.callTool({ name: fqName.slice(split + 2), arguments: args as Record<string, unknown> }, undefined, { timeout: 60_000 }))
    } catch (error) {
      // A connection failure or timeout is a tool result for the model, not a failed request.
      return `Tool error: ${(error as Error).message}`
    }
    const content = Array.isArray(result.content) ? result.content : []
    const text = content.map((c: { type: string; text?: string }) => (c.type === 'text' ? c.text ?? '' : `[${c.type}]`)).join('\n')
    return result.isError ? `Tool error: ${text}` : text
  }
}
