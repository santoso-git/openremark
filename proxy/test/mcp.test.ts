import { afterEach, expect, test } from 'vitest'
import { McpRegistry } from '../src/mcp.ts'
import type { McpServerConfig } from '../src/config.ts'
import { startFakeMcp, type FakeMcp } from './fake-mcp.ts'

const open: FakeMcp[] = []
afterEach(async () => { await Promise.all(open.splice(0).map((f) => f.close())) })

async function fake(...args: Parameters<typeof startFakeMcp>) { const f = await startFakeMcp(...args); open.push(f); return f }
const server = (name: string, url: string, extra: Partial<McpServerConfig> = {}): McpServerConfig =>
  ({ name, url, headers: {}, confirm: [], confirm_destructive: true, ...extra })

test('lists tools with server-prefixed names and confirmation flags', async () => {
  const notes = await fake([{ name: 'status', readOnly: true, handler: () => 'ok' }, { name: 'delete_note', destructive: true, handler: () => 'gone' }])
  const { tools, unavailable } = await new McpRegistry([server('notes', notes.url)]).listTools()
  expect(unavailable).toEqual([])
  expect(tools.map((t) => [t.fqName, t.needsConfirm])).toEqual([['notes__status', false], ['notes__delete_note', true]])
  expect(tools[0]).toMatchObject({ server: 'notes', tool: 'status', description: 'Fake status' })
  expect(tools[0].inputSchema).toMatchObject({ type: 'object' })
})

test('confirm list and confirm_destructive decide what needs a yes', async () => {
  const notes = await fake([{ name: 'status', readOnly: true, handler: () => 'ok' }, { name: 'delete_note', destructive: true, handler: () => 'gone' }])
  const { tools } = await new McpRegistry([server('notes', notes.url, { confirm: ['status'], confirm_destructive: false })]).listTools()
  expect(tools.map((t) => [t.tool, t.needsConfirm])).toEqual([['status', true], ['delete_note', false]])
})

test('sends configured headers', async () => {
  const notes = await fake([{ name: 'status', handler: () => 'ok' }])
  await new McpRegistry([server('notes', notes.url, { headers: { 'x-consumer-api-key': 'ck-123' } })]).listTools()
  expect(notes.headers.some((h) => h['x-consumer-api-key'] === 'ck-123')).toBe(true)
})

test('a server that does not answer is skipped, the others still work', async () => {
  const notes = await fake([{ name: 'status', handler: () => 'ok' }])
  const registry = new McpRegistry([server('dead', 'http://127.0.0.1:9/mcp'), server('notes', notes.url)])
  const { tools, unavailable } = await registry.listTools()
  expect(unavailable).toEqual(['dead'])
  expect(tools.map((t) => t.fqName)).toEqual(['notes__status'])
})

test('tool lists are cached for five minutes per server', async () => {
  const notes = await fake([{ name: 'status', handler: () => 'ok' }])
  let now = 1_000
  const registry = new McpRegistry([server('notes', notes.url)], () => now)
  await registry.listTools()
  now += 299_999
  await registry.listTools()
  expect(notes.listCount()).toBe(1)
  now += 2
  await registry.listTools()
  expect(notes.listCount()).toBe(2)
})

test('call returns the tool text, and tool errors come back as text', async () => {
  const notes = await fake([
    { name: 'status', handler: (args) => `status ${JSON.stringify(args)}` },
    { name: 'broken', handler: () => { throw new Error('boom') } },
  ])
  const registry = new McpRegistry([server('notes', notes.url)])
  await registry.listTools()
  expect(await registry.call('notes__status', { a: 1 })).toBe('status {"a":1}')
  expect(await registry.call('notes__broken', {})).toBe('Tool error: boom')
  expect(notes.calls.map((c) => c.name)).toEqual(['status', 'broken'])
})

test('calling an unknown tool throws', async () => {
  const registry = new McpRegistry([])
  await expect(registry.call('nope__x', {})).rejects.toThrow(/unknown tool/i)
})

test('tools without annotations need a yes, read-only and non-destructive ones do not', async () => {
  // MCP: destructiveHint defaults to true for a tool that is not readOnlyHint.
  const notes = await fake([{ name: 'send', handler: () => 'sent' }, { name: 'status', readOnly: true, handler: () => 'ok' }])
  const { tools } = await new McpRegistry([server('notes', notes.url)]).listTools()
  expect(tools.map((t) => [t.tool, t.needsConfirm])).toEqual([['send', true], ['status', false]])
})

test('a server that fails during a call gives a tool error instead of throwing', async () => {
  const notes = await fake([{ name: 'status', readOnly: true, handler: () => 'ok' }])
  const registry = new McpRegistry([server('notes', notes.url)])
  await registry.listTools()
  await notes.close()
  expect(await registry.call('notes__status', {})).toMatch(/^Tool error: /)
})
