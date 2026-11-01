import { afterEach, expect, test } from 'vitest'
import { runCompletion } from '../src/loop.ts'
import { McpRegistry } from '../src/mcp.ts'
import { PendingStore } from '../src/pending.ts'
import type { ChatRequest, ChatResponse } from '../src/upstream.ts'
import { startFakeMcp, type FakeMcp } from './fake-mcp.ts'

const open: FakeMcp[] = []
afterEach(async () => { await Promise.all(open.splice(0).map((f) => f.close())) })

const FORMAT = { type: 'json_schema', json_schema: { name: 'diary_answer', strict: true, schema: { type: 'object' } } }
const page = (text: string): ChatRequest => ({
  model: 'google/gemma-4-31b-it:free', max_tokens: 4000, response_format: FORMAT,
  messages: [{ role: 'system', content: 'Read the page.' }, { role: 'user', content: [{ type: 'text', text }] }],
})
const say = (content: string): ChatResponse => ({ choices: [{ finish_reason: 'stop', message: { role: 'assistant', content } }] })
const callTool = (name: string, args: object, id = 'c1'): ChatResponse => ({
  choices: [{ finish_reason: 'tool_calls', message: { role: 'assistant', content: null,
    tool_calls: [{ id, type: 'function', function: { name, arguments: JSON.stringify(args) } }] } }],
})
function scripted(...replies: ChatResponse[]) {
  const requests: ChatRequest[] = []
  const upstream = async (req: ChatRequest) => {
    requests.push(structuredClone(req))
    const next = replies.shift()
    if (!next) throw new Error('upstream called more times than scripted')
    return next
  }
  return { upstream, requests }
}

async function setup(now = { t: 0 }) {
  const notes = await startFakeMcp([{ name: 'status', readOnly: true, handler: () => '3 posts scheduled' },
    { name: 'delete_note', destructive: true, handler: (a) => `deleted ${a.id}` }])
  open.push(notes)
  const registry = new McpRegistry([{ name: 'notes', url: notes.url, headers: {}, confirm: [], confirm_destructive: true }], () => now.t)
  const pending = new PendingStore(600_000, () => now.t)
  return { notes, registry, pending, now }
}
const systemOf = (req: ChatRequest) => String(req.messages[0].content)

test('answers without tools: one tool-enabled request, then the formatted one', async () => {
  const { registry, pending } = await setup()
  const { upstream, requests } = scripted(say('no tools needed'), say('{"kind":"text","text":"Hi"}'))
  const res = await runCompletion(page('hello'), { clientKey: 'k', registry, upstream, pending, maxSteps: 5 })
  expect(res.choices[0].message.content).toBe('{"kind":"text","text":"Hi"}')
  expect(requests).toHaveLength(2)
  expect(requests[0].tools?.map((t) => t.function.name)).toEqual(['notes__status', 'notes__delete_note'])
  expect(requests[0].response_format).toBeUndefined()
  expect(requests[1].response_format).toEqual(FORMAT)
  expect(requests[1].tools).toBeUndefined()
  expect(systemOf(requests[0])).toContain('You can use tools from these services: notes.')
  expect(pending.get('k')).toBeUndefined()
})

test('runs a tool, then formats with its result', async () => {
  const { notes, registry, pending } = await setup()
  const { upstream, requests } = scripted(callTool('notes__status', {}), say('done'), say('{"kind":"text","text":"3 posts"}'))
  await runCompletion(page('status of my notes?'), { clientKey: 'k', registry, upstream, pending, maxSteps: 5 })
  expect(notes.calls.map((c) => c.name)).toEqual(['status'])
  const last = requests.at(-1)!
  expect(last.tools).toBeUndefined()
  expect(last.response_format).toEqual(FORMAT)
  expect(last.messages.some((m) => m.role === 'tool' && m.content === '3 posts scheduled' && m.tool_call_id === 'c1')).toBe(true)
  expect(pending.get('k')?.blocked).toBeUndefined()
})

