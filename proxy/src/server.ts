import { timingSafeEqual } from 'node:crypto'
import { createServer as createHttpServer, type IncomingMessage, type Server, type ServerResponse } from 'node:http'
import { loadConfig, type ProxyConfig } from './config.ts'
import { runCompletion, TurnTimeoutError } from './loop.ts'
import { McpRegistry } from './mcp.ts'
import { PendingStore } from './pending.ts'
import { httpUpstream, UpstreamError, type ChatRequest, type Upstream } from './upstream.ts'

const MAX_BODY = 8 * 1024 * 1024

class HttpError extends Error {
  status: number
  constructor(status: number, message: string) { super(message); this.status = status }
}

function keyMatches(header: string | undefined, key: string): boolean {
  const given = Buffer.from(header?.startsWith('Bearer ') ? header.slice(7) : '')
  const expected = Buffer.from(key)
  return given.length === expected.length && timingSafeEqual(given, expected)
}

async function readBody(req: IncomingMessage): Promise<ChatRequest> {
  const chunks: Buffer[] = []
  let size = 0
  for await (const chunk of req) {
    size += (chunk as Buffer).length
    if (size > MAX_BODY) throw new HttpError(413, 'Request too large')
    chunks.push(chunk as Buffer)
  }
  try { return JSON.parse(Buffer.concat(chunks).toString('utf8')) as ChatRequest } catch { throw new HttpError(400, 'Invalid JSON') }
}

function send(res: ServerResponse, status: number, body: unknown): void {
  res.writeHead(status, { 'Content-Type': 'application/json' })
  res.end(JSON.stringify(body))
}

// Upstream errors keep their status and OpenAI-style body, so the tablet shows the same messages as without the proxy.
function upstreamMessage(error: UpstreamError): string {
  try { return (JSON.parse(error.body) as { error?: { message?: string } }).error?.message ?? error.message } catch { return error.message }
}

export function createServer(cfg: ProxyConfig, deps: { upstream?: Upstream; log?: (line: string) => void; deadlineMs?: number } = {}): Server {
  const deadlineMs = deps.deadlineMs ?? 160_000
  const upstream = deps.upstream ?? httpUpstream(cfg.upstream)
  const log = deps.log ?? ((line: string) => console.log(line))
  const registry = new McpRegistry(cfg.mcp_servers)
  const pending = new PendingStore(cfg.pending_ttl_minutes * 60_000)

  return createHttpServer(async (req, res) => {
    const started = Date.now()
    const tools: string[] = []
    let status = 200
    try {
      if (req.method === 'GET' && req.url === '/health') return send(res, 200, { ok: true })
      if (req.method !== 'POST' || req.url !== '/v1/chat/completions') throw new HttpError(404, 'Not found')
      if (!keyMatches(req.headers.authorization, cfg.client_key)) throw new HttpError(401, 'Invalid client key')
      const body = await readBody(req)
      const gone = new AbortController()
      res.on('close', () => { if (!res.writableFinished) gone.abort() })
      let timer: NodeJS.Timeout | undefined
      const timeout = new Promise<never>((_, reject) => { timer = setTimeout(() => { gone.abort(); reject(new TurnTimeoutError()) }, deadlineMs) })
      try {
        const answer = await Promise.race([runCompletion(body, { clientKey: cfg.client_key, registry, upstream, pending,
          maxSteps: cfg.max_tool_steps, onToolCall: (name) => tools.push(name), deadlineMs, signal: gone.signal }), timeout])
        send(res, 200, answer)
      } finally {
        clearTimeout(timer)
      }
    } catch (error) {
      if (error instanceof HttpError) status = error.status
      else if (error instanceof TurnTimeoutError) status = 504
      else if (error instanceof UpstreamError) status = error.status
      else status = 502
      const message = error instanceof UpstreamError ? upstreamMessage(error)
        : error instanceof HttpError || error instanceof TurnTimeoutError ? error.message : 'Proxy error'
      if (status === 413) req.resume()
      send(res, status, { error: { message } })
    } finally {
      // Never log keys, page content or tool results.
      if (req.url !== '/health') log(`${new Date().toISOString()} ${status} ${Date.now() - started}ms tools=${tools.join(',')}`)
    }
  })
}

if (import.meta.main) {
  const cfg = loadConfig()
  createServer(cfg).listen(cfg.listen, () => console.log(`openremark proxy listening on ${cfg.listen}`))
}
