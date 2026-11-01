import type { AddressInfo } from 'node:net'
import type { Server } from 'node:http'
import { afterEach, expect, test } from 'vitest'
import { parseConfig } from '../src/config.ts'
import { createServer } from '../src/server.ts'
import { UpstreamError, type ChatRequest } from '../src/upstream.ts'

const KEY = 'client-key-0123456789abcdef'
const config = parseConfig({ client_key: KEY, upstream: { base_url: 'https://openrouter.ai/api/v1', api_key: 'sk-or-secret' }, mcp_servers: [] }, {})
const servers: Server[] = []
afterEach(async () => { await Promise.all(servers.splice(0).map((s) => new Promise((r) => s.close(r)))) })

async function start(upstream = async (_: ChatRequest) => ({ choices: [{ finish_reason: 'stop', message: { role: 'assistant', content: 'Hi' } }] })) {
  const lines: string[] = []
  const server = createServer(config, { upstream, log: (line) => lines.push(line) })
  await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve))
  servers.push(server)
  return { base: `http://127.0.0.1:${(server.address() as AddressInfo).port}`, lines }
}
const chat = (base: string, body: unknown, key: string | null = KEY) => fetch(`${base}/v1/chat/completions`, {
  method: 'POST', headers: { 'Content-Type': 'application/json', ...(key ? { Authorization: `Bearer ${key}` } : {}) }, body: JSON.stringify(body),
})
const request = { model: 'm', messages: [{ role: 'system', content: 'x' }, { role: 'user', content: 'a secret question' }] }

test('rejects a missing or wrong client key', async () => {
  const { base } = await start()
  for (const key of [null, 'wrong', `${KEY}x`]) {
    const res = await chat(base, request, key)
    expect(res.status).toBe(401)
    expect(await res.json()).toEqual({ error: { message: 'Invalid client key' } })
  }
})

test('answers with the upstream response', async () => {
  const { base } = await start()
  const res = await chat(base, request)
  expect(res.status).toBe(200)
  expect((await res.json()).choices[0].message.content).toBe('Hi')
})

test('rejects bodies over 8 MiB', async () => {
  const { base } = await start()
  const res = await chat(base, { ...request, padding: 'x'.repeat(9 * 1024 * 1024) })
  expect(res.status).toBe(413)
})

test('rejects invalid JSON with 400', async () => {
  const { base } = await start()
  const res = await fetch(`${base}/v1/chat/completions`, { method: 'POST', headers: { Authorization: `Bearer ${KEY}` }, body: '{nope' })
  expect(res.status).toBe(400)
})

test('passes upstream errors on with their status', async () => {
  const { base } = await start(async () => { throw new UpstreamError(429, '{"error":{"message":"Rate limit exceeded: free-models-per-day"}}') })
  const res = await chat(base, request)
  expect(res.status).toBe(429)
  expect((await res.json()).error.message).toContain('free-models-per-day')
})

test('other failures become 502', async () => {
  const { base } = await start(async () => { throw new Error('socket hang up') })
  expect((await chat(base, request)).status).toBe(502)
})

test('health and unknown routes', async () => {
  const { base } = await start()
  const health = await fetch(`${base}/health`)
  expect(health.status).toBe(200)
  expect(await health.json()).toEqual({ ok: true })
  expect((await fetch(`${base}/v1/models`)).status).toBe(404)
})

test('logs never contain keys or page content', async () => {
  const { base, lines } = await start()
  await chat(base, request)
  await chat(base, request, 'wrong')
  expect(lines.length).toBeGreaterThanOrEqual(2)
  for (const line of lines) {
    expect(line).not.toContain(KEY)
    expect(line).not.toContain('sk-or-secret')
    expect(line).not.toContain('a secret question')
    expect(line).toMatch(/^\S+ \d{3} \d+ms tools=/)
  }
})

test('a turn that runs past its deadline answers 504', async () => {
  const lines: string[] = []
  const slow = async () => { await new Promise((r) => setTimeout(r, 50)); return { choices: [{ finish_reason: 'stop', message: { role: 'assistant', content: 'late' } }] } }
  const withTools = parseConfig({ client_key: KEY, upstream: { base_url: 'https://openrouter.ai/api/v1', api_key: 'x' }, mcp_servers: [] }, {})
  const server = createServer(withTools, { upstream: slow, log: (l) => lines.push(l), deadlineMs: 10 })
  await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve))
  servers.push(server)
  const res = await chat(`http://127.0.0.1:${(server.address() as AddressInfo).port}`, request)
  expect(res.status).toBe(504)
})