test('caps tool steps at five', async () => {
  const { notes, registry, pending } = await setup()
  const loop = Array.from({ length: 5 }, (_, i) => callTool('notes__status', {}, `c${i}`))
  const { upstream, requests } = scripted(...loop, say('{"kind":"text","text":"stopped"}'))
  await runCompletion(page('loop'), { clientKey: 'k', registry, upstream, pending, maxSteps: 5 })
  expect(notes.calls).toHaveLength(5)
  expect(requests).toHaveLength(6)
  expect(requests[5].response_format).toEqual(FORMAT)
})

test('a tool that needs a yes is not run on the first tap', async () => {
  const { notes, registry, pending } = await setup()
  const { upstream, requests } = scripted(callTool('notes__delete_note', { id: 7 }), say('summary'), say('{"kind":"text","text":"Delete note 7? Write yes and tap again."}'))
  await runCompletion(page('delete note 7'), { clientKey: 'k', registry, upstream, pending, maxSteps: 5 })
  expect(notes.calls).toEqual([])
  expect(pending.get('k')?.blocked).toEqual({ fqName: 'notes__delete_note', args: '{"id":7}' })
  expect(requests[1].messages.some((m) => m.role === 'tool' && String(m.content).startsWith('Not executed: this action needs the user'))).toBe(true)
})

test('after a yes the same call runs', async () => {
  const { notes, registry, pending } = await setup()
  const first = scripted(callTool('notes__delete_note', { id: 7 }), say('summary'), say('{"kind":"text","text":"Delete 7?"}'))
  await runCompletion(page('delete note 7'), { clientKey: 'k', registry, upstream: first.upstream, pending, maxSteps: 5 })
  const second = scripted(callTool('notes__delete_note', { id: 7 }, 'c2'), say('done'), say('{"kind":"text","text":"Deleted."}'))
  await runCompletion(page('ja'), { clientKey: 'k', registry, upstream: second.upstream, pending, maxSteps: 5 })
  expect(notes.calls).toEqual([{ name: 'delete_note', args: { id: 7 } }])
  expect(systemOf(second.requests[0])).toContain('approves the pending action notes__delete_note({"id":7})')
  // The earlier turn travels with the new page.
  expect(second.requests[0].messages.some((m) => m.role === 'user' && JSON.stringify(m.content).includes('delete note 7'))).toBe(true)
  expect(pending.get('k')?.blocked).toBeUndefined()
})

test('different arguments after a yes are blocked again', async () => {
  const { notes, registry, pending } = await setup()
  const first = scripted(callTool('notes__delete_note', { id: 7 }), say('s'), say('{"kind":"text","text":"Delete 7?"}'))
  await runCompletion(page('delete note 7'), { clientKey: 'k', registry, upstream: first.upstream, pending, maxSteps: 5 })
  const second = scripted(callTool('notes__delete_note', { id: 8 }, 'c2'), say('s'), say('{"kind":"text","text":"Delete 8?"}'))
  await runCompletion(page('ja'), { clientKey: 'k', registry, upstream: second.upstream, pending, maxSteps: 5 })
  expect(notes.calls).toEqual([])
  expect(pending.get('k')?.blocked).toEqual({ fqName: 'notes__delete_note', args: '{"id":8}' })
})

test('an expired pending turn never runs the blocked tool', async () => {
  const now = { t: 0 }
  const { notes, registry, pending } = await setup(now)
  const first = scripted(callTool('notes__delete_note', { id: 7 }), say('s'), say('{"kind":"text","text":"Delete 7?"}'))
  await runCompletion(page('delete note 7'), { clientKey: 'k', registry, upstream: first.upstream, pending, maxSteps: 5 })
  now.t += 600_001
  const second = scripted(callTool('notes__delete_note', { id: 7 }, 'c2'), say('s'), say('{"kind":"text","text":"Delete 7?"}'))
  await runCompletion(page('ja'), { clientKey: 'k', registry, upstream: second.upstream, pending, maxSteps: 5 })
  expect(notes.calls).toEqual([])
  expect(systemOf(second.requests[0])).not.toContain('A previous turn is still open')
})

