import { expect, test } from 'vitest'
import { loadConfig, parseConfig } from '../src/config.ts'

const key = 'k'.repeat(24)
const upstream = { base_url: 'https://openrouter.ai/api/v1', api_key: 'sk-or' }

test('expands env and applies defaults', () => {
  const c = parseConfig({ client_key: '${KEY}', upstream: { base_url: 'https://openrouter.ai/api/v1', api_key: '${OR}' },
    mcp_servers: [{ name: 'composio', url: 'https://connect.composio.dev/mcp', headers: { 'x-consumer-api-key': '${CK}' } }] },
    { KEY: key, OR: 'sk-or', CK: 'ck' })
  expect(c.listen).toBe(8787)
  expect(c.max_tool_steps).toBe(5)
  expect(c.pending_ttl_minutes).toBe(10)
  expect(c.client_key).toBe(key)
  expect(c.upstream.api_key).toBe('sk-or')
  expect(c.mcp_servers[0].headers['x-consumer-api-key']).toBe('ck')
  expect(c.mcp_servers[0].confirm).toEqual([])
  expect(c.mcp_servers[0].confirm_destructive).toBe(true)
})

test('missing env var names the variable', () => {
  expect(() => parseConfig({ client_key: '${NOPE}', upstream, mcp_servers: [] }, {})).toThrow(/NOPE/)
})

test('duplicate server names are rejected', () => {
  const server = { name: 'a', url: 'https://a.example/mcp' }
  expect(() => parseConfig({ client_key: key, upstream, mcp_servers: [server, server] }, {})).toThrow(/duplicate/i)
})

test('server names must be short and simple', () => {
  expect(() => parseConfig({ client_key: key, upstream, mcp_servers: [{ name: 'Bad Name', url: 'https://a.example/mcp' }] }, {})).toThrow()
  expect(() => parseConfig({ client_key: key, upstream, mcp_servers: [{ name: 'a__b', url: 'https://a.example/mcp' }] }, {})).toThrow()
})

test('client key must be at least 24 characters', () => {
  expect(() => parseConfig({ client_key: 'short', upstream, mcp_servers: [] }, {})).toThrow(/client_key/)
})

test('confirm list and confirm_destructive are kept', () => {
  const c = parseConfig({ client_key: key, upstream, mcp_servers: [{ name: 'notes', url: 'https://notes.example/mcp', confirm: ['delete_note'], confirm_destructive: false }] }, {})
  expect(c.mcp_servers[0].confirm).toEqual(['delete_note'])
  expect(c.mcp_servers[0].confirm_destructive).toBe(false)
})

test('CONFIG_JSON in the environment is used instead of a file, with ${NAME} still expanded', () => {
  const c = loadConfig({ CONFIG_JSON: JSON.stringify({ client_key: '${KEY}', upstream, mcp_servers: [] }), KEY: key, CONFIG_PATH: '/does/not/exist.json' })
  expect(c.client_key).toBe(key)
})

test('without CONFIG_JSON the file at CONFIG_PATH is read', () => {
  expect(() => loadConfig({ CONFIG_PATH: '/does/not/exist.json' })).toThrow(/ENOENT/)
})
