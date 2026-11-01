import { readFileSync } from 'node:fs'
import { z } from 'zod'

// "__" separates server and tool in the names the model sees, so it may not appear in a server name.
const serverName = z.string().regex(/^[a-z0-9_-]{1,32}$/, 'name must be 1-32 of a-z, 0-9, _ and -')
  .refine((name) => !name.includes('__'), 'name may not contain "__"')

const McpServerSchema = z.object({
  name: serverName,
  url: z.url(),
  headers: z.record(z.string(), z.string()).default({}),
  confirm: z.array(z.string()).default([]),
  confirm_destructive: z.boolean().default(true),
})

const ProxyConfigSchema = z.object({
  listen: z.number().int().min(0).max(65535).default(8787),
  client_key: z.string().min(24, 'client_key must be at least 24 characters'),
  upstream: z.object({ base_url: z.url(), api_key: z.string().min(1), model: z.string().optional() }),
  max_tool_steps: z.number().int().min(1).max(20).default(5),
  pending_ttl_minutes: z.number().positive().default(10),
  mcp_servers: z.array(McpServerSchema).default([]),
}).refine((c) => new Set(c.mcp_servers.map((s) => s.name)).size === c.mcp_servers.length,
  { message: 'duplicate mcp_servers name' })

export type McpServerConfig = z.infer<typeof McpServerSchema>
export type ProxyConfig = z.infer<typeof ProxyConfigSchema>

// Replaces "${NAME}" in every string with env.NAME, so keys can stay out of the file.
function expand(value: unknown, env: Record<string, string | undefined>): unknown {
  if (typeof value === 'string') {
    return value.replace(/\$\{([A-Za-z_][A-Za-z0-9_]*)\}/g, (_, name: string) => {
      const found = env[name]
      if (found === undefined) throw new Error(`environment variable ${name} is not set`)
      return found
    })
  }
  if (Array.isArray(value)) return value.map((item) => expand(item, env))
  if (value && typeof value === 'object') {
    return Object.fromEntries(Object.entries(value).map(([k, v]) => [k, expand(v, env)]))
  }
  return value
}

export function parseConfig(raw: unknown, env: Record<string, string | undefined>): ProxyConfig {
  const result = ProxyConfigSchema.safeParse(expand(raw, env))
  if (!result.success) throw new Error(`Invalid config: ${z.prettifyError(result.error)}`)
  return result.data
}

// CONFIG_JSON holds the whole config for hosts that set environment variables but do not mount files.
export function loadConfig(env: Record<string, string | undefined> = process.env): ProxyConfig {
  const text = env.CONFIG_JSON ?? readFileSync(env.CONFIG_PATH ?? './config.json', 'utf8')
  return parseConfig(JSON.parse(text), env)
}