test('a new topic without tools clears the pending turn', async () => {
  const { registry, pending } = await setup()
  const first = scripted(callTool('notes__delete_note', { id: 7 }), say('s'), say('{"kind":"text","text":"Delete 7?"}'))
  await runCompletion(page('delete note 7'), { clientKey: 'k', registry, upstream: first.upstream, pending, maxSteps: 5 })
  const second = scripted(say('no tools'), say('{"kind":"text","text":"Paris"}'))
  await runCompletion(page('capital of France?'), { clientKey: 'k', registry, upstream: second.upstream, pending, maxSteps: 5 })
  expect(pending.get('k')).toBeUndefined()
})

test('pending turns are kept per client key', async () => {
  const { registry, pending } = await setup()
  const first = scripted(callTool('notes__delete_note', { id: 7 }), say('s'), say('{"kind":"text","text":"Delete 7?"}'))
  await runCompletion(page('delete note 7'), { clientKey: 'a', registry, upstream: first.upstream, pending, maxSteps: 5 })
  expect(pending.get('b')).toBeUndefined()
})

test('web search requests pass through unchanged', async () => {
  const { registry, pending } = await setup()
  const { upstream, requests } = scripted(say('Sunny'))
  const search: ChatRequest = { model: 'm', max_tokens: 2000, plugins: [{ id: 'web', max_results: 3 }],
    messages: [{ role: 'system', content: 'search' }, { role: 'user', content: 'weather' }] }
  const res = await runCompletion(search, { clientKey: 'k', registry, upstream, pending, maxSteps: 5 })
  expect(res.choices[0].message.content).toBe('Sunny')
  expect(requests).toEqual([search])
})

test('without MCP servers the request passes through in one call', async () => {
  const { upstream, requests } = scripted(say('{"kind":"text","text":"Hi"}'))
  await runCompletion(page('hello'), { clientKey: 'k', registry: new McpRegistry([]), upstream, pending: new PendingStore(1000), maxSteps: 5 })
  expect(requests).toEqual([page('hello')])
})

test('an unavailable server is mentioned to the model', async () => {
  const { notes } = await setup()
  const registry = new McpRegistry([{ name: 'notes', url: notes.url, headers: {}, confirm: [], confirm_destructive: true },
    { name: 'dead', url: 'http://127.0.0.1:9/mcp', headers: {}, confirm: [], confirm_destructive: true }])
  const { upstream, requests } = scripted(say('x'), say('{"kind":"text","text":"x"}'))
  await runCompletion(page('hello'), { clientKey: 'k', registry, upstream, pending: new PendingStore(1000), maxSteps: 5 })
  expect(systemOf(requests[0])).toContain('Unavailable right now: dead.')
})

test('an approval runs at most once, even when the turn then fails', async () => {
  const { notes, registry, pending } = await setup()
  const first = scripted(callTool('notes__delete_note', { id: 7 }), say('s'), say('{"kind":"text","text":"Delete 7?"}'))
  await runCompletion(page('delete note 7'), { clientKey: 'k', registry, upstream: first.upstream, pending, maxSteps: 5 })
  // Yes: the tool runs, then the final request fails (for example the daily limit).
  const second = scripted(callTool('notes__delete_note', { id: 7 }, 'c2'), say('done'))
  await expect(runCompletion(page('ja'), { clientKey: 'k', registry, upstream: second.upstream, pending, maxSteps: 5 })).rejects.toThrow()
  expect(notes.calls).toHaveLength(1)
  // The user taps again with the same yes on the page: the deletion must not run a second time.
  const third = scripted(callTool('notes__delete_note', { id: 7 }, 'c3'), say('s'), say('{"kind":"text","text":"Delete 7?"}'))
  await runCompletion(page('ja'), { clientKey: 'k', registry, upstream: third.upstream, pending, maxSteps: 5 })
  expect(notes.calls).toHaveLength(1)
})

test('an approval is not reused within the same turn', async () => {
  const { notes, registry, pending } = await setup()
  const first = scripted(callTool('notes__delete_note', { id: 7 }), say('s'), say('{"kind":"text","text":"Delete 7?"}'))
  await runCompletion(page('delete note 7'), { clientKey: 'k', registry, upstream: first.upstream, pending, maxSteps: 5 })
  const second = scripted(callTool('notes__delete_note', { id: 7 }, 'c2'), callTool('notes__delete_note', { id: 7 }, 'c3'), say('s'), say('{"kind":"text","text":"Deleted."}'))
  await runCompletion(page('ja'), { clientKey: 'k', registry, upstream: second.upstream, pending, maxSteps: 5 })
  expect(notes.calls).toHaveLength(1)
})

