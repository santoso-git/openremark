import type { ProxyConfig } from './config.ts'

export type ToolCall = { id: string; type: 'function'; function: { name: string; arguments: string } }
export type ChatMessage = {
  role: 'system' | 'user' | 'assistant' | 'tool'
  content: unknown
  tool_calls?: ToolCall[]
  tool_call_id?: string
}
export type ToolSpec = { type: 'function'; function: { name: string; description: string; parameters: object } }
export type ChatRequest = {
  model?: string
  messages: ChatMessage[]
  max_tokens?: number
  response_format?: object
  plugins?: object[]
  tools?: ToolSpec[]
  [key: string]: unknown
}
export type ChatResponse = {
  choices: { finish_reason: string | null; message: { role: string; content: string | null; tool_calls?: ToolCall[]; [key: string]: unknown } }[]
  [key: string]: unknown
}
export type Upstream = (req: ChatRequest) => Promise<ChatResponse>

export class UpstreamError extends Error {
  status: number
  body: string
  constructor(status: number, body: string) {
    super(`Upstream answered HTTP ${status}`)
    this.status = status
    this.body = body
  }
}

export function httpUpstream(cfg: ProxyConfig['upstream']): Upstream {
  const url = `${cfg.base_url.replace(/\/+$/, '')}/chat/completions`
  return async (req) => {
    const body = cfg.model ? { ...req, model: cfg.model } : req
    const res = await fetch(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json', Authorization: `Bearer ${cfg.api_key}`,
        'HTTP-Referer': 'https://github.com/santoso-git/openremark', 'X-Title': 'openremark' },
      body: JSON.stringify(body),
      signal: AbortSignal.timeout(170_000),
    })
    const text = await res.text()
    if (!res.ok) throw new UpstreamError(res.status, text)
    return JSON.parse(text) as ChatResponse
  }
}
