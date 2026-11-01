import { createServer, type IncomingMessage } from 'node:http'
import type { AddressInfo } from 'node:net'
import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js'
import { StreamableHTTPServerTransport } from '@modelcontextprotocol/sdk/server/streamableHttp.js'
import { z } from 'zod'

export type FakeTool = { name: string; destructive?: boolean; readOnly?: boolean; handler: (args: Record<string, unknown>) => string }
export type FakeMcp = { url: string; close(): Promise<void>; calls: { name: string; args: Record<string, unknown> }[]; listCount(): number; headers: IncomingMessage['headers'][] }

async function readJson(req: IncomingMessage): Promise<unknown> {
  const chunks: Buffer[] = []
  for await (const chunk of req) chunks.push(chunk as Buffer)
  return chunks.length ? JSON.parse(Buffer.concat(chunks).toString('utf8')) : undefined
}

// A stateless MCP server on a random port, built the same way as a real one.
export async function startFakeMcp(tools: FakeTool[], instructions?: string): Promise<FakeMcp> {
  const calls: FakeMcp['calls'] = []
  const headers: FakeMcp['headers'] = []
  let lists = 0
  const http = createServer(async (req, res) => {
    headers.push(req.headers)
    const body = await readJson(req)
    const messages = Array.isArray(body) ? body : [body]
    if (messages.some((m) => (m as { method?: string })?.method === 'tools/list')) lists++
    const server = new McpServer({ name: 'fake', version: '1.0.0' }, instructions ? { instructions } : {})
    for (const tool of tools) {
      server.registerTool(tool.name, {
        description: `Fake ${tool.name}`,
        inputSchema: z.looseObject({}),
        annotations: tool.readOnly ? { readOnlyHint: true } : tool.destructive ? { destructiveHint: true } : {},
      }, async (args: Record<string, unknown>) => {
        calls.push({ name: tool.name, args })
        try {
          return { content: [{ type: 'text' as const, text: tool.handler(args) }] }
        } catch (error) {
          return { isError: true, content: [{ type: 'text' as const, text: (error as Error).message }] }
        }
      })
    }
    const transport = new StreamableHTTPServerTransport({ sessionIdGenerator: undefined, enableJsonResponse: true })
    res.on('close', () => { void transport.close(); void server.close() })
    await server.connect(transport)
    await transport.handleRequest(req, res, body)
  })
  await new Promise<void>((resolve) => http.listen(0, '127.0.0.1', resolve))
  const { port } = http.address() as AddressInfo
  return {
    url: `http://127.0.0.1:${port}/mcp`,
    calls,
    headers,
    listCount: () => lists,
    close: () => new Promise<void>((resolve) => { http.closeAllConnections(); http.close(() => resolve()) }),
  }
}