test('with every server unavailable the request passes through without tools', async () => {
  const registry = new McpRegistry([{ name: 'dead', url: 'http://127.0.0.1:9/mcp', headers: {}, confirm: [], confirm_destructive: true }])
  const { upstream, requests } = scripted(say('{"kind":"text","text":"Hi"}'))
  await runCompletion(page('hello'), { clientKey: 'k', registry, upstream, pending: new PendingStore(1000), maxSteps: 5 })
  expect(requests).toEqual([page('hello')])
})

test('no tool runs after the turn deadline has passed', async () => {
  const { notes, registry, pending } = await setup()
  let clock = 0
  const slow = scripted(callTool('notes__status', {}), say('s'), say('{"kind":"text","text":"x"}'))
  const upstream = async (req: ChatRequest) => { clock += 200_000; return slow.upstream(req) }
  await expect(runCompletion(page('läget?'), { clientKey: 'k', registry, upstream, pending, maxSteps: 5, now: () => clock, deadlineMs: 150_000 }))
    .rejects.toThrow(/took too long/i)
  expect(notes.calls).toEqual([])
})

test('no tool runs after the client has gone', async () => {
  const { notes, registry, pending } = await setup()
  const controller = new AbortController()
  const slow = scripted(callTool('notes__status', {}), say('s'), say('{"kind":"text","text":"x"}'))
  const upstream = async (req: ChatRequest) => { controller.abort(); return slow.upstream(req) }
  await expect(runCompletion(page('läget?'), { clientKey: 'k', registry, upstream, pending, maxSteps: 5, signal: controller.signal })).rejects.toThrow()
  expect(notes.calls).toEqual([])
})

test("the server's own instructions reach the model, so it knows what belongs to that service", async () => {
  const notes = await startFakeMcp([{ name: 'create_note', handler: () => 'draft 12 created' }],
    "Notes is the user's note-taking service for notes and lists. Answer in Swedish.")
  open.push(notes)
  const registry = new McpRegistry([{ name: 'notes', url: notes.url, headers: {}, confirm: [], confirm_destructive: false }])
  const { upstream, requests } = scripted(say('x'), say('{"kind":"text","text":"x"}'))
  await runCompletion(page('Create a note about the trip'), { clientKey: 'k', registry, upstream, pending: new PendingStore(1000), maxSteps: 5 })
  expect(systemOf(requests[0])).toContain("About notes: Notes is the user's note-taking service for notes and lists. Answer in Swedish.")
  expect(systemOf(requests[1])).toContain("About notes: Notes is the user's note-taking service")
})

test('the model is told to use the tools for their services and never to invent what they did not return', async () => {
  const { registry, pending } = await setup()
  const { upstream, requests } = scripted(say('x'), say('{"kind":"text","text":"x"}'))
  await runCompletion(page('hello'), { clientKey: 'k', registry, upstream, pending, maxSteps: 5 })
  const system = systemOf(requests[0])
  expect(system).toContain('Anything that belongs to one of these services must be done or looked up with its tools')
  expect(system).toContain('Never invent codes, prices, offers, numbers or other facts')
})

test('very long server instructions are cut short', async () => {
  const notes = await startFakeMcp([{ name: 'status', readOnly: true, handler: () => 'ok' }], 'x'.repeat(10_000))
  open.push(notes)
  const registry = new McpRegistry([{ name: 'notes', url: notes.url, headers: {}, confirm: [], confirm_destructive: true }])
  const { upstream, requests } = scripted(say('x'), say('{"kind":"text","text":"x"}'))
  await runCompletion(page('hello'), { clientKey: 'k', registry, upstream, pending: new PendingStore(1000), maxSteps: 5 })
  expect(systemOf(requests[0])).not.toContain('x'.repeat(4001))
  expect(systemOf(requests[0])).toContain('x'.repeat(4000))
})
